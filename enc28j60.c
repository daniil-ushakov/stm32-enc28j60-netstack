// Библиотека для работы с модулем ENC28J60
// Автор: Ушаков Д.Д.
// Контроллер STM32F103, библиотека HAL
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "enc28j60.h"                                                                           // Подключаем нужный заголовочный файл
#include "stm32f1xx_hal.h"                                                                      // Подключаем нужный заголовочный файл
extern SPI_HandleTypeDef hspi1;                                                                 // Используем SPI1, объявленный в другом файле
static uint16_t enc_next_packet = ENC28J60_RX_BUF_START;                                        // Запоминаем адрес следующего принятого пакета
static uint8_t enc_current_bank = 0xFF;                                                         // Запоминаем выбранный банк регистров
///////////////////////////////////////////////////////////////////////////////////////////////////
static void ENC28J60_SetCS(GPIO_PinState state)                                                 // Объявляем внутреннюю функцию
{                                                                                               // Начало блока команд
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, state);                                                // Переключаем вывод выбора микросхемы
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_BitFieldSetRaw(uint8_t regAddr,                               // Объявляем внутреннюю функцию
                                                 uint8_t mask)                                  // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFS | (regAddr & 0x1F)), mask};                     // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем или отпускаем ENC28J60
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);                          // Передаём данные по SPI
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Выбираем или отпускаем ENC28J60
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_BitFieldClearRaw(uint8_t regAddr,                             // Объявляем внутреннюю функцию
                                                   uint8_t mask)                                // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFC | (regAddr & 0x1F)), mask};                     // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем или отпускаем ENC28J60
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);                          // Передаём данные по SPI
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Выбираем или отпускаем ENC28J60
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_SelectBank(uint8_t bank)                                      // Объявляем внутреннюю функцию
{                                                                                               // Начало блока команд
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    bank &= 0x03;                                                                               // Присваиваем или вычисляем значение
    if (enc_current_bank == bank) return HAL_OK;                                                // Проверяем условие перед следующим действием
    status = ENC28J60_BitFieldClearRaw(                                                         // Сбрасываем нужные биты регистра
        ENC28J60_REG_ECON1,                                                                     // Передаём следующий параметр
        ENC28J60_ECON1_BSEL0 | ENC28J60_ECON1_BSEL1);                                           // Создаём переменную для работы функции
    if (status != HAL_OK)                                                                       // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        enc_current_bank = 0xFF;                                                                // Присваиваем или вычисляем значение
        return status;                                                                          // Возвращаем полученное значение
    }                                                                                           // Конец блока команд
    status = ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1, bank);                                 // Устанавливаем нужные биты регистра
    enc_current_bank = (status == HAL_OK) ? bank : 0xFF;                                        // Присваиваем или вычисляем значение
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_ReadControlReg(uint8_t reg,                                   // Объявляем внутреннюю функцию
                                                uint8_t *value)                                 // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t addr = reg & 0x1F;                                                                  // Создаём переменную для работы функции
    uint8_t bank = (reg >> 5) & 0x03;                                                           // Создаём переменную для работы функции
    uint8_t isMacMii = (reg & 0x80) != 0;                                                       // Создаём переменную для работы функции
    uint8_t tx[3] = {addr, 0x00, 0x00};                                                         // Создаём переменную для работы функции
    uint8_t rx[3] = {0};                                                                        // Создаём переменную для работы функции
    uint16_t size = isMacMii ? 3 : 2;                                                           // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    if (value == 0) return HAL_ERROR;                                                           // Проверяем условие перед следующим действием
    *value = 0;                                                                                 // Присваиваем или вычисляем значение
    if (addr < 0x1B)                                                                            // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        status = ENC28J60_SelectBank(bank);                                                     // Вызываем функцию работы с контроллером
        if (status != HAL_OK) return status;                                                    // Проверяем условие перед следующим действием
    }                                                                                           // Конец блока команд
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем или отпускаем ENC28J60
    status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, size,                                      // Передаём и принимаем данные по SPI
                                     ENC28J60_SPI_TIMEOUT_MS);                                  // Завершаем вызов функции
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Выбираем или отпускаем ENC28J60
    if (status == HAL_OK) *value = rx[size - 1];                                                // Проверяем условие перед следующим действием
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_WriteControlReg(uint8_t reg,                                  // Объявляем внутреннюю функцию
                                                  uint8_t value)                                // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t addr = reg & 0x1F;                                                                  // Создаём переменную для работы функции
    uint8_t bank = (reg >> 5) & 0x03;                                                           // Создаём переменную для работы функции
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_WCR | addr), value};                                // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    if (addr < 0x1B)                                                                            // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        status = ENC28J60_SelectBank(bank);                                                     // Вызываем функцию работы с контроллером
        if (status != HAL_OK)                                                                   // Проверяем условие перед следующим действием
        {                                                                                       // Начало блока команд
            return status;                                                                      // Возвращаем полученное значение
        }                                                                                       // Конец блока команд
    }                                                                                           // Конец блока команд
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем или отпускаем ENC28J60
    status = HAL_SPI_Transmit(&hspi1, tx, 2,                                                    // Передаём данные по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Выбираем или отпускаем ENC28J60
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_WriteControlRegPair(uint8_t reg,                              // Объявляем внутреннюю функцию
                                                      uint16_t value)                           // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    status = ENC28J60_WriteControlReg(reg, (uint8_t)value);                                     // Записываем регистр контроллера
    if (status != HAL_OK)                                                                       // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return status;                                                                          // Возвращаем полученное значение
    }                                                                                           // Конец блока команд
    status = ENC28J60_WriteControlReg((uint8_t)(reg + 1),                                       // Записываем регистр контроллера
                                     (uint8_t)(value >> 8));                                    // Продолжаем проверку условия или вызов функции
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_ReadControlRegPair(uint8_t reg,                               // Объявляем внутреннюю функцию
                                                    uint16_t *value)                            // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t low, high;                                                                          // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    if (value == 0) return HAL_ERROR;                                                           // Проверяем условие перед следующим действием
    *value = 0;                                                                                 // Присваиваем или вычисляем значение
    status = ENC28J60_ReadControlReg(reg, &low);                                                // Читаем регистр контроллера
    if (status != HAL_OK) return status;                                                        // Проверяем условие перед следующим действием
    status = ENC28J60_ReadControlReg((uint8_t)(reg + 1), &high);                                // Читаем регистр контроллера
    if (status != HAL_OK) return status;                                                        // Проверяем условие перед следующим действием
    *value = (uint16_t)((uint16_t)low | ((uint16_t)high << 8));                                 // Завершаем вызов функции
    return HAL_OK;                                                                              // Возвращаем результат операции
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_WriteBufferMem(uint8_t *data,                                 // Объявляем внутреннюю функцию
                                                 uint16_t size)                                 // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t command = ENC28J60_CMD_WBM;                                                         // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    if (data == 0 || size == 0)                                                                 // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return HAL_ERROR;                                                                       // Возвращаем полученное значение
    }                                                                                           // Конец блока команд
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем или отпускаем ENC28J60
    status = HAL_SPI_Transmit(&hspi1, &command, 1,                                              // Передаём данные по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    if (status == HAL_OK)                                                                       // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        status = HAL_SPI_Transmit(&hspi1, data, size,                                           // Передаём данные по SPI
                                  ENC28J60_SPI_TIMEOUT_MS);                                     // Завершаем вызов функции
    }                                                                                           // Конец блока команд
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Выбираем или отпускаем ENC28J60
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_ReadBufferMem(uint8_t *data,                                  // Объявляем внутреннюю функцию
                                                uint16_t size)                                  // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t command = ENC28J60_CMD_RBM;                                                         // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    if (data == 0 || size == 0)                                                                 // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return HAL_ERROR;                                                                       // Возвращаем полученное значение
    }                                                                                           // Конец блока команд
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем или отпускаем ENC28J60
    status = HAL_SPI_Transmit(&hspi1, &command, 1,                                              // Передаём данные по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    if (status == HAL_OK)                                                                       // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        status = HAL_SPI_Receive(&hspi1, data, size,                                            // Принимаем данные по SPI
                                 ENC28J60_SPI_TIMEOUT_MS);                                      // Завершаем вызов функции
    }                                                                                           // Конец блока команд
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Выбираем или отпускаем ENC28J60
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_SoftReset(void)                                                                // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    uint8_t command = ENC28J60_CMD_SRC;                                                         // Создаём переменную для работы функции
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    ENC28J60_SetCS(GPIO_PIN_RESET);                                                             // Выбираем или отпускаем ENC28J60
    status = HAL_SPI_Transmit(&hspi1, &command, 1,                                              // Передаём данные по SPI
                              ENC28J60_SPI_TIMEOUT_MS);                                         // Завершаем вызов функции
    ENC28J60_SetCS(GPIO_PIN_SET);                                                               // Выбираем или отпускаем ENC28J60
    if (status != HAL_OK)                                                                       // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    enc_current_bank = 0xFF;                                                                    // Присваиваем или вычисляем значение
    HAL_Delay(ENC28J60_RESET_DELAY_MS);                                                         // Ожидаем указанное время
    return 1;                                                                                   // Сообщаем об успешном выполнении
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_ReadRevision(void)                                                             // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    uint8_t value = 0;                                                                          // Создаём переменную для работы функции
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EREVID, &value) != HAL_OK)                         // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    return value;                                                                               // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_ConfigureBuffers(void)                                                         // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    uint16_t check;                                                                             // Создаём переменную для работы функции
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXSTL,                                       // Проверяем условие перед следующим действием
                                    ENC28J60_RX_BUF_START) != HAL_OK) return 0;                 // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXNDL,                                       // Проверяем условие перед следующим действием
                                    ENC28J60_RX_BUF_END) != HAL_OK) return 0;                   // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXRDPTL,                                     // Проверяем условие перед следующим действием
                                    ENC28J60_RX_BUF_END) != HAL_OK) return 0;                   // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,                                       // Проверяем условие перед следующим действием
                                    ENC28J60_RX_BUF_START) != HAL_OK) return 0;                 // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,                                             // Проверяем условие перед следующим действием
                               ENC28J60_ECON2_AUTOINC) != HAL_OK) return 0;                     // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXSTL, &check) != HAL_OK ||                   // Проверяем условие перед следующим действием
        check != ENC28J60_RX_BUF_START) return 0;                                               // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXNDL, &check) != HAL_OK ||                   // Проверяем условие перед следующим действием
        check != ENC28J60_RX_BUF_END) return 0;                                                 // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXRDPTL, &check) != HAL_OK ||                 // Проверяем условие перед следующим действием
        check != ENC28J60_RX_BUF_END) return 0;                                                 // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERDPTL, &check) != HAL_OK ||                   // Проверяем условие перед следующим действием
        check != ENC28J60_RX_BUF_START) return 0;                                               // Присваиваем или вычисляем значение
    enc_next_packet = ENC28J60_RX_BUF_START;                                                    // Присваиваем или вычисляем значение
    return 1;                                                                                   // Сообщаем об успешном выполнении
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex)                                       // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    uint8_t reg_value;                                                                          // Создаём переменную для работы функции
    uint16_t check;                                                                             // Создаём переменную для работы функции
    uint8_t macon3_value = ENC28J60_MACON3_VALUE;                                               // Создаём переменную для работы функции
    uint8_t mabbipg_value = ENC28J60_MABBIPG_VALUE;                                             // Создаём переменную для работы функции
    uint16_t maipg_value = ENC28J60_MAIPG_VALUE;                                                // Создаём переменную для работы функции
    if (duplex != ENC28J60_DUPLEX_HALF &&                                                       // Проверяем условие перед следующим действием
        duplex != ENC28J60_DUPLEX_FULL) return 0;                                               // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &reg_value) != HAL_OK)                      // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    if (reg_value & (ENC28J60_ECON1_RXEN | ENC28J60_ECON1_TXRTS))                               // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    if (duplex == ENC28J60_DUPLEX_FULL)                                                         // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        macon3_value |= ENC28J60_MACON3_FULDPX;                                                 // Присваиваем или вычисляем значение
        mabbipg_value = ENC28J60_MABBIPG_FULL;                                                  // Присваиваем или вычисляем значение
        maipg_value = ENC28J60_MAIPG_FULL;                                                      // Присваиваем или вычисляем значение
    }                                                                                           // Конец блока команд
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON1,                                           // Проверяем условие перед следующим действием
                                 ENC28J60_MACON1_VALUE) != HAL_OK) return 0;                    // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON3,                                           // Проверяем условие перед следующим действием
                                 macon3_value) != HAL_OK) return 0;                             // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MABBIPG,                                          // Проверяем условие перед следующим действием
                                 mabbipg_value) != HAL_OK) return 0;                            // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MAIPGL,                                       // Проверяем условие перед следующим действием
                                     maipg_value) != HAL_OK) return 0;                          // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MAMXFLL,                                      // Проверяем условие перед следующим действием
                                     ENC28J60_MAMXFL_VALUE) != HAL_OK) return 0;                // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON1, &reg_value) != HAL_OK ||                   // Проверяем условие перед следующим действием
        reg_value != ENC28J60_MACON1_VALUE) return 0;                                           // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON3, &reg_value) != HAL_OK ||                   // Проверяем условие перед следующим действием
        reg_value != macon3_value) return 0;                                                    // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MABBIPG, &reg_value) != HAL_OK ||                  // Проверяем условие перед следующим действием
        reg_value != mabbipg_value) return 0;                                                   // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAIPGL, &check) != HAL_OK ||                   // Проверяем условие перед следующим действием
        check != maipg_value) return 0;                                                         // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAMXFLL, &check) != HAL_OK ||                  // Проверяем условие перед следующим действием
        check != ENC28J60_MAMXFL_VALUE) return 0;                                               // Присваиваем или вычисляем значение
    return 1;                                                                                   // Сообщаем об успешном выполнении
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac)                                        // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    const uint8_t regs[ENC28J60_MAC_ADDRESS_SIZE] =                                             // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        ENC28J60_REG_MAADR1, ENC28J60_REG_MAADR2,                                               // Передаём следующий параметр
        ENC28J60_REG_MAADR3, ENC28J60_REG_MAADR4,                                               // Передаём следующий параметр
        ENC28J60_REG_MAADR5, ENC28J60_REG_MAADR6                                                // Выполняем эту часть команды
    };                                                                                          // Конец перечисления
    uint8_t i, value;                                                                           // Создаём переменную для работы функции
    if (mac == 0) return 0;                                                                     // Проверяем условие перед следующим действием
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)                                             // Повторяем действия в цикле
    {                                                                                           // Начало блока команд
        if (ENC28J60_WriteControlReg(regs[i], mac[i]) != HAL_OK)                                // Проверяем условие перед следующим действием
            return 0;                                                                           // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)                                             // Повторяем действия в цикле
    {                                                                                           // Начало блока команд
        if (ENC28J60_ReadControlReg(regs[i], &value) != HAL_OK ||                               // Проверяем условие перед следующим действием
            value != mac[i]) return 0;                                                          // Присваиваем или вычисляем значение
    }                                                                                           // Конец блока команд
    return 1;                                                                                   // Сообщаем об успешном выполнении
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_WaitMII(void)                                                 // Объявляем внутреннюю функцию
{                                                                                               // Начало блока команд
    uint32_t startTime = HAL_GetTick();                                                         // Создаём переменную для работы функции
    uint8_t state;                                                                              // Создаём переменную для работы функции
    do                                                                                          // Начинаем цикл с проверкой в конце
    {                                                                                           // Начало блока команд
        if (ENC28J60_ReadControlReg(ENC28J60_REG_MISTAT, &state) != HAL_OK)                     // Проверяем условие перед следующим действием
            return HAL_ERROR;                                                                   // Возвращаем полученное значение
        if ((state & ENC28J60_MISTAT_BUSY) == 0)                                                // Проверяем условие перед следующим действием
            return HAL_OK;                                                                      // Возвращаем результат операции
    }                                                                                           // Конец блока команд
    while ((uint32_t)(HAL_GetTick() - startTime) < ENC28J60_PHY_TIMEOUT_MS);                    // Продолжаем цикл, пока условие верно
    return HAL_TIMEOUT;                                                                         // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value)                                          // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    HAL_StatusTypeDef status;                                                                   // Создаём переменную для работы функции
    if (value == 0) return 0;                                                                   // Проверяем условие перед следующим действием
    *value = 0;                                                                                 // Присваиваем или вычисляем значение
    status = ENC28J60_WriteControlReg(ENC28J60_REG_MIREGADR, reg);                              // Записываем регистр контроллера
    if (status != HAL_OK) return 0;                                                             // Проверяем условие перед следующим действием
    status = ENC28J60_WriteControlReg(ENC28J60_REG_MICMD,                                       // Записываем регистр контроллера
                                     ENC28J60_MICMD_MIIRD);                                     // Завершаем вызов функции
    if (status != HAL_OK) return 0;                                                             // Проверяем условие перед следующим действием
    status = ENC28J60_WaitMII();                                                                // Вызываем функцию работы с контроллером
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MICMD, 0) != HAL_OK)                              // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    if (status != HAL_OK) return 0;                                                             // Проверяем условие перед следующим действием
    return (ENC28J60_ReadControlRegPair(ENC28J60_REG_MIRDL, value) == HAL_OK);                  // Возвращаем результат операции
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value)                                          // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MIREGADR, reg) != HAL_OK)                         // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MIWRL, value) != HAL_OK)                      // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    return (ENC28J60_WaitMII() == HAL_OK);                                                      // Возвращаем результат операции
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_StartReceiving(void)                                                           // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    uint8_t value;                                                                              // Создаём переменную для работы функции
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                           // Проверяем условие перед следующим действием
                                 ENC28J60_ECON1_RXEN) != HAL_OK) return 0;                      // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON4,                                           // Проверяем условие перед следующим действием
                                 ENC28J60_MACON4_VALUE) != HAL_OK) return 0;                    // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlReg(ENC28J60_REG_ERXFCON,                                          // Проверяем условие перед следующим действием
                                 ENC28J60_ERXFCON_VALUE) != HAL_OK) return 0;                   // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON4, &value) != HAL_OK ||                       // Проверяем условие перед следующим действием
        value != ENC28J60_MACON4_VALUE) return 0;                                               // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ERXFCON, &value) != HAL_OK ||                      // Проверяем условие перед следующим действием
        value != ENC28J60_ERXFCON_VALUE) return 0;                                              // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,                                             // Проверяем условие перед следующим действием
                               ENC28J60_ECON1_RXEN) != HAL_OK) return 0;                        // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &value) != HAL_OK)                          // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    return ((value & ENC28J60_ECON1_RXEN) != 0);                                                // Возвращаем код результата
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,                                           // Объявляем функцию для работы с ENC28J60
                                   ENC28J60_DuplexMode duplex)                                  // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t revision;                                                                           // Создаём переменную для работы функции
    uint16_t phy_value = 0;                                                                     // Создаём переменную для работы функции
    uint16_t phy_duplex;                                                                        // Создаём переменную для работы функции
    uint16_t phy_loopback;                                                                      // Создаём переменную для работы функции
    uint8_t i;                                                                                  // Создаём переменную для работы функции
    uint8_t mac_nonzero = 0;                                                                    // Создаём переменную для работы функции
    if (mac == 0 ||                                                                             // Проверяем условие перед следующим действием
        (duplex != ENC28J60_DUPLEX_HALF &&                                                      // Вызываем функцию работы с контроллером
         duplex != ENC28J60_DUPLEX_FULL))                                                       // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_PARAM;                                                         // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if ((mac[0] & 0x01) != 0)                                                                   // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_PARAM;                                                         // Возвращаем код результата
    }                                                                                           // Конец блока команд
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)                                             // Повторяем действия в цикле
    {                                                                                           // Начало блока команд
        if (mac[i] != 0)                                                                        // Проверяем условие перед следующим действием
        {                                                                                       // Начало блока команд
            mac_nonzero = 1;                                                                    // Присваиваем или вычисляем значение
        }                                                                                       // Конец блока команд
    }                                                                                           // Конец блока команд
    if (mac_nonzero == 0)                                                                       // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_PARAM;                                                         // Возвращаем код результата
    }                                                                                           // Конец блока команд
    phy_duplex = (duplex == ENC28J60_DUPLEX_FULL)                                               // Вызываем функцию работы с контроллером
               ? ENC28J60_PHCON1_PDPXMD : 0;                                                    // Выбираем значение по условию
    phy_loopback = (duplex == ENC28J60_DUPLEX_HALF)                                             // Вызываем функцию работы с контроллером
                 ? ENC28J60_PHCON2_VALUE : 0;                                                   // Выбираем значение по условию
    if (ENC28J60_SoftReset() != 1)                                                              // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_RESET;                                                         // Возвращаем код результата
    }                                                                                           // Конец блока команд
    revision = ENC28J60_ReadRevision();                                                         // Вызываем функцию работы с контроллером
    if (revision == 0x00 || revision == 0xFF)                                                   // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_REVISION;                                                      // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_ConfigureBuffers() != 1)                                                       // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_BUFFERS;                                                       // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_ConfigureMAC(duplex) != 1)                                                     // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_MAC;                                                           // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_ConfigureMACAddress(mac) != 1)                                                 // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_ADDRESS;                                                       // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_WritePHY(                                                                      // Проверяем условие перед следующим действием
        ENC28J60_PHY_PHCON1,                                                                    // Передаём следующий параметр
        phy_duplex) != 1)                                                                       // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_ReadPHY(                                                                       // Проверяем условие перед следующим действием
        ENC28J60_PHY_PHCON1,                                                                    // Передаём следующий параметр
        &phy_value) != 1 ||                                                                     // Передаём адрес переменной
        (phy_value & ENC28J60_PHCON1_PDPXMD) != phy_duplex)                                     // Вызываем функцию работы с контроллером
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_WritePHY(                                                                      // Проверяем условие перед следующим действием
        ENC28J60_PHY_PHCON2,                                                                    // Передаём следующий параметр
        phy_loopback) != 1)                                                                     // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_ReadPHY(                                                                       // Проверяем условие перед следующим действием
        ENC28J60_PHY_PHCON2,                                                                    // Передаём следующий параметр
        &phy_value) != 1 ||                                                                     // Передаём адрес переменной
        (phy_value & ENC28J60_PHCON2_HDLDIS) != phy_loopback)                                   // Вызываем функцию работы с контроллером
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_PHY;                                                           // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if (ENC28J60_StartReceiving() != 1)                                                         // Проверяем условие перед следующим действием
    {                                                                                           // Начало блока команд
        return ENC28J60_INIT_ERR_RX;                                                            // Возвращаем код результата
    }                                                                                           // Конец блока команд
    return ENC28J60_INIT_OK;                                                                    // Возвращаем код результата
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_GetPacketCount(void)                                                           // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    uint8_t count = 0;                                                                          // Создаём переменную для работы функции
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &count) != HAL_OK)                        // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    return count;                                                                               // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t ENC28J60_PrepareTxFrame(uint8_t *data,                                           // Объявляем внутреннюю функцию
                                      uint16_t size)                                            // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t controlByte = 0x00, econ1;                                                          // Создаём переменную для работы функции
    if (data == 0 || size < ENC28J60_FRAME_MIN ||                                               // Проверяем условие перед следующим действием
        size > ENC28J60_TX_FRAME_MAX) return 0;                                                 // Выполняем эту часть команды
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &econ1) != HAL_OK)                          // Проверяем условие перед следующим действием
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    if (econ1 & ENC28J60_ECON1_TXRTS) return 0;                                                 // Проверяем условие перед следующим действием
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_EWRPTL,                                       // Проверяем условие перед следующим действием
                                     ENC28J60_TX_BUF_START) != HAL_OK) return 0;                // Присваиваем или вычисляем значение
    if (ENC28J60_WriteBufferMem(&controlByte, 1) != HAL_OK) return 0;                           // Проверяем условие перед следующим действием
    if (ENC28J60_WriteBufferMem(data, size) != HAL_OK) return 0;                                // Проверяем условие перед следующим действием
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ETXSTL,                                       // Проверяем условие перед следующим действием
                                     ENC28J60_TX_BUF_START) != HAL_OK) return 0;                // Присваиваем или вычисляем значение
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ETXNDL,                                       // Проверяем условие перед следующим действием
                                     (uint16_t)(ENC28J60_TX_BUF_START + size))                  // Вызываем функцию работы с контроллером
        != HAL_OK) return 0;                                                                    // Проверяем результат операции
    return 1;                                                                                   // Сообщаем об успешном выполнении
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t ENC28J60_ResetTxLogic(void)                                                      // Объявляем внутреннюю функцию
{                                                                                               // Начало блока команд
    if (ENC28J60_BitFieldClearRaw(                                                              // Проверяем условие перед следующим действием
        ENC28J60_REG_ECON1,                                                                     // Передаём следующий параметр
        ENC28J60_ECON1_TXRTS) != HAL_OK)                                                        // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    if (ENC28J60_BitFieldSetRaw(                                                                // Проверяем условие перед следующим действием
        ENC28J60_REG_ECON1,                                                                     // Передаём следующий параметр
        ENC28J60_ECON1_TXRST) != HAL_OK)                                                        // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    if (ENC28J60_BitFieldClearRaw(                                                              // Проверяем условие перед следующим действием
        ENC28J60_REG_ECON1,                                                                     // Передаём следующий параметр
        ENC28J60_ECON1_TXRST) != HAL_OK)                                                        // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    if (ENC28J60_BitFieldClearRaw(                                                              // Проверяем условие перед следующим действием
        ENC28J60_REG_EIR,                                                                       // Добавляем значение результата или режима
        ENC28J60_EIR_TXERIF | ENC28J60_EIR_TXIF) != HAL_OK)                                     // Создаём переменную для работы функции
    {                                                                                           // Начало блока команд
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    return 1;                                                                                   // Сообщаем об успешном выполнении
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static HAL_StatusTypeDef ENC28J60_ReadTxStatus(uint8_t status[7])                               // Объявляем внутреннюю функцию
{                                                                                               // Начало блока команд
    uint16_t end;                                                                               // Создаём переменную для работы функции
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ETXNDL, &end) != HAL_OK)                       // Проверяем условие перед следующим действием
        return HAL_ERROR;                                                                       // Возвращаем полученное значение
    if (end > (ENC28J60_TX_BUF_END - 7)) return HAL_ERROR;                                      // Проверяем условие перед следующим действием
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,                                       // Проверяем условие перед следующим действием
                                     (uint16_t)(end + 1)) != HAL_OK)                            // Продолжаем проверку условия или вызов функции
        return HAL_ERROR;                                                                       // Возвращаем полученное значение
    return ENC28J60_ReadBufferMem(status, 7);                                                   // Возвращаем код результата
}                                                                                               // Конец блока команд
#define ENC28J60_TX_LATE_RETRIES 3u                                                             // Задаём значение tx late retries
#define ENC28J60_TX_TSV_LATECOL 0x20u                                                           // Задаём значение tx tsv latecol
///////////////////////////////////////////////////////////////////////////////////////////////////
ENC28J60_TxResult ENC28J60_TransmitFrame(uint8_t *data,                                         // Объявляем функцию для работы с ENC28J60
                                         uint16_t size)                                         // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint16_t phy_status = 0;                                                                    // Создаём переменную для работы функции
    uint32_t startTime;                                                                         // Создаём переменную для работы функции
    uint8_t flags, estat, econ1, attempts;                                                      // Создаём переменную для работы функции
    uint8_t tsv[7];                                                                             // Создаём переменную для работы функции
    uint8_t late_collision;                                                                     // Создаём переменную для работы функции
    if (data == 0 || size < ENC28J60_FRAME_MIN ||                                               // Проверяем условие перед следующим действием
        size > ENC28J60_TX_FRAME_MAX)                                                           // Выполняем эту часть команды
        return ENC28J60_TX_INVALID_FRAME;                                                       // Возвращаем код результата
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &econ1) != HAL_OK)                          // Проверяем условие перед следующим действием
        return ENC28J60_TX_ERROR;                                                               // Возвращаем код результата
    if (econ1 & ENC28J60_ECON1_TXRTS) return ENC28J60_TX_BUSY;                                  // Проверяем условие перед следующим действием
    if (ENC28J60_ReadPHY(ENC28J60_PHY_PHSTAT2, &phy_status) != 1)                               // Проверяем условие перед следующим действием
        return ENC28J60_TX_ERROR;                                                               // Возвращаем код результата
    if ((phy_status & ENC28J60_PHSTAT2_LSTAT) == 0)                                             // Проверяем условие перед следующим действием
        return ENC28J60_TX_NO_LINK;                                                             // Возвращаем код результата
    if (ENC28J60_ResetTxLogic() != 1) return ENC28J60_TX_ERROR;                                 // Проверяем условие перед следующим действием
    if (ENC28J60_PrepareTxFrame(data, size) != 1)                                               // Проверяем условие перед следующим действием
        return ENC28J60_TX_ERROR;                                                               // Возвращаем код результата
    for (attempts = 0; attempts <= ENC28J60_TX_LATE_RETRIES; attempts++)                        // Повторяем действия в цикле
    {                                                                                           // Начало блока команд
        if (attempts > 0 && ENC28J60_ResetTxLogic() != 1)                                       // Проверяем условие перед следующим действием
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата
        if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,                                         // Проверяем условие перед следующим действием
                                   ENC28J60_ECON1_TXRTS) != HAL_OK)                             // Присваиваем или вычисляем значение
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата
        startTime = HAL_GetTick();                                                              // Получаем время работы микроконтроллера
        for (;;)                                                                                // Повторяем действия в цикле
        {                                                                                       // Начало блока команд
            if (ENC28J60_ReadControlReg(ENC28J60_REG_EIR, &flags) != HAL_OK)                    // Проверяем условие перед следующим действием
            {                                                                                   // Начало блока команд
                ENC28J60_ResetTxLogic();                                                        // Сбрасываем передатчик и его ошибки
                return ENC28J60_TX_ERROR;                                                       // Возвращаем код результата
            }                                                                                   // Конец блока команд
            if (flags & (ENC28J60_EIR_TXERIF | ENC28J60_EIR_TXIF))                              // Проверяем условие перед следующим действием
                break;                                                                          // Прерываем цикл
            if ((uint32_t)(HAL_GetTick() - startTime) >= ENC28J60_TX_TIMEOUT_MS)                // Проверяем условие перед следующим действием
            {                                                                                   // Начало блока команд
                ENC28J60_ResetTxLogic();                                                        // Сбрасываем передатчик и его ошибки
                return ENC28J60_TX_TIMEOUT;                                                     // Возвращаем код результата
            }                                                                                   // Конец блока команд
        }                                                                                       // Конец блока команд
        if (ENC28J60_ReadControlReg(ENC28J60_REG_ESTAT, &estat) != HAL_OK)                      // Проверяем условие перед следующим действием
        {                                                                                       // Начало блока команд
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и его ошибки
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата
        }                                                                                       // Конец блока команд
        if ((flags & ENC28J60_EIR_TXERIF) == 0 &&                                               // Проверяем условие перед следующим действием
            (flags & ENC28J60_EIR_TXIF) != 0 &&                                                 // Вызываем функцию работы с контроллером
            (estat & ENC28J60_ESTAT_TXABRT) == 0)                                               // Вызываем функцию работы с контроллером
            return ENC28J60_TX_OK;                                                              // Возвращаем код результата
        if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                       // Проверяем условие перед следующим действием
                                     ENC28J60_ECON1_TXRTS) != HAL_OK)                           // Присваиваем или вычисляем значение
        {                                                                                       // Начало блока команд
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и его ошибки
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата
        }                                                                                       // Конец блока команд
        late_collision = 0;                                                                     // Присваиваем или вычисляем значение
        if (ENC28J60_ReadTxStatus(tsv) != HAL_OK)                                               // Проверяем условие перед следующим действием
        {                                                                                       // Начало блока команд
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и его ошибки
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата
        }                                                                                       // Конец блока команд
        if ((tsv[3] & ENC28J60_TX_TSV_LATECOL) != 0)                                            // Проверяем условие перед следующим действием
            late_collision = 1;                                                                 // Присваиваем или вычисляем значение
        if (!late_collision || attempts == ENC28J60_TX_LATE_RETRIES)                            // Проверяем условие перед следующим действием
        {                                                                                       // Начало блока команд
            ENC28J60_ResetTxLogic();                                                            // Сбрасываем передатчик и его ошибки
            return ENC28J60_TX_ERROR;                                                           // Возвращаем код результата
        }                                                                                       // Конец блока команд
    }                                                                                           // Конец блока команд
    return ENC28J60_TX_ERROR;                                                                   // Возвращаем код результата
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t ENC28J60_RecoverRx(void)                                                         // Объявляем внутреннюю функцию
{                                                                                               // Начало блока команд
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                           // Проверяем условие перед следующим действием
                                 ENC28J60_ECON1_RXEN) != HAL_OK) return 0;                      // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,                                             // Проверяем условие перед следующим действием
                               ENC28J60_ECON1_RXRST) != HAL_OK) return 0;                       // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,                                           // Проверяем условие перед следующим действием
                                 ENC28J60_ECON1_RXRST) != HAL_OK) return 0;                     // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_EIR,                                             // Проверяем условие перед следующим действием
                                 ENC28J60_EIR_RXERIF) != HAL_OK) return 0;                      // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        uint8_t pending = 0;                                                                    // Создаём переменную для работы функции
        if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &pending) != HAL_OK)                  // Проверяем условие перед следующим действием
            return 0;                                                                           // Сообщаем об ошибке или отсутствии данных
        while (pending-- != 0)                                                                  // Продолжаем цикл, пока условие верно
        {                                                                                       // Начало блока команд
            if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,                                     // Проверяем условие перед следующим действием
                                       ENC28J60_ECON2_PKTDEC) != HAL_OK)                        // Присваиваем или вычисляем значение
                return 0;                                                                       // Сообщаем об ошибке или отсутствии данных
        }                                                                                       // Конец блока команд
        if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &pending) != HAL_OK ||                // Проверяем условие перед следующим действием
            pending != 0)                                                                       // Присваиваем или вычисляем значение
            return 0;                                                                           // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    if (ENC28J60_ConfigureBuffers() != 1) return 0;                                             // Проверяем условие перед следующим действием
    return ENC28J60_StartReceiving();                                                           // Возвращаем код результата
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
ENC28J60_RxResult ENC28J60_ReceiveFrame(uint8_t *data,                                          // Объявляем функцию для работы с ENC28J60
                                        uint16_t capacity,                                      // Создаём переменную для работы функции
                                        uint16_t *receivedLen)                                  // Создаём переменную для работы функции
{                                                                                               // Начало блока команд
    uint8_t header[ENC28J60_RX_HEADER_SIZE] = {0};                                              // Создаём переменную для работы функции
    uint8_t packetCount = 0;                                                                    // Создаём переменную для работы функции
    uint16_t nextPacket, frameSize, rxStatus, dataSize, freePtr;                                // Создаём переменную для работы функции
    ENC28J60_RxResult result = ENC28J60_RX_BAD_FRAME;                                           // Создаём переменную для работы функции
    if (data == 0 || receivedLen == 0 || capacity == 0)                                         // Проверяем условие перед следующим действием
        return ENC28J60_RX_INVALID_ARG;                                                         // Возвращаем код результата
    *receivedLen = 0;                                                                           // Присваиваем или вычисляем значение
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &packetCount)                             // Проверяем условие перед следующим действием
        != HAL_OK) return ENC28J60_RX_SPI_ERROR;                                                // Проверяем результат операции
    if (packetCount == 0) return ENC28J60_RX_EMPTY;                                             // Проверяем условие перед следующим действием
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,                                       // Проверяем условие перед следующим действием
                                     enc_next_packet) != HAL_OK)                                // Присваиваем или вычисляем значение
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата
    if (ENC28J60_ReadBufferMem(header, ENC28J60_RX_HEADER_SIZE) != HAL_OK)                      // Проверяем условие перед следующим действием
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата
    nextPacket = (uint16_t)(header[0] | ((uint16_t)header[1] << 8));                            // Завершаем вызов функции
    frameSize = (uint16_t)(header[2] | ((uint16_t)header[3] << 8));                             // Завершаем вызов функции
    rxStatus = (uint16_t)(header[4] | ((uint16_t)header[5] << 8));                              // Завершаем вызов функции
    if (nextPacket > ENC28J60_RX_BUF_END ||                                                     // Проверяем условие перед следующим действием
        (nextPacket & 1) != 0)                                                                  // Продолжаем проверку условия или вызов функции
    {                                                                                           // Начало блока команд
        if (ENC28J60_RecoverRx() != 1)                                                          // Проверяем условие перед следующим действием
            return ENC28J60_RX_SPI_ERROR;                                                       // Возвращаем код результата
        return ENC28J60_RX_BAD_POINTER;                                                         // Возвращаем код результата
    }                                                                                           // Конец блока команд
    if ((rxStatus & ENC28J60_RX_STATUS_OK) != 0 &&                                              // Проверяем условие перед следующим действием
        frameSize >= (ENC28J60_FRAME_MIN + ENC28J60_CRC_SIZE) &&                                // Вызываем функцию работы с контроллером
        frameSize <= ENC28J60_RX_FRAME_MAX)                                                     // Присваиваем или вычисляем значение
    {                                                                                           // Начало блока команд
        dataSize = frameSize - ENC28J60_CRC_SIZE;                                               // Присваиваем или вычисляем значение
        if (dataSize > capacity)                                                                // Проверяем условие перед следующим действием
            result = ENC28J60_RX_BUFFER_SMALL;                                                  // Присваиваем или вычисляем значение
        else                                                                                    // Действие, если условие не выполнилось
        {                                                                                       // Начало блока команд
            if (ENC28J60_ReadBufferMem(data, dataSize) != HAL_OK)                               // Проверяем условие перед следующим действием
                return ENC28J60_RX_SPI_ERROR;                                                   // Возвращаем код результата
            *receivedLen = dataSize;                                                            // Присваиваем или вычисляем значение
            result = ENC28J60_RX_OK;                                                            // Присваиваем или вычисляем значение
        }                                                                                       // Конец блока команд
    }                                                                                           // Конец блока команд
    freePtr = (nextPacket == ENC28J60_RX_BUF_START)                                             // Вызываем функцию работы с контроллером
                ? ENC28J60_RX_BUF_END : (uint16_t)(nextPacket - 1);                             // Вызываем функцию работы с контроллером
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXRDPTL,                                     // Проверяем условие перед следующим действием
                                     freePtr) != HAL_OK)                                        // Присваиваем или вычисляем значение
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата
    enc_next_packet = nextPacket;                                                               // Присваиваем или вычисляем значение
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,                                             // Проверяем условие перед следующим действием
                               ENC28J60_ECON2_PKTDEC) != HAL_OK)                                // Присваиваем или вычисляем значение
        return ENC28J60_RX_SPI_ERROR;                                                           // Возвращаем код результата
    return result;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд
///////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t ENC28J60_GetPHYStatus(void)                                                            // Объявляем функцию для работы с ENC28J60
{                                                                                               // Начало блока команд
    uint16_t status = 0;                                                                        // Создаём переменную для работы функции
    if (ENC28J60_ReadPHY(                                                                       // Проверяем условие перед следующим действием
        ENC28J60_PHY_PHSTAT2,                                                                   // Передаём следующий параметр
        &status) != 1)                                                                          // Передаём адрес переменной
    {                                                                                           // Начало блока команд
        return 0;                                                                               // Сообщаем об ошибке или отсутствии данных
    }                                                                                           // Конец блока команд
    return status;                                                                              // Возвращаем полученное значение
}                                                                                               // Конец блока команд

