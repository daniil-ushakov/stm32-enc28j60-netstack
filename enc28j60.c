
#include "enc28j60.h"         // Константы, прототипы и коды результата
#include "stm32f1xx_hal.h"   // Функции HAL для SPI, GPIO и задержек

// Структура SPI1 уже создана в main.c
extern SPI_HandleTypeDef hspi1;

// Указатель следующего непрочитанного пакета в RX-памяти
static uint16_t enc_next_packet = ENC28J60_RX_BUF_START;


/* 1. Низкоуровневые функции SPI */

// Управление линией выбора контроллера CS (PA4)
static void ENC28J60_SetCS(GPIO_PinState state)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, state); // Устанавливаем состояние PA4
}

// Устанавливаем отдельные биты ETH-регистра в текущем банке
static HAL_StatusTypeDef ENC28J60_BitFieldSetRaw(uint8_t regAddr,
                                                 uint8_t mask)
{
    // Команда BFS плюс пятибитный адрес регистра; второй байт - маска
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFS | (regAddr & 0x1F)), mask};
    HAL_StatusTypeDef status; // Результат передачи по SPI

    ENC28J60_SetCS(GPIO_PIN_RESET); // Начало транзакции
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);
    ENC28J60_SetCS(GPIO_PIN_SET);   // Конец транзакции

    return status; // Передаем результат HAL
}

// Сбрасываем отдельные биты ETH-регистра в текущем банке
static HAL_StatusTypeDef ENC28J60_BitFieldClearRaw(uint8_t regAddr,
                                                   uint8_t mask)
{
    // Команда BFC плюс пятибитный адрес регистра; второй байт - маска
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFC | (regAddr & 0x1F)), mask};
    HAL_StatusTypeDef status; // Результат передачи по SPI

    ENC28J60_SetCS(GPIO_PIN_RESET); // Начало транзакции
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);
    ENC28J60_SetCS(GPIO_PIN_SET);   // Конец транзакции

    return status;
}

// Выбор одного из четырех банков регистров ENC28J60
static HAL_StatusTypeDef ENC28J60_SelectBank(uint8_t bank)
{
    HAL_StatusTypeDef status; // Результат команды SPI

    // Сначала сбрасываем оба бита BSEL1 и BSEL0 в ECON1
    status = ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_BSEL0 | ENC28J60_ECON1_BSEL1
    );

    if (status != HAL_OK) // Не продолжаем при ошибке SPI
    {
        return status;
    }

    // Устанавливаем номер требуемого банка двумя младшими битами
    status = ENC28J60_BitFieldSetRaw(
        ENC28J60_REG_ECON1,
        (uint8_t)(bank & 0x03)
    );

    return status;
}

// Читаем один управляющий регистр (адрес+банк+тип заданы в reg)
static uint8_t ENC28J60_ReadControlReg(uint8_t reg)
{
    uint8_t addr = reg & 0x1F;        // Адрес внутри банка
    uint8_t bank = (reg >> 5) & 0x03; // Номер банка
    uint8_t isMacMii = (reg & 0x80) != 0; // Требуется ли dummy-байт
    uint8_t tx[3] = {addr, 0x00, 0x00}; // RCR и байты для тактов чтения
    uint8_t rx[3] = {0};              // Буфер ответа
    uint16_t size = isMacMii ? 3 : 2; // MAC/MII читаем с dummy-байтом
    HAL_StatusTypeDef status;        // Результат HAL

    // Общие регистры 0x1B...0x1F не требуют смены банка
    if (addr < 0x1B)
    {
        status = ENC28J60_SelectBank(bank); // Выбираем нужный банк

        if (status != HAL_OK)
        {
            return 0; // Ошибка чтения
        }
    }

    ENC28J60_SetCS(GPIO_PIN_RESET); // Начинаем чтение регистра

    status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, size,
                                     ENC28J60_SPI_TIMEOUT_MS);

    ENC28J60_SetCS(GPIO_PIN_SET);   // Завершаем чтение

    if (status != HAL_OK)
    {
        return 0;
    }

    return rx[size - 1]; // Значение регистра - последний принятый байт
}

