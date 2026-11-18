// Библиотека для работы с модулем ENC28J60
// Автор: Ушаков Д.Д.
// Контроллер STM32F103, библиотека HAL

//////////////////////////////////////////////////////////////////////////////////////////////////

#include "enc28j60.h"                                                                           // Объявления функций и регистров ENC28J60
#include "stm32f1xx_hal.h"                                                                      // Функции HAL для STM32
extern SPI_HandleTypeDef hspi1;                                                                 // SPI1 настраивается в основном проекте
static uint16_t enc_next_packet = ENC28J60_RX_BUF_START;                                        // Адрес следующего кадра в кольцевом RX-буфере
static uint8_t enc_current_bank = 0xFF;                                                         // Текущий банк регистров; 0xFF означает неизвестный

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ENC28J60_SetCS(GPIO_PinState state)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, state);                                                // Опускаем CS для начала обмена или поднимаем для завершения
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_BitFieldSetRaw(uint8_t regAddr,
                                                 uint8_t mask)                                  // Биты регистра, которые требуется изменить
{
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFS | (regAddr & 0x1F)), mask};                     // Байты команды и данных для отправки по SPI
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем ENC28J60 для обмена по SPI
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);                          // Отправляем команду или данные контроллеру по SPI
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Завершаем обмен по SPI и отпускаем ENC28J60
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_BitFieldClearRaw(uint8_t regAddr,
                                                   uint8_t mask)                                // Биты регистра, которые требуется изменить
{
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFC | (regAddr & 0x1F)), mask};                     // Байты команды и данных для отправки по SPI
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем ENC28J60 для обмена по SPI
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);                          // Отправляем команду или данные контроллеру по SPI
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Завершаем обмен по SPI и отпускаем ENC28J60
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_SelectBank(uint8_t bank)
{
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    bank &= 0x03;
    if (enc_current_bank == bank) return HAL_OK;                                                // Прекращаем работу при ошибке обмена по SPI
    status = ENC28J60_BitFieldClearRaw(                                                         // Сбрасываем нужные управляющие биты ENC28J60
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_BSEL0 | ENC28J60_ECON1_BSEL1);
    if (status != HAL_OK)                                                                       // Прекращаем работу при ошибке обмена по SPI
    {
        enc_current_bank = 0xFF;                                                                // Запоминаем выбранный банк, чтобы не переключать его повторно
        return status;                                                                          // Передаём результат SPI-операции вызывающей функции
    }
    status = ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1, bank);                                 // Устанавливаем нужные управляющие биты ENC28J60
    enc_current_bank = (status == HAL_OK) ? bank : 0xFF;                                        // Запоминаем выбранный банк, чтобы не переключать его повторно
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_ReadControlReg(uint8_t reg,
                                                uint8_t *value)                                 // Считанное значение регистра
{
    uint8_t addr = reg & 0x1F;                                                                  // Адрес регистра внутри выбранного банка
    uint8_t bank = (reg >> 5) & 0x03;                                                           // Номер банка регистров ENC28J60
    uint8_t isMacMii = (reg & 0x80) != 0;                                                       // Признак регистра MAC/MII, которому нужен пустой байт SPI
    uint8_t tx[3] = {addr, 0x00, 0x00};                                                         // Байты команды и данных для отправки по SPI
    uint8_t rx[3] = {0};                                                                        // Буфер для ответа контроллера по SPI
    uint16_t size = isMacMii ? 3 : 2;                                                           // Количество байтов для обмена по SPI
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    if (value == 0) return HAL_ERROR;                                                           // Проверяем, что передан действительный указатель
    *value = 0;                                                                                 // Сохраняем значение для последующей проверки
    if (addr < 0x1B)                                                                            // Банковые регистры требуют переключения банка
    {
        status = ENC28J60_SelectBank(bank);                                                     // Выбираем банк, содержащий нужный регистр
        if (status != HAL_OK) return status;                                                    // Прекращаем работу при ошибке обмена по SPI
    }
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем ENC28J60 для обмена по SPI
    status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, size,                                      // Одновременно отправляем команду и читаем ответ SPI
                                     ENC28J60_SPI_TIMEOUT_MS);                                  // Завершаем вызов функции
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Завершаем обмен по SPI и отпускаем ENC28J60
    if (status == HAL_OK) *value = rx[size - 1];                                                // Прекращаем работу при ошибке обмена по SPI
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_WriteControlReg(uint8_t reg,
                                                  uint8_t value)                                // Считанное значение регистра
{
    uint8_t addr = reg & 0x1F;                                                                  // Адрес регистра внутри выбранного банка
    uint8_t bank = (reg >> 5) & 0x03;                                                           // Номер банка регистров ENC28J60
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_WCR | addr), value};                                // Байты команды и данных для отправки по SPI
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    if (addr < 0x1B)                                                                            // Банковые регистры требуют переключения банка
    {
        status = ENC28J60_SelectBank(bank);                                                     // Выбираем банк, содержащий нужный регистр
        if (status != HAL_OK)                                                                   // Прекращаем работу при ошибке обмена по SPI
        {
            return status;                                                                      // Передаём результат SPI-операции вызывающей функции
        }
    }
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем ENC28J60 для обмена по SPI
    status = HAL_SPI_Transmit(&hspi1, tx, 2,                                                    // Отправляем команду или данные контроллеру по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Завершаем обмен по SPI и отпускаем ENC28J60
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_WriteControlRegPair(uint8_t reg,
                                                      uint16_t value)                           // Считанное значение регистра
{
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    status = ENC28J60_WriteControlReg(reg, (uint8_t)value);                                     // Записываем настройку в регистр ENC28J60
    if (status != HAL_OK)                                                                       // Прекращаем работу при ошибке обмена по SPI
    {
        return status;                                                                          // Передаём результат SPI-операции вызывающей функции
    }
    status = ENC28J60_WriteControlReg((uint8_t)(reg + 1),                                       // Записываем настройку в регистр ENC28J60
                                     (uint8_t)(value >> 8));
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_ReadControlRegPair(uint8_t reg,
                                                    uint16_t *value)                            // Считанное значение регистра
{
    uint8_t low, high;                                                                          // Младший и старший байты двухбайтового регистра
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    if (value == 0) return HAL_ERROR;                                                           // Проверяем, что передан действительный указатель
    *value = 0;                                                                                 // Сохраняем значение для последующей проверки
    status = ENC28J60_ReadControlReg(reg, &low);                                                // Читаем регистр ENC28J60 для проверки состояния
    if (status != HAL_OK) return status;                                                        // Прекращаем работу при ошибке обмена по SPI
    status = ENC28J60_ReadControlReg((uint8_t)(reg + 1), &high);                                // Читаем регистр ENC28J60 для проверки состояния
    if (status != HAL_OK) return status;                                                        // Прекращаем работу при ошибке обмена по SPI
    *value = (uint16_t)((uint16_t)low | ((uint16_t)high << 8));                                 // Сохраняем значение для последующей проверки
    return HAL_OK;                                                                              // Возвращаем результат проверки или считанное значение
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_WriteBufferMem(uint8_t *data,
                                                 uint16_t size)                                 // Количество байтов для обмена по SPI
{
    uint8_t command = ENC28J60_CMD_WBM;                                                         // Команда для передачи контроллеру по SPI
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    if (data == 0 || size == 0)                                                                 // Проверяем, что передан действительный указатель
    {
        return HAL_ERROR;                                                                       // Возвращаем результат проверки или считанное значение
    }
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем ENC28J60 для обмена по SPI
    status = HAL_SPI_Transmit(&hspi1, &command, 1,                                              // Отправляем команду или данные контроллеру по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    if (status == HAL_OK)                                                                       // Прекращаем работу при ошибке обмена по SPI
    {
        status = HAL_SPI_Transmit(&hspi1, data, size,                                           // Отправляем команду или данные контроллеру по SPI
                                  ENC28J60_SPI_TIMEOUT_MS);                                     // Завершаем вызов функции
    }
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Завершаем обмен по SPI и отпускаем ENC28J60
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_ReadBufferMem(uint8_t *data,
                                                uint16_t size)                                  // Количество байтов для обмена по SPI
{
    uint8_t command = ENC28J60_CMD_RBM;                                                         // Команда для передачи контроллеру по SPI
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    if (data == 0 || size == 0)                                                                 // Проверяем, что передан действительный указатель
    {
        return HAL_ERROR;                                                                       // Возвращаем результат проверки или считанное значение
    }
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем ENC28J60 для обмена по SPI
    status = HAL_SPI_Transmit(&hspi1, &command, 1,                                              // Отправляем команду или данные контроллеру по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    if (status == HAL_OK)                                                                       // Прекращаем работу при ошибке обмена по SPI
    {
        status = HAL_SPI_Receive(&hspi1, data, size,                                            // Читаем данные из внутренней памяти контроллера
                                 ENC28J60_SPI_TIMEOUT_MS);                                      // Завершаем вызов функции
    }
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Завершаем обмен по SPI и отпускаем ENC28J60
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_SoftReset(void)
{
    uint8_t command = ENC28J60_CMD_SRC;                                                         // Команда для передачи контроллеру по SPI
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем ENC28J60 для обмена по SPI
    status = HAL_SPI_Transmit(&hspi1, &command, 1,                                              // Отправляем команду или данные контроллеру по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Завершаем обмен по SPI и отпускаем ENC28J60
    if (status != HAL_OK)                                                                       // Прекращаем работу при ошибке обмена по SPI
    {
        return 0;                                                                               // Сообщаем об ошибке
    }
    enc_current_bank = 0xFF;                                                                    // Запоминаем выбранный банк, чтобы не переключать его повторно
    HAL_Delay(ENC28J60_RESET_DELAY_MS);                                                         // Ждём готовности контроллера после сброса
    return 1;                                                                                   // Сообщаем об успешном выполнении
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_ReadRevision(void)
{
    uint8_t value = 0;                                                                          // Считанное значение регистра
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EREVID, &value) != HAL_OK)                         // Прекращаем работу при ошибке обмена по SPI
        return 0;                                                                               // Сообщаем об ошибке
    return value;                                                                               // Передаём считанное значение вызывающей функции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_ConfigureBuffers(void)
{
    uint16_t check;                                                                             // Считанное значение для проверки записи регистров
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXSTL,                                       // Проверяем, удалось ли записать адрес в регистры
                                    ENC28J60_RX_BUF_START) != HAL_OK) return 0;                 // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXNDL,                                       // Проверяем, удалось ли записать адрес в регистры
                                    ENC28J60_RX_BUF_END) != HAL_OK) return 0;                   // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXRDPTL,                                     // Проверяем, удалось ли записать адрес в регистры
                                    ENC28J60_RX_BUF_END) != HAL_OK) return 0;                   // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,                                       // Проверяем, удалось ли записать адрес в регистры
                                    ENC28J60_RX_BUF_START) != HAL_OK) return 0;                 // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,                                             // Проверяем установку управляющих битов
                               ENC28J60_ECON2_AUTOINC) != HAL_OK) return 0;                     // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXSTL, &check) != HAL_OK ||                   // Проверяем чтение двухбайтового регистра
        check != ENC28J60_RX_BUF_START) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXNDL, &check) != HAL_OK ||                   // Проверяем чтение двухбайтового регистра
        check != ENC28J60_RX_BUF_END) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXRDPTL, &check) != HAL_OK ||                 // Проверяем чтение двухбайтового регистра
        check != ENC28J60_RX_BUF_END) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERDPTL, &check) != HAL_OK ||                   // Проверяем чтение двухбайтового регистра
        check != ENC28J60_RX_BUF_START) return 0;
    enc_next_packet = ENC28J60_RX_BUF_START;                                                    // Сохраняем адрес следующего кадра для очередного чтения
    return 1;                                                                                   // Сообщаем об успешном выполнении
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex)
{
    uint8_t reg_value;                                                                          // Значение регистра для проверки настройки
    uint16_t check;                                                                             // Считанное значение для проверки записи регистров
    uint8_t macon3_value = ENC28J60_MACON3_VALUE;                                               // Параметры MAC для выбранного Duplex
    uint8_t mabbipg_value = ENC28J60_MABBIPG_VALUE;                                             // Интервал между соседними кадрами
    uint16_t maipg_value = ENC28J60_MAIPG_VALUE;                                                // Интервал между Ethernet-кадрами
    if (duplex != ENC28J60_DUPLEX_HALF &&                                                       // Проверяем выбранный режим Duplex
        duplex != ENC28J60_DUPLEX_FULL) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &reg_value) != HAL_OK)                      // Прекращаем работу при ошибке обмена по SPI
        return 0;                                                                               // Сообщаем об ошибке
    if (reg_value & (ENC28J60_ECON1_RXEN | ENC28J60_ECON1_TXRTS))                               // Проверяем, не занята ли передача кадра
        return 0;                                                                               // Сообщаем об ошибке
    if (duplex == ENC28J60_DUPLEX_FULL)                                                         // Проверяем выбранный режим Duplex
    {
        macon3_value |= ENC28J60_MACON3_FULDPX;                                                 // Настраиваем параметры MAC под выбранный Duplex
        mabbipg_value = ENC28J60_MABBIPG_FULL;                                                  // Задаём паузу между соседними кадрами
        maipg_value = ENC28J60_MAIPG_FULL;                                                      // Задаём межкадровый интервал
    }
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON1,                                           // Проверяем корректность MAC-адреса
                                 ENC28J60_MACON1_VALUE) != HAL_OK) return 0;                    // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON3,                                           // Проверяем корректность MAC-адреса
                                 macon3_value) != HAL_OK) return 0;                             // Настраиваем параметры MAC под выбранный Duplex
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MABBIPG,                                          // Проверяем состояние контроллера перед продолжением
                                 mabbipg_value) != HAL_OK) return 0;                            // Задаём паузу между соседними кадрами
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MAIPGL,                                       // Проверяем, удалось ли записать адрес в регистры
                                     maipg_value) != HAL_OK) return 0;                          // Задаём межкадровый интервал
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MAMXFLL,                                      // Проверяем, удалось ли записать адрес в регистры
                                     ENC28J60_MAMXFL_VALUE) != HAL_OK) return 0;                // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON1, &reg_value) != HAL_OK ||                   // Прекращаем работу при ошибке обмена по SPI
        reg_value != ENC28J60_MACON1_VALUE) return 0;                                           // Проверяем считанное значение регистра
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON3, &reg_value) != HAL_OK ||                   // Прекращаем работу при ошибке обмена по SPI
        reg_value != macon3_value) return 0;                                                    // Настраиваем параметры MAC под выбранный Duplex
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MABBIPG, &reg_value) != HAL_OK ||                  // Прекращаем работу при ошибке обмена по SPI
        reg_value != mabbipg_value) return 0;                                                   // Задаём паузу между соседними кадрами
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAIPGL, &check) != HAL_OK ||                   // Проверяем чтение двухбайтового регистра
        check != maipg_value) return 0;                                                         // Задаём межкадровый интервал
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAMXFLL, &check) != HAL_OK ||                  // Проверяем чтение двухбайтового регистра
        check != ENC28J60_MAMXFL_VALUE) return 0;
    return 1;                                                                                   // Сообщаем об успешном выполнении
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac)
{
    const uint8_t regs[ENC28J60_MAC_ADDRESS_SIZE] =                                             // Адреса регистров, в которые записываем MAC
    {
        ENC28J60_REG_MAADR1, ENC28J60_REG_MAADR2,
        ENC28J60_REG_MAADR3, ENC28J60_REG_MAADR4,
        ENC28J60_REG_MAADR5, ENC28J60_REG_MAADR6                                                // Выполняем эту часть команды
    };
    uint8_t i, value;                                                                           // Считанное значение регистра
    if (mac == 0) return 0;                                                                     // Проверяем, что передан действительный указатель
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)                                             // Последовательно обрабатываем элементы или попытки
    {
        if (ENC28J60_WriteControlReg(regs[i], mac[i]) != HAL_OK)                                // Прекращаем работу при ошибке обмена по SPI
            return 0;                                                                           // Сообщаем об ошибке
    }
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)                                             // Последовательно обрабатываем элементы или попытки
    {
        if (ENC28J60_ReadControlReg(regs[i], &value) != HAL_OK ||                               // Прекращаем работу при ошибке обмена по SPI
            value != mac[i]) return 0;
    }
    return 1;                                                                                   // Сообщаем об успешном выполнении
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_WaitMII(void)
{
    uint32_t startTime = HAL_GetTick();                                                         // Время начала ожидания для контроля тайм-аута
    uint8_t state;                                                                              // Состояние операции чтения или записи PHY
    do                                                                                          // Начинаем цикл ожидания завершения операции
    {
        if (ENC28J60_ReadControlReg(ENC28J60_REG_MISTAT, &state) != HAL_OK)                     // Прекращаем работу при ошибке обмена по SPI
            return HAL_ERROR;                                                                   // Возвращаем результат проверки или считанное значение
        if ((state & ENC28J60_MISTAT_BUSY) == 0)                                                // Ждём освобождения интерфейса PHY
            return HAL_OK;                                                                      // Возвращаем результат проверки или считанное значение
    }
    while ((uint32_t)(HAL_GetTick() - startTime) < ENC28J60_PHY_TIMEOUT_MS);                    // Повторяем проверку, пока условие выполняется
    return HAL_TIMEOUT;                                                                         // Возвращаем результат проверки или считанное значение
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value)
{
    HAL_StatusTypeDef status;                                                                   // Результат обмена с контроллером или состояние кадра
    if (value == 0) return 0;                                                                   // Проверяем, что передан действительный указатель
    *value = 0;                                                                                 // Сохраняем значение для последующей проверки
    status = ENC28J60_WriteControlReg(ENC28J60_REG_MIREGADR, reg);                              // Записываем настройку в регистр ENC28J60
    if (status != HAL_OK) return 0;                                                             // Прекращаем работу при ошибке обмена по SPI
    status = ENC28J60_WriteControlReg(ENC28J60_REG_MICMD,                                       // Записываем настройку в регистр ENC28J60
                                     ENC28J60_MICMD_MIIRD);                                     // Завершаем вызов функции
    if (status != HAL_OK) return 0;                                                             // Прекращаем работу при ошибке обмена по SPI
    status = ENC28J60_WaitMII();                                                                // Ждём завершения обмена с PHY
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MICMD, 0) != HAL_OK)                              // Прекращаем работу при ошибке обмена по SPI
        return 0;                                                                               // Сообщаем об ошибке
    if (status != HAL_OK) return 0;                                                             // Прекращаем работу при ошибке обмена по SPI
    return (ENC28J60_ReadControlRegPair(ENC28J60_REG_MIRDL, value) == HAL_OK);                  // Возвращаем результат проверки или считанное значение
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value)
{
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MIREGADR, reg) != HAL_OK)                         // Прекращаем работу при ошибке обмена по SPI
        return 0;                                                                               // Сообщаем об ошибке
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MIWRL, value) != HAL_OK)                      // Проверяем, удалось ли записать адрес в регистры
        return 0;                                                                               // Сообщаем об ошибке
    return (ENC28J60_WaitMII() == HAL_OK);                                                      // Возвращаем результат проверки или считанное значение
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_StartReceiving(void)
{
    uint8_t value;                                                                              // Считанное значение регистра
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                           // Проверяем сброс управляющих битов
                                 ENC28J60_ECON1_RXEN) != HAL_OK) return 0;                      // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON4,                                           // Проверяем корректность MAC-адреса
                                 ENC28J60_MACON4_VALUE) != HAL_OK) return 0;                    // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlReg(ENC28J60_REG_ERXFCON,                                          // Проверяем состояние контроллера перед продолжением
                                 ENC28J60_ERXFCON_VALUE) != HAL_OK) return 0;                   // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON4, &value) != HAL_OK ||                       // Прекращаем работу при ошибке обмена по SPI
        value != ENC28J60_MACON4_VALUE) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ERXFCON, &value) != HAL_OK ||                      // Прекращаем работу при ошибке обмена по SPI
        value != ENC28J60_ERXFCON_VALUE) return 0;
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,                                             // Проверяем установку управляющих битов
                               ENC28J60_ECON1_RXEN) != HAL_OK) return 0;                        // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &value) != HAL_OK)                          // Прекращаем работу при ошибке обмена по SPI
        return 0;                                                                               // Сообщаем об ошибке
    return ((value & ENC28J60_ECON1_RXEN) != 0);                                                // Возвращаем результат проверки или считанное значение
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,
                                   ENC28J60_DuplexMode duplex)
{
    uint8_t revision;                                                                           // Номер аппаратной ревизии контроллера
    uint16_t phy_value = 0;                                                                     // Значение регистра PHY для проверки настройки
    uint16_t phy_duplex;                                                                        // Значение настройки Duplex для PHY
    uint16_t phy_loopback;                                                                      // Настройка обратной петли PHY
    uint8_t i;                                                                                  // Номер текущего байта MAC-адреса
    uint8_t mac_nonzero = 0;                                                                    // Признак того, что MAC-адрес не нулевой
    if (mac == 0 ||                                                                             // Проверяем, что передан действительный указатель
        (duplex != ENC28J60_DUPLEX_HALF &&
         duplex != ENC28J60_DUPLEX_FULL))
    {
        return ENC28J60_INIT_ERR_PARAM;                                                         // Возвращаем код результата операции
    }
    if ((mac[0] & 0x01) != 0)                                                                   // Проверяем корректность MAC-адреса
    {
        return ENC28J60_INIT_ERR_PARAM;                                                         // Возвращаем код результата операции
    }
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)                                             // Последовательно обрабатываем элементы или попытки
    {
        if (mac[i] != 0)                                                                        // Проверяем корректность MAC-адреса
        {
            mac_nonzero = 1;
        }
    }
    if (mac_nonzero == 0)                                                                       // Проверяем, что передан действительный указатель
    {
        return ENC28J60_INIT_ERR_PARAM;                                                         // Возвращаем код результата операции
    }
    phy_duplex = (duplex == ENC28J60_DUPLEX_FULL)                                               // Подбираем режим PHY под выбранный Duplex
               ? ENC28J60_PHCON1_PDPXMD : 0;
    phy_loopback = (duplex == ENC28J60_DUPLEX_HALF)                                             // Отключаем обратную петлю в полудуплексном режиме
                 ? ENC28J60_PHCON2_VALUE : 0;
    if (ENC28J60_SoftReset() != 1)                                                              // Убеждаемся, что контроллер сбросился
    {
        return ENC28J60_INIT_ERR_RESET;                                                         // Возвращаем код результата операции
    }
    revision = ENC28J60_ReadRevision();
    if (revision == 0x00 || revision == 0xFF)                                                   // Отбрасываем недопустимый номер ревизии
    {
        return ENC28J60_INIT_ERR_REVISION;                                                      // Возвращаем код результата операции
    }
    if (ENC28J60_ConfigureBuffers() != 1)                                                       // Проверяем настройку буферной памяти
    {
        return ENC28J60_INIT_ERR_BUFFERS;                                                       // Возвращаем код результата операции
    }
    if (ENC28J60_ConfigureMAC(duplex) != 1)                                                     // Проверяем выбранный режим Duplex
    {
        return ENC28J60_INIT_ERR_MAC;                                                           // Возвращаем код результата операции
    }
    if (ENC28J60_ConfigureMACAddress(mac) != 1)                                                 // Проверяем корректность MAC-адреса
    {
        return ENC28J60_INIT_ERR_ADDRESS;                                                       // Возвращаем код результата операции
    }
    if (ENC28J60_WritePHY(                                                                      // Проверяем настройку регистра PHY
        ENC28J60_PHY_PHCON1,
        phy_duplex) != 1)
    {
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата операции
    }
    if (ENC28J60_ReadPHY(                                                                       // Проверяем чтение регистра PHY
        ENC28J60_PHY_PHCON1,
        &phy_value) != 1 ||
        (phy_value & ENC28J60_PHCON1_PDPXMD) != phy_duplex)
    {
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата операции
    }
    if (ENC28J60_WritePHY(                                                                      // Проверяем настройку регистра PHY
        ENC28J60_PHY_PHCON2,
        phy_loopback) != 1)
    {
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата операции
    }
    if (ENC28J60_ReadPHY(                                                                       // Проверяем чтение регистра PHY
        ENC28J60_PHY_PHCON2,
        &phy_value) != 1 ||
        (phy_value & ENC28J60_PHCON2_HDLDIS) != phy_loopback)
    {
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата операции
    }
    if (ENC28J60_StartReceiving() != 1)                                                         // Проверяем включение приёма кадров
    {
        return ENC28J60_INIT_ERR_RX;                                                            // Возвращаем код результата операции
    }
    return ENC28J60_INIT_OK;                                                                    // Возвращаем код результата операции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_GetPacketCount(void)
{
    uint8_t count = 0;                                                                          // Количество принятых пакетов
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &count) != HAL_OK)                        // Прекращаем работу при ошибке обмена по SPI
        return 0;                                                                               // Сообщаем об ошибке
    return count;                                                                               // Передаём количество ожидающих кадров
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t ENC28J60_PrepareTxFrame(uint8_t *data,
                                      uint16_t size)                                            // Количество байтов для обмена по SPI
{
    uint8_t controlByte = 0x00, econ1;                                                          // Служебный байт перед Ethernet-кадром
    if (data == 0 || size < ENC28J60_FRAME_MIN ||                                               // Проверяем, что передан действительный указатель
        size > ENC28J60_TX_FRAME_MAX) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &econ1) != HAL_OK)                          // Прекращаем работу при ошибке обмена по SPI
        return 0;                                                                               // Сообщаем об ошибке
    if (econ1 & ENC28J60_ECON1_TXRTS) return 0;                                                 // Проверяем, не занята ли передача кадра
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_EWRPTL,                                       // Проверяем, удалось ли записать адрес в регистры
                                     ENC28J60_TX_BUF_START) != HAL_OK) return 0;                // Присваиваем или вычисляем значение
    if (ENC28J60_WriteBufferMem(&controlByte, 1) != HAL_OK) return 0;                           // Прекращаем работу при ошибке обмена по SPI
    if (ENC28J60_WriteBufferMem(data, size) != HAL_OK) return 0;                                // Прекращаем работу при ошибке обмена по SPI
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ETXSTL,                                       // Проверяем, удалось ли записать адрес в регистры
                                     ENC28J60_TX_BUF_START) != HAL_OK) return 0;                // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ETXNDL,                                       // Проверяем, удалось ли записать адрес в регистры
                                     (uint16_t)(ENC28J60_TX_BUF_START + size))
        != HAL_OK) return 0;
    return 1;                                                                                   // Сообщаем об успешном выполнении
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t ENC28J60_ResetTxLogic(void)
{
    if (ENC28J60_BitFieldClearRaw(                                                              // Проверяем сброс управляющих битов
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRTS) != HAL_OK)                                                        // Присваиваем или вычисляем значение
    {
        return 0;                                                                               // Сообщаем об ошибке
    }
    if (ENC28J60_BitFieldSetRaw(                                                                // Проверяем установку управляющих битов
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRST) != HAL_OK)                                                        // Присваиваем или вычисляем значение
    {
        return 0;                                                                               // Сообщаем об ошибке
    }
    if (ENC28J60_BitFieldClearRaw(                                                              // Проверяем сброс управляющих битов
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRST) != HAL_OK)                                                        // Присваиваем или вычисляем значение
    {
        return 0;                                                                               // Сообщаем об ошибке
    }
    if (ENC28J60_BitFieldClearRaw(                                                              // Проверяем сброс управляющих битов
        ENC28J60_REG_EIR,                                                                       // Добавляем значение результата или режима
        ENC28J60_EIR_TXERIF | ENC28J60_EIR_TXIF) != HAL_OK)
    {
        return 0;                                                                               // Сообщаем об ошибке
    }
    return 1;                                                                                   // Сообщаем об успешном выполнении
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HAL_StatusTypeDef ENC28J60_ReadTxStatus(uint8_t status[7])
{
    uint16_t end;                                                                               // Промежуточные значения для выполнения операции
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ETXNDL, &end) != HAL_OK)                       // Проверяем чтение двухбайтового регистра
        return HAL_ERROR;                                                                       // Возвращаем результат проверки или считанное значение
    if (end > (ENC28J60_TX_BUF_END - 7)) return HAL_ERROR;                                      // Проверяем состояние контроллера перед продолжением
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,                                       // Проверяем, удалось ли записать адрес в регистры
                                     (uint16_t)(end + 1)) != HAL_OK)
        return HAL_ERROR;                                                                       // Возвращаем результат проверки или считанное значение
    return ENC28J60_ReadBufferMem(status, 7);                                                   // Возвращаем код результата операции
}
#define ENC28J60_TX_LATE_RETRIES 3u                                                             // Битовая маска или значение параметра TX_LATE_RETRIES
#define ENC28J60_TX_TSV_LATECOL 0x20u                                                           // Битовая маска или значение параметра TX_TSV_LATECOL