// Записываем байт в управляющий регистр
static HAL_StatusTypeDef ENC28J60_WriteControlReg(uint8_t reg,
                                                  uint8_t value)
{
    uint8_t addr = reg & 0x1F;        // Адрес внутри банка
    uint8_t bank = (reg >> 5) & 0x03; // Номер банка
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_WCR | addr), value};
    HAL_StatusTypeDef status;

    // Общие регистры не требуют выбора банка
    if (addr < 0x1B)
    {
        status = ENC28J60_SelectBank(bank);

        if (status != HAL_OK)
        {
            return status;
        }
    }

    ENC28J60_SetCS(GPIO_PIN_RESET); // Начинаем запись

    status = HAL_SPI_Transmit(&hspi1, tx, 2,
                              ENC28J60_SPI_TIMEOUT_MS);

    ENC28J60_SetCS(GPIO_PIN_SET);   // Завершаем запись

    return status;
}

// Записываем 16-битное число в два соседних регистра L и H
static HAL_StatusTypeDef ENC28J60_WriteControlRegPair(uint8_t reg,
                                                      uint16_t value)
{
    HAL_StatusTypeDef status;

    // Младшие восемь бит записываем в регистр L
    status = ENC28J60_WriteControlReg(reg, (uint8_t)value);

    if (status != HAL_OK)
    {
        return status;
    }

    // Старшие восемь бит записываем в следующий регистр H
    status = ENC28J60_WriteControlReg((uint8_t)(reg + 1),
                                     (uint8_t)(value >> 8));

    return status;
}

// Читаем два соседних регистра L и H как одно 16-битное число
static uint16_t ENC28J60_ReadControlRegPair(uint8_t reg)
{
    // Читаем младший байт
    uint16_t low = ENC28J60_ReadControlReg(reg);

    // Читаем старший байт
    uint16_t high = ENC28J60_ReadControlReg((uint8_t)(reg + 1));

    // Объединяем оба байта в одно число
    return (uint16_t)(low | (high << 8));
}

// Запись последовательности байтов во внутреннюю память ENC28J60
static HAL_StatusTypeDef ENC28J60_WriteBufferMem(uint8_t *data,
                                                 uint16_t size)
{
    uint8_t command = ENC28J60_CMD_WBM; // Команда Write Buffer Memory
    HAL_StatusTypeDef status;

    // Проверяем корректность аргументов
    if (data == 0 || size == 0)
    {
        return HAL_ERROR;
    }

    // CS остается низким до конца передачи массива
    ENC28J60_SetCS(GPIO_PIN_RESET);

    // Передаем команду записи в буферную память
    status = HAL_SPI_Transmit(&hspi1, &command, 1,
                              ENC28J60_SPI_TIMEOUT_MS);

    // Если команда отправлена успешно
    if (status == HAL_OK)
    {
        // Передаем массив байтов в память ENC28J60
        status = HAL_SPI_Transmit(&hspi1, data, size,
                                  ENC28J60_SPI_TIMEOUT_MS);
    }

    // Завершаем обмен
    ENC28J60_SetCS(GPIO_PIN_SET);

    return status;
}

// Чтение последовательности байтов из внутренней памяти ENC28J60
static HAL_StatusTypeDef ENC28J60_ReadBufferMem(uint8_t *data,
                                                uint16_t size)
{
    uint8_t command = ENC28J60_CMD_RBM; // Команда Read Buffer Memory
    HAL_StatusTypeDef status;

    // Проверяем корректность аргументов
    if (data == 0 || size == 0)
    {
        return HAL_ERROR;
    }

    // CS остается низким до конца чтения
    ENC28J60_SetCS(GPIO_PIN_RESET);

    // Передаем команду чтения буферной памяти
    status = HAL_SPI_Transmit(&hspi1, &command, 1,
                              ENC28J60_SPI_TIMEOUT_MS);

    // Если команда отправлена успешно
    if (status == HAL_OK)
    {
        // Принимаем заданное количество байтов
        status = HAL_SPI_Receive(&hspi1, data, size,
                                 ENC28J60_SPI_TIMEOUT_MS);
    }

    // Завершаем обмен
    ENC28J60_SetCS(GPIO_PIN_SET);

    return status;
}


/* 2. Сброс и настройка ENC28J60 */

// Программный сброс через команду SRC
uint8_t ENC28J60_SoftReset(void)
{
    uint8_t command = ENC28J60_CMD_SRC; // Код команды сброса 0xFF
    HAL_StatusTypeDef status;

    // Выбираем контроллер
    ENC28J60_SetCS(GPIO_PIN_RESET);

    // Передаем команду программного сброса
    status = HAL_SPI_Transmit(&hspi1, &command, 1,
                              ENC28J60_SPI_TIMEOUT_MS);

    // Завершаем транзакцию
    ENC28J60_SetCS(GPIO_PIN_SET);

    if (status != HAL_OK)
    {
        return 0; // Команду передать не удалось
    }

    // Выдерживаем паузу после сброса
    HAL_Delay(ENC28J60_RESET_DELAY_MS);

    // Команда передана успешно
    return 1;
}

// Получаем аппаратную ревизию ENC28J60
uint8_t ENC28J60_ReadRevision(void)
{
    // Регистр EREVID находится в банке 3
    return ENC28J60_ReadControlReg(ENC28J60_REG_EREVID);
}

// Настройка границ кольцевого RX-буфера
uint8_t ENC28J60_ConfigureBuffers(void)
{
    HAL_StatusTypeDef status;

    // Устанавливаем начало кольцевого RX-буфера
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ERXSTL,
        ENC28J60_RX_BUF_START
    );

    if (status != HAL_OK) return 0;

    // Устанавливаем конец RX-буфера
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ERXNDL,
        ENC28J60_RX_BUF_END
    );

    if (status != HAL_OK) return 0;

    // Устанавливаем указатель освобожденной памяти
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ERXRDPTL,
        ENC28J60_RX_BUF_END
    );

    if (status != HAL_OK) return 0;

    // Устанавливаем указатель чтения на начало RX-буфера
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ERDPTL,
        ENC28J60_RX_BUF_START
    );

    if (status != HAL_OK) return 0;

    // Проверяем начало RX-буфера
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXSTL)
        != ENC28J60_RX_BUF_START)
    {
        return 0;
    }

    // Проверяем конец RX-буфера
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXNDL)
        != ENC28J60_RX_BUF_END)
    {
        return 0;
    }

    // Проверяем указатель освобождения памяти
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXRDPTL)
        != ENC28J60_RX_BUF_END)
    {
        return 0;
    }

    // Проверяем указатель чтения памяти
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERDPTL)
        != ENC28J60_RX_BUF_START)
    {
        return 0;
    }

    // Возвращаем программный указатель в начало RX
    enc_next_packet = ENC28J60_RX_BUF_START;

    return 1;
}

// Настройка параметров MAC-контроллера
uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex)
{
    HAL_StatusTypeDef status;
    uint8_t macon3_value = ENC28J60_MACON3_VALUE;
    uint8_t mabbipg_value = ENC28J60_MABBIPG_VALUE;
    uint16_t maipg_value = ENC28J60_MAIPG_VALUE;

    // Проверяем выбранный режим Duplex
    if (duplex != ENC28J60_DUPLEX_HALF &&
        duplex != ENC28J60_DUPLEX_FULL)
    {
        return 0;
    }

    // Переключать режим допускается только при остановленных RX и TX
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1) &
        (ENC28J60_ECON1_RXEN | ENC28J60_ECON1_TXRTS))
    {
        return 0;
    }

    // Для Full Duplex отличаются FULDPX и межкадровые интервалы
    if (duplex == ENC28J60_DUPLEX_FULL)
    {
        macon3_value |= ENC28J60_MACON3_FULDPX;
        mabbipg_value = ENC28J60_MABBIPG_FULL;
        maipg_value = ENC28J60_MAIPG_FULL;
    }

    // Разрешаем прием MAC и PAUSE-кадры
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MACON1,
        ENC28J60_MACON1_VALUE
    );

    if (status != HAL_OK) return 0;

    // Выбранный Duplex, дополнение коротких кадров, CRC и контроль длины
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MACON3,
        macon3_value
    );

    if (status != HAL_OK) return 0;

    // Интервал между последовательными кадрами
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MABBIPG,
        mabbipg_value
    );

    if (status != HAL_OK) return 0;

    // Интервал Non-Back-to-Back, два регистра
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_MAIPGL,
        maipg_value
    );

    if (status != HAL_OK) return 0;

    // Максимальный Ethernet-кадр вместе с FCS
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_MAMXFLL,
        ENC28J60_MAMXFL_VALUE
    );

    if (status != HAL_OK) return 0;

    // Проверяем записанные значения MACON1
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON1)
        != ENC28J60_MACON1_VALUE)
    {
        return 0;
    }

    // Проверяем MACON3
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON3)
        != macon3_value)
    {
        return 0;
    }

    // Проверяем MABBIPG
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MABBIPG)
        != mabbipg_value)
    {
        return 0;
    }

    // Проверяем MAIPG
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAIPGL)
        != maipg_value)
    {
        return 0;
    }

    // Проверяем максимальную длину кадра
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAMXFLL)
        != ENC28J60_MAMXFL_VALUE)
    {
        return 0;
    }

    return 1;
}