//////////////////////////////////////////////////////////////////////////////////////////////////

ENC28J60_TxResult ENC28J60_TransmitFrame(uint8_t *data,
                                         uint16_t size)                                         // Количество байтов для обмена по SPI
{
    uint16_t phy_status = 0;                                                                    // Результат обмена с контроллером или состояние кадра
    uint32_t startTime;                                                                         // Время начала ожидания для контроля тайм-аута
    uint8_t flags, estat, econ1, attempts;                                                      // Счётчик повторных попыток передачи
    uint8_t tsv[7];                                                                             // Семь байтов состояния последней передачи
    uint8_t late_collision;                                                                     // Признак поздней коллизии при передаче
    if (data == 0 || size < ENC28J60_FRAME_MIN ||                                               // Проверяем, что передан действительный указатель
        size > ENC28J60_TX_FRAME_MAX)
        return ENC28J60_TX_INVALID_FRAME;                                                       // Возвращаем код результата операции
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &econ1) != HAL_OK)                          // Прекращаем работу при ошибке обмена по SPI
        return ENC28J60_TX_ERROR;                                                               // Возвращаем код результата операции
    if (econ1 & ENC28J60_ECON1_TXRTS) return ENC28J60_TX_BUSY;                                  // Проверяем, не занята ли передача кадра
    if (ENC28J60_ReadPHY(ENC28J60_PHY_PHSTAT2, &phy_status) != 1)                               // Проверяем чтение регистра PHY
        return ENC28J60_TX_ERROR;                                                               // Возвращаем код результата операции
    if ((phy_status & ENC28J60_PHSTAT2_LSTAT) == 0)                                             // Проверяем наличие физического Ethernet-соединения
        return ENC28J60_TX_NO_LINK;                                                             // Возвращаем код результата операции
    if (ENC28J60_ResetTxLogic() != 1) return ENC28J60_TX_ERROR;                                 // Проверяем состояние контроллера перед продолжением
    if (ENC28J60_PrepareTxFrame(data, size) != 1)                                               // Проверяем допустимую длину Ethernet-кадра
        return ENC28J60_TX_ERROR;                                                               // Возвращаем код результата операции
    for (attempts = 0; attempts <= ENC28J60_TX_LATE_RETRIES; attempts++)                        // Последовательно обрабатываем элементы или попытки
    {
        if (attempts > 0 && ENC28J60_ResetTxLogic() != 1)                                       // Проверяем состояние контроллера перед продолжением
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата операции
        if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,                                         // Проверяем установку управляющих битов
                                   ENC28J60_ECON1_TXRTS) != HAL_OK)                             // Присваиваем или вычисляем значение
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата операции
        startTime = HAL_GetTick();                                                              // Контролируем время ожидания, чтобы не зависнуть
        for (;;)                                                                                // Последовательно обрабатываем элементы или попытки
        {
            if (ENC28J60_ReadControlReg(ENC28J60_REG_EIR, &flags) != HAL_OK)                    // Прекращаем работу при ошибке обмена по SPI
            {
                ENC28J60_ResetTxLogic();                                                        // Сбрасываем передатчик и очищаем флаги ошибок
                return ENC28J60_TX_ERROR;                                                       // Возвращаем код результата операции
            }
            if (flags & (ENC28J60_EIR_TXERIF | ENC28J60_EIR_TXIF))                              // Проверяем состояние контроллера перед продолжением
                break;                                                                          // Выходим из ожидания после завершения передачи
            if ((uint32_t)(HAL_GetTick() - startTime) >= ENC28J60_TX_TIMEOUT_MS)                // Прерываем ожидание по истечении времени
            {
                ENC28J60_ResetTxLogic();                                                        // Сбрасываем передатчик и очищаем флаги ошибок
                return ENC28J60_TX_TIMEOUT;                                                     // Возвращаем код результата операции
            }
        }
        if (ENC28J60_ReadControlReg(ENC28J60_REG_ESTAT, &estat) != HAL_OK)                      // Прекращаем работу при ошибке обмена по SPI
        {
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и очищаем флаги ошибок
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата операции
        }
        if ((flags & ENC28J60_EIR_TXERIF) == 0 &&                                               // Проверяем состояние контроллера перед продолжением
            (flags & ENC28J60_EIR_TXIF) != 0 &&
            (estat & ENC28J60_ESTAT_TXABRT) == 0)
            return ENC28J60_TX_OK;                                                              // Возвращаем код результата операции
        if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                       // Проверяем сброс управляющих битов
                                     ENC28J60_ECON1_TXRTS) != HAL_OK)                           // Присваиваем или вычисляем значение
        {
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и очищаем флаги ошибок
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата операции
        }
        late_collision = 0;                                                                     // Сохраняем признак поздней коллизии
        if (ENC28J60_ReadTxStatus(tsv) != HAL_OK)                                               // Прекращаем работу при ошибке обмена по SPI
        {
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и очищаем флаги ошибок
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата операции
        }
        if ((tsv[3] & ENC28J60_TX_TSV_LATECOL) != 0)                                            // Проверяем состояние контроллера перед продолжением
            late_collision = 1;                                                                 // Сохраняем признак поздней коллизии
        if (!late_collision || attempts == ENC28J60_TX_LATE_RETRIES)                            // Решаем, требуется ли повторная передача
        {
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и очищаем флаги ошибок
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата операции
        }
    }
    return ENC28J60_TX_ERROR;                                                                   // Возвращаем код результата операции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t ENC28J60_RecoverRx(void)
{
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                           // Проверяем сброс управляющих битов
                                 ENC28J60_ECON1_RXEN) != HAL_OK) return 0;                      // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,                                             // Проверяем установку управляющих битов
                               ENC28J60_ECON1_RXRST) != HAL_OK) return 0;                       // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                           // Проверяем сброс управляющих битов
                                 ENC28J60_ECON1_RXRST) != HAL_OK) return 0;                     // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_EIR,                                             // Проверяем сброс управляющих битов
                                 ENC28J60_EIR_RXERIF) != HAL_OK) return 0;                      // Присваиваем или вычисляем значение
    {
        uint8_t pending = 0;                                                                    // Промежуточные значения для выполнения операции
        if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &pending) != HAL_OK)                  // Прекращаем работу при ошибке обмена по SPI
            return 0;                                                                           // Сообщаем об ошибке
        while (pending-- != 0)                                                                  // Повторяем проверку, пока условие выполняется
        {
            if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,                                     // Проверяем установку управляющих битов
                                       ENC28J60_ECON2_PKTDEC) != HAL_OK)                        // Присваиваем или вычисляем значение
                return 0;                                                                       // Сообщаем об ошибке
        }
        if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &pending) != HAL_OK ||                // Прекращаем работу при ошибке обмена по SPI
            pending != 0)                                                                       // Освобождаем оставшиеся кадры из счётчика RX
            return 0;                                                                           // Сообщаем об ошибке
    }
    if (ENC28J60_ConfigureBuffers() != 1) return 0;                                             // Проверяем настройку буферной памяти
    return ENC28J60_StartReceiving();                                                           // Возвращаем код результата операции
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ENC28J60_RxResult ENC28J60_ReceiveFrame(uint8_t *data,
                                        uint16_t capacity,                                      // Промежуточные значения для выполнения операции
                                        uint16_t *receivedLen)                                  // Промежуточные значения для выполнения операции
{
    uint8_t header[ENC28J60_RX_HEADER_SIZE] = {0};                                              // Служебные байты принятого Ethernet-кадра
    uint8_t packetCount = 0;                                                                    // Число кадров, ожидающих чтения из RX-буфера
    uint16_t nextPacket, frameSize, rxStatus, dataSize, freePtr;                                // Адрес следующего кадра в кольцевом RX-буфере
    ENC28J60_RxResult result = ENC28J60_RX_BAD_FRAME;                                           // Промежуточные значения для выполнения операции
    if (data == 0 || receivedLen == 0 || capacity == 0)                                         // Проверяем, что передан действительный указатель
        return ENC28J60_RX_INVALID_ARG;                                                         // Возвращаем код результата операции
    *receivedLen = 0;                                                                           // Сохраняем длину принятого кадра без CRC
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &packetCount)                             // Проверяем наличие принятых пакетов
        != HAL_OK) return ENC28J60_RX_SPI_ERROR;
    if (packetCount == 0) return ENC28J60_RX_EMPTY;                                             // Проверяем наличие принятых пакетов
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,                                       // Проверяем, удалось ли записать адрес в регистры
                                     enc_next_packet) != HAL_OK)
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата операции
    if (ENC28J60_ReadBufferMem(header, ENC28J60_RX_HEADER_SIZE) != HAL_OK)                      // Прекращаем работу при ошибке обмена по SPI
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата операции
    nextPacket = (uint16_t)(header[0] | ((uint16_t)header[1] << 8));                            // Получаем адрес следующего кадра из заголовка RX
    frameSize = (uint16_t)(header[2] | ((uint16_t)header[3] << 8));                             // Получаем длину кадра из заголовка RX
    rxStatus = (uint16_t)(header[4] | ((uint16_t)header[5] << 8));                              // Получаем признаки ошибок и корректности кадра
    if (nextPacket > ENC28J60_RX_BUF_END ||                                                     // Проверяем адрес следующего кадра в RX-памяти
        (nextPacket & 1) != 0)
    {
        if (ENC28J60_RecoverRx() != 1)                                                          // Проверяем состояние контроллера перед продолжением
            return ENC28J60_RX_SPI_ERROR;                                                       // Возвращаем код результата операции
        return ENC28J60_RX_BAD_POINTER;                                                         // Возвращаем код результата операции
    }
    if ((rxStatus & ENC28J60_RX_STATUS_OK) != 0 &&                                              // Проверяем правильность принятого кадра
        frameSize >= (ENC28J60_FRAME_MIN + ENC28J60_CRC_SIZE) &&
        frameSize <= ENC28J60_RX_FRAME_MAX)
    {
        dataSize = frameSize - ENC28J60_CRC_SIZE;                                               // Убираем длину CRC из размера принятого кадра
        if (dataSize > capacity)                                                                // Проверяем, поместится ли кадр в выделенную память
            result = ENC28J60_RX_BUFFER_SMALL;                                                  // Сохраняем результат проверки принятого кадра
        else
        {
            if (ENC28J60_ReadBufferMem(data, dataSize) != HAL_OK)                               // Прекращаем работу при ошибке обмена по SPI
                return ENC28J60_RX_SPI_ERROR;                                                   // Возвращаем код результата операции
            *receivedLen = dataSize;                                                            // Сохраняем длину принятого кадра без CRC
            result = ENC28J60_RX_OK;                                                            // Сохраняем результат проверки принятого кадра
        }
    }
    freePtr = (nextPacket == ENC28J60_RX_BUF_START)                                             // Получаем адрес следующего кадра из заголовка RX
                ? ENC28J60_RX_BUF_END : (uint16_t)(nextPacket - 1);
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXRDPTL,                                     // Проверяем, удалось ли записать адрес в регистры
                                     freePtr) != HAL_OK)
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата операции
    enc_next_packet = nextPacket;                                                               // Сохраняем адрес следующего кадра для очередного чтения
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,                                             // Проверяем установку управляющих битов
                               ENC28J60_ECON2_PKTDEC) != HAL_OK)                                // Присваиваем или вычисляем значение
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата операции
    return result;                                                                              // Передаём результат обработки кадра
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t ENC28J60_GetPHYStatus(void)
{
    uint16_t status = 0;                                                                        // Результат обмена с контроллером или состояние кадра
    if (ENC28J60_ReadPHY(                                                                       // Проверяем чтение регистра PHY
        ENC28J60_PHY_PHSTAT2,
        &status) != 1)
    {
        return 0;                                                                               // Сообщаем об ошибке
    }
    return status;                                                                              // Передаём результат SPI-операции вызывающей функции
}