// Запись и проверка шести байтов MAC-адреса устройства
uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac)
{
    // Адреса шести регистров MAC-контроллера
    const uint8_t regs[ENC28J60_MAC_ADDRESS_SIZE] =
    {
        ENC28J60_REG_MAADR1,
        ENC28J60_REG_MAADR2,
        ENC28J60_REG_MAADR3,
        ENC28J60_REG_MAADR4,
        ENC28J60_REG_MAADR5,
        ENC28J60_REG_MAADR6
    };

    HAL_StatusTypeDef status;
    uint8_t i;

    // Проверяем указатель на MAC-адрес
    if (mac == 0)
    {
        return 0;
    }

    // Записываем MAC-адрес побайтно
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)
    {
        status = ENC28J60_WriteControlReg(
            regs[i],
            mac[i]
        );

        if (status != HAL_OK)
        {
            return 0;
        }
    }

    // Проверяем каждый байт чтением
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)
    {
        if (ENC28J60_ReadControlReg(regs[i])
            != mac[i])
        {
            return 0;
        }
    }

    return 1;
}


/* 3. Работа с PHY */

// Чтение 16-битного PHY-регистра через интерфейс MIIM
uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value)
{
    HAL_StatusTypeDef status;
    uint32_t startTime;

    // Проверяем адрес переменной для результата
    if (value == 0)
    {
        return 0;
    }

    // Очищаем результат перед чтением
    *value = 0;

    // Выбираем адрес регистра PHY
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MIREGADR,
        reg
    );

    if (status != HAL_OK) return 0;

    // Запускаем чтение PHY установкой MICMD.MIIRD
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MICMD,
        ENC28J60_MICMD_MIIRD
    );

    if (status != HAL_OK) return 0;

    // Даем MIIM время на выполнение операции
    HAL_Delay(1);

    // Запоминаем начало ожидания BUSY
    startTime = HAL_GetTick();

    // Проверяем, что интерфейс MIIM освободился
    while (ENC28J60_ReadControlReg(ENC28J60_REG_MISTAT)
           & ENC28J60_MISTAT_BUSY)
    {
        // Защита от зависания
        if ((HAL_GetTick() - startTime) >= ENC28J60_PHY_TIMEOUT_MS)
        {
            // При превышении времени ожидания отключаем MIIRD
            ENC28J60_WriteControlReg(ENC28J60_REG_MICMD, 0);

            return 0;
        }
    }

    // Обязательно сбрасываем MIIRD после завершения чтения
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MICMD,
        0x00
    );

    if (status != HAL_OK) return 0;

    // Считываем два байта результата MIRDL/MIRDH
    *value = ENC28J60_ReadControlRegPair(ENC28J60_REG_MIRDL);

    return 1;
}

// Запись 16-битного значения в PHY-регистр
uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value)
{
    HAL_StatusTypeDef status;
    uint32_t startTime;

    // Задаем адрес PHY-регистра
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MIREGADR,
        reg
    );

    if (status != HAL_OK) return 0;

    // Записываем MIWRL, затем MIWRH
    // Запись старшего байта запускает операцию MIIM
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_MIWRL,
        value
    );

    if (status != HAL_OK) return 0;

    // Ждем завершения внутренней записи
    HAL_Delay(1);

    // Запоминаем начало проверки BUSY
    startTime = HAL_GetTick();

    while (ENC28J60_ReadControlReg(ENC28J60_REG_MISTAT)
           & ENC28J60_MISTAT_BUSY)
    {
        if ((HAL_GetTick() - startTime) >= ENC28J60_PHY_TIMEOUT_MS)
        {
            return 0; // Интерфейс MIIM не освободился
        }
    }

    return 1;
}

// Устанавливаем фильтры приема и включаем RXEN
uint8_t ENC28J60_StartReceiving(void)
{
    HAL_StatusTypeDef status;

    // Выключаем RXEN на время изменения настроек
    status = ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_RXEN
    );

    if (status != HAL_OK) return 0;

    // Включаем MACON4.DEFER для полудуплексной передачи
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_MACON4,
        ENC28J60_MACON4_VALUE
    );

    if (status != HAL_OK) return 0;

    // Разрешаем Unicast, Broadcast и проверку CRC
    status = ENC28J60_WriteControlReg(
        ENC28J60_REG_ERXFCON,
        ENC28J60_ERXFCON_VALUE
    );

    if (status != HAL_OK) return 0;

    // Проверяем MACON4
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON4)
        != ENC28J60_MACON4_VALUE)
    {
        return 0;
    }

    // Проверяем ERXFCON
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ERXFCON)
        != ENC28J60_ERXFCON_VALUE)
    {
        return 0;
    }

    // Включаем прием кадров через ECON1.RXEN
    status = ENC28J60_BitFieldSetRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_RXEN
    );

    if (status != HAL_OK) return 0;

    // Проверяем, что бит RXEN действительно установлен
    if ((ENC28J60_ReadControlReg(ENC28J60_REG_ECON1)
         & ENC28J60_ECON1_RXEN) == 0)
    {
        return 0;
    }

    return 1;
}

// Полная последовательная инициализация ENC28J60
ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,
                                   ENC28J60_DuplexMode duplex)
{
    uint8_t revision;       // Аппаратная ревизия
    uint16_t phy_value = 0; // Проверка регистров PHY
    uint16_t phy_duplex;    // Значение PHCON1.PDPXMD
    uint16_t phy_loopback;  // Значение PHCON2.HDLDIS
    uint8_t i;              // Индекс байта MAC-адреса
    uint8_t mac_nonzero = 0;

    // Проверяем аргументы до обращения к контроллеру
    if (mac == 0 ||
        (duplex != ENC28J60_DUPLEX_HALF &&
         duplex != ENC28J60_DUPLEX_FULL))
    {
        return ENC28J60_INIT_ERR_PARAM;
    }

    // MAC-адрес должен быть индивидуальным (Unicast)
    if ((mac[0] & 0x01) != 0)
    {
        return ENC28J60_INIT_ERR_PARAM;
    }

    // Нулевой MAC-адрес использовать нельзя
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)
    {
        if (mac[i] != 0)
        {
            mac_nonzero = 1;
        }
    }

    if (mac_nonzero == 0)
    {
        return ENC28J60_INIT_ERR_PARAM;
    }

    // Параметры PHY для выбранного режима работы
    phy_duplex = (duplex == ENC28J60_DUPLEX_FULL)
               ? ENC28J60_PHCON1_PDPXMD : 0;

    // Отключаем внутреннюю петлю только в Half Duplex
    phy_loopback = (duplex == ENC28J60_DUPLEX_HALF)
                 ? ENC28J60_PHCON2_VALUE : 0;

    // Выполняем программный сброс
    if (ENC28J60_SoftReset() != 1)
    {
        return ENC28J60_INIT_ERR_RESET;
    }

    // Читаем EREVID после сброса
    revision = ENC28J60_ReadRevision();

    if (revision == 0x00 || revision == 0xFF)
    {
        return ENC28J60_INIT_ERR_REVISION;
    }

    // Настраиваем буферную память
    if (ENC28J60_ConfigureBuffers() != 1)
    {
        return ENC28J60_INIT_ERR_BUFFERS;
    }

    // Настраиваем регистры MAC
    if (ENC28J60_ConfigureMAC(duplex) != 1)
    {
        return ENC28J60_INIT_ERR_MAC;
    }

    // Записываем MAC-адрес
    if (ENC28J60_ConfigureMACAddress(mac) != 1)
    {
        return ENC28J60_INIT_ERR_ADDRESS;
    }

    // Согласовываем режим PHY с ранее настроенным MACON3
    // Во время настройки передатчик и приемник еще выключены
    if (ENC28J60_WritePHY(
        ENC28J60_PHY_PHCON1,
        phy_duplex) != 1)
    {
        return ENC28J60_INIT_ERR_PHY;
    }

    // Проверяем бит PHY.PDPXMD
    if (ENC28J60_ReadPHY(
        ENC28J60_PHY_PHCON1,
        &phy_value) != 1 ||
        (phy_value & ENC28J60_PHCON1_PDPXMD) != phy_duplex)
    {
        return ENC28J60_INIT_ERR_PHY;
    }

    // Для Half Duplex отключаем автоматическую внутреннюю петлю PHY
    if (ENC28J60_WritePHY(
        ENC28J60_PHY_PHCON2,
        phy_loopback) != 1)
    {
        return ENC28J60_INIT_ERR_PHY;
    }

    // Проверяем бит PHY.HDLDIS
    if (ENC28J60_ReadPHY(
        ENC28J60_PHY_PHCON2,
        &phy_value) != 1 ||
        (phy_value & ENC28J60_PHCON2_HDLDIS) != phy_loopback)
    {
        return ENC28J60_INIT_ERR_PHY;
    }

    // Настраиваем фильтры и включаем прием
    if (ENC28J60_StartReceiving() != 1)
    {
        return ENC28J60_INIT_ERR_RX;
    }

    // Все этапы выполнены успешно
    return ENC28J60_INIT_OK;
}


/* 4. Работа с Ethernet-кадрами */

// Возвращаем количество пакетов, ожидающих обработки
uint8_t ENC28J60_GetPacketCount(void)
{
    // Читаем счетчик EPKTCNT
    return ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT);
}

// Готовим кадр к передаче
static uint8_t ENC28J60_PrepareTxFrame(uint8_t *data,
                                      uint16_t size)
{
    // Управляющий байт: используем настройки MACON3
    uint8_t controlByte = 0x00;
    HAL_StatusTypeDef status;

    // Проверяем адрес данных и допустимый размер кадра
    if (data == 0 ||
        size < ENC28J60_FRAME_MIN ||
        size > ENC28J60_TX_FRAME_MAX)
    {
        return 0;
    }

    // Передатчик не должен выполнять предыдущую передачу
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1)
        & ENC28J60_ECON1_TXRTS)
    {
        return 0;
    }

    // Устанавливаем EWRPT на начало TX-области
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_EWRPTL,
        ENC28J60_TX_BUF_START
    );

    if (status != HAL_OK) return 0;

    // Первым записываем управляющий байт кадра
    status = ENC28J60_WriteBufferMem(&controlByte, 1);

    if (status != HAL_OK) return 0;

    // Записываем Ethernet-кадр без FCS
    status = ENC28J60_WriteBufferMem(data, size);

    if (status != HAL_OK) return 0;

    // ETXST указывает на управляющий байт
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ETXSTL,
        ENC28J60_TX_BUF_START
    );

    if (status != HAL_OK) return 0;

    // ETXND указывает на последний байт самого кадра
    status = ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ETXNDL,
        ENC28J60_TX_BUF_START + size
    );

    if (status != HAL_OK) return 0;

    return 1;
}

// Сбрасываем передающую логику и старые флаги
static uint8_t ENC28J60_ResetTxLogic(void)
{
    // Снимаем запрос предыдущей передачи
    if (ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRTS) != HAL_OK)
    {
        return 0;
    }

    // Устанавливаем бит аппаратного сброса TXRST
    if (ENC28J60_BitFieldSetRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRST) != HAL_OK)
    {
        return 0;
    }

    // Снимаем бит TXRST
    if (ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRST) != HAL_OK)
    {
        return 0;
    }

    // Очищаем флаги ошибки и завершения передачи
    if (ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_EIR,
        ENC28J60_EIR_TXERIF | ENC28J60_EIR_TXIF) != HAL_OK)
    {
        return 0;
    }

    return 1;
}

// Передаем готовый Ethernet-кадр без FCS
ENC28J60_TxResult ENC28J60_TransmitFrame(uint8_t *data,
                                         uint16_t size)
{
    uint16_t phy_status = 0; // Состояние физического соединения
    uint32_t startTime;       // Начало ожидания передачи
    uint8_t flags;            // Флаги событий ENC28J60

    // Проверяем длину и указатель
    if (data == 0 ||
        size < ENC28J60_FRAME_MIN ||
        size > ENC28J60_TX_FRAME_MAX)
    {
        return ENC28J60_TX_INVALID_FRAME;
    }

    // Проверяем, что предыдущая передача завершена
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1)
        & ENC28J60_ECON1_TXRTS)
    {
        return ENC28J60_TX_BUSY;
    }

    // Читаем состояние физического интерфейса
    if (ENC28J60_ReadPHY(
        ENC28J60_PHY_PHSTAT2,
        &phy_status) != 1)
    {
        return ENC28J60_TX_ERROR;
    }

    // Без соединения возвращаем специальный код
    if ((phy_status & ENC28J60_PHSTAT2_LSTAT) == 0)
    {
        return ENC28J60_TX_NO_LINK;
    }

    // Сбрасываем передатчик и старые флаги
    if (ENC28J60_ResetTxLogic() != 1)
    {
        return ENC28J60_TX_ERROR;
    }

    // Записываем кадр в TX-память
    if (ENC28J60_PrepareTxFrame(data, size) != 1)
    {
        return ENC28J60_TX_ERROR;
    }

    // Запоминаем время начала передачи
    startTime = HAL_GetTick();

    // Устанавливаем TXRTS
    if (ENC28J60_BitFieldSetRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRTS) != HAL_OK)
    {
        return ENC28J60_TX_ERROR;
    }

    // Ожидаем результат передачи
    while (1)
    {
        // Читаем флаги событий
        flags = ENC28J60_ReadControlReg(ENC28J60_REG_EIR);

        // Проверяем наличие ошибки
        if (flags & ENC28J60_EIR_TXERIF)
        {
            ENC28J60_ResetTxLogic();

            return ENC28J60_TX_ERROR;
        }

        // Проверяем успешное завершение
        if (flags & ENC28J60_EIR_TXIF)
        {
            return ENC28J60_TX_OK;
        }

        // Защита от бесконечного ожидания
        if ((HAL_GetTick() - startTime) >= ENC28J60_TX_TIMEOUT_MS)
        {
            ENC28J60_ResetTxLogic();

            return ENC28J60_TX_TIMEOUT;
        }
    }
}

// Извлекаем один принятый Ethernet-кадр из кольцевого RX-буфера
ENC28J60_RxResult ENC28J60_ReceiveFrame(uint8_t *data,
                                        uint16_t capacity,
                                        uint16_t *receivedLen)
{
    // Служебные 6 байт перед принятым кадром
    uint8_t header[ENC28J60_RX_HEADER_SIZE] = {0};

    uint16_t nextPacket; // Адрес следующего пакета
    uint16_t frameSize;  // Размер кадра, включая FCS
    uint16_t rxStatus;   // Статус приема
    uint16_t dataSize;   // Длина кадра без FCS
    uint16_t freePtr;    // Граница освобождаемой памяти

    // Результат обработки кадра
    ENC28J60_RxResult result = ENC28J60_RX_BAD_FRAME;

    // Проверяем аргументы
    if (data == 0 || receivedLen == 0 || capacity == 0)
    {
        return ENC28J60_RX_INVALID_ARG;
    }

    // До успешного приема длина равна нулю
    *receivedLen = 0;

    // Если пакетов нет, буфер читать не требуется
    if (ENC28J60_GetPacketCount() == 0)
    {
        return ENC28J60_RX_EMPTY;
    }

    // Устанавливаем указатель чтения на текущий пакет
    if (ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ERDPTL,
        enc_next_packet) != HAL_OK)
    {
        return ENC28J60_RX_SPI_ERROR;
    }

    // Считываем служебный заголовок
    if (ENC28J60_ReadBufferMem(
        header,
        ENC28J60_RX_HEADER_SIZE) != HAL_OK)
    {
        return ENC28J60_RX_SPI_ERROR;
    }

    // Получаем указатель следующего пакета
    nextPacket = (uint16_t)(
        header[0] | ((uint16_t)header[1] << 8)
    );

    // Получаем длину принятого кадра вместе с FCS
    frameSize = (uint16_t)(
        header[2] | ((uint16_t)header[3] << 8)
    );

    // Получаем оставшиеся 16 бит статуса приема
    rxStatus = (uint16_t)(
        header[4] | ((uint16_t)header[5] << 8)
    );

    // Проверяем адрес следующего пакета
    // Он должен быть четным и лежать в RX-области
    if (nextPacket > ENC28J60_RX_BUF_END ||
        (nextPacket & 1) != 0)
    {
        return ENC28J60_RX_BAD_POINTER;
    }

    // Проверяем успешность приема и допустимую длину
    // RX_FRAME_MAX = 1518, включая FCS
    if ((rxStatus & ENC28J60_RX_STATUS_OK) != 0 &&
        frameSize >= (ENC28J60_FRAME_MIN + ENC28J60_CRC_SIZE) &&
        frameSize <= ENC28J60_RX_FRAME_MAX)
    {
        // Убираем четыре байта FCS из длины
        dataSize = frameSize - ENC28J60_CRC_SIZE;

        // Проверяем вместимость массива STM32
        if (dataSize > capacity)
        {
            result = ENC28J60_RX_BUFFER_SMALL;
        }
        else
        {
            // Читаем Ethernet-кадр без FCS
            if (ENC28J60_ReadBufferMem(
                data,
                dataSize) != HAL_OK)
            {
                return ENC28J60_RX_SPI_ERROR;
            }

            // Сохраняем длину принятого кадра
            *receivedLen = dataSize;

            // Прием успешно завершен
            result = ENC28J60_RX_OK;
        }
    }

    // Особенность ревизии B7:
    // ERXRDPT должен получать только нечетные адреса
    if (nextPacket == ENC28J60_RX_BUF_START)
    {
        // При возврате в начало кольцевого буфера
        freePtr = ENC28J60_RX_BUF_END;
    }
    else
    {
        // Освобождаем область до следующего пакета
        freePtr = nextPacket - 1;
    }

    // Обновляем указатель освобожденной RX-памяти
    if (ENC28J60_WriteControlRegPair(
        ENC28J60_REG_ERXRDPTL,
        freePtr) != HAL_OK)
    {
        return ENC28J60_RX_SPI_ERROR;
    }

    // Запоминаем адрес следующего пакета
    enc_next_packet = nextPacket;

    // Уменьшаем счетчик принятых пакетов
    if (ENC28J60_BitFieldSetRaw(
        ENC28J60_REG_ECON2,
        ENC28J60_ECON2_PKTDEC) != HAL_OK)
    {
        return ENC28J60_RX_SPI_ERROR;
    }

    return result;
}

// Получение PHSTAT2 для диагностики Link и Duplex
uint16_t ENC28J60_GetPHYStatus(void)
{
    // При ошибке вернем ноль
    uint16_t status = 0;

    // Читаем регистр состояния PHY
    if (ENC28J60_ReadPHY(
        ENC28J60_PHY_PHSTAT2,
        &status) != 1)
    {
        return 0;
    }

    // Возвращаем состояние PHY
    return status;
}
