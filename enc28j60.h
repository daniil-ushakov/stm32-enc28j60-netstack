// Библиотека для работы с модулем ENC28J60
// Автор: Ушаков Д.Д.
// Контроллер STM32F103, библиотека HAL

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ENC28J60_H                                                                              // Защита заголовка от повторного подключения
#define ENC28J60_H                                                                              // Признак подключения заголовка
#include <stdint.h>                                                                             // Типы данных C

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_CMD_RCR                   0x00                                                 // Код команды SPI: чтение регистра
#define ENC28J60_CMD_RBM                   0x3A                                                 // Код команды SPI: чтение памяти
#define ENC28J60_CMD_WCR                   0x40                                                 // Код команды SPI: запись регистра
#define ENC28J60_CMD_WBM                   0x7A                                                 // Код команды SPI: запись памяти
#define ENC28J60_CMD_BFS                   0x80                                                 // Код команды SPI: установка битов
#define ENC28J60_CMD_BFC                   0xA0                                                 // Код команды SPI: сброс битов
#define ENC28J60_CMD_SRC                   0xFF                                                 // Код команды SPI: сброс контроллера

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_REG_ERDPTL                0x00                                                 // Адрес регистра ERDPTL для управления контроллером
#define ENC28J60_REG_ERDPTH                0x01                                                 // Адрес регистра ERDPTH для управления контроллером
#define ENC28J60_REG_EWRPTL                0x02                                                 // Адрес регистра EWRPTL для управления контроллером
#define ENC28J60_REG_EWRPTH                0x03                                                 // Адрес регистра EWRPTH для управления контроллером
#define ENC28J60_REG_ETXSTL                0x04                                                 // Адрес регистра ETXSTL для управления передачей
#define ENC28J60_REG_ETXSTH                0x05                                                 // Адрес регистра ETXSTH для управления передачей
#define ENC28J60_REG_ETXNDL                0x06                                                 // Адрес регистра ETXNDL для управления передачей
#define ENC28J60_REG_ETXNDH                0x07                                                 // Адрес регистра ETXNDH для управления передачей
#define ENC28J60_REG_ERXSTL                0x08                                                 // Адрес регистра ERXSTL для управления приёмом
#define ENC28J60_REG_ERXSTH                0x09                                                 // Адрес регистра ERXSTH для управления приёмом
#define ENC28J60_REG_ERXNDL                0x0A                                                 // Адрес регистра ERXNDL для управления приёмом
#define ENC28J60_REG_ERXNDH                0x0B                                                 // Адрес регистра ERXNDH для управления приёмом
#define ENC28J60_REG_ERXRDPTL              0x0C                                                 // Адрес регистра ERXRDPTL для управления приёмом
#define ENC28J60_REG_ERXRDPTH              0x0D                                                 // Адрес регистра ERXRDPTH для управления приёмом
#define ENC28J60_REG_ERXFCON               0x38                                                 // Адрес регистра ERXFCON для управления приёмом
#define ENC28J60_REG_EPKTCNT               0x39                                                 // Адрес регистра EPKTCNT для управления приёмом
#define ENC28J60_REG_MACON1                0xC0                                                 // Адрес регистра MACON1 для управления MAC
#define ENC28J60_REG_MACON3                0xC2                                                 // Адрес регистра MACON3 для управления MAC
#define ENC28J60_REG_MACON4                0xC3                                                 // Адрес регистра MACON4 для управления MAC
#define ENC28J60_REG_MABBIPG               0xC4                                                 // Адрес регистра MABBIPG для управления MAC
#define ENC28J60_REG_MAIPGL                0xC6                                                 // Адрес регистра MAIPGL для управления MAC
#define ENC28J60_REG_MAIPGH                0xC7                                                 // Адрес регистра MAIPGH для управления MAC
#define ENC28J60_REG_MAMXFLL               0xCA                                                 // Адрес регистра MAMXFLL для управления MAC
#define ENC28J60_REG_MAMXFLH               0xCB                                                 // Адрес регистра MAMXFLH для управления MAC
#define ENC28J60_REG_MICMD                 0xD2                                                 // Адрес регистра MICMD для управления контроллером
#define ENC28J60_REG_MIREGADR              0xD4                                                 // Адрес регистра MIREGADR для управления контроллером
#define ENC28J60_REG_MIWRL                 0xD6                                                 // Адрес регистра MIWRL для управления контроллером
#define ENC28J60_REG_MIWRH                 0xD7                                                 // Адрес регистра MIWRH для управления контроллером
#define ENC28J60_REG_MIRDL                 0xD8                                                 // Адрес регистра MIRDL для управления контроллером
#define ENC28J60_REG_MIRDH                 0xD9                                                 // Адрес регистра MIRDH для управления контроллером
#define ENC28J60_REG_MAADR5                0xE0                                                 // Адрес регистра MAADR5 для управления MAC
#define ENC28J60_REG_MAADR6                0xE1                                                 // Адрес регистра MAADR6 для управления MAC
#define ENC28J60_REG_MAADR3                0xE2                                                 // Адрес регистра MAADR3 для управления MAC
#define ENC28J60_REG_MAADR4                0xE3                                                 // Адрес регистра MAADR4 для управления MAC
#define ENC28J60_REG_MAADR1                0xE4                                                 // Адрес регистра MAADR1 для управления MAC
#define ENC28J60_REG_MAADR2                0xE5                                                 // Адрес регистра MAADR2 для управления MAC
#define ENC28J60_REG_MISTAT                0xEA                                                 // Адрес регистра MISTAT для управления контроллером
#define ENC28J60_REG_EREVID                0x72                                                 // Адрес регистра EREVID для управления контроллером
#define ENC28J60_REG_EIE                   0x1B                                                 // Адрес регистра EIE для управления контроллером
#define ENC28J60_REG_EIR                   0x1C                                                 // Адрес регистра EIR для управления контроллером
#define ENC28J60_REG_ESTAT                 0x1D                                                 // Адрес регистра ESTAT для управления контроллером
#define ENC28J60_REG_ECON2                 0x1E                                                 // Адрес регистра ECON2 для управления контроллером
#define ENC28J60_REG_ECON1                 0x1F                                                 // Адрес регистра ECON1 для управления контроллером

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_PHY_PHCON1                0x00                                                 // Адрес регистра PHY PHCON1
#define ENC28J60_PHY_PHSTAT1               0x01                                                 // Адрес регистра PHY PHSTAT1
#define ENC28J60_PHY_PHID1                 0x02                                                 // Адрес регистра PHY PHID1
#define ENC28J60_PHY_PHID2                 0x03                                                 // Адрес регистра PHY PHID2
#define ENC28J60_PHY_PHCON2                0x10                                                 // Адрес регистра PHY PHCON2
#define ENC28J60_PHY_PHSTAT2               0x11                                                 // Адрес регистра PHY PHSTAT2

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_ECON1_BSEL0               0x01                                                 // Битовая маска или значение параметра ECON1_BSEL0
#define ENC28J60_ECON1_BSEL1               0x02                                                 // Битовая маска или значение параметра ECON1_BSEL1
#define ENC28J60_ECON1_RXEN                0x04                                                 // Битовая маска или значение параметра ECON1_RXEN
#define ENC28J60_ECON1_TXRTS               0x08                                                 // Битовая маска или значение параметра ECON1_TXRTS
#define ENC28J60_ECON1_RXRST               0x40                                                 // Битовая маска или значение параметра ECON1_RXRST
#define ENC28J60_ECON1_TXRST               0x80                                                 // Битовая маска или значение параметра ECON1_TXRST
#define ENC28J60_ECON2_PKTDEC              0x40                                                 // Битовая маска или значение параметра ECON2_PKTDEC
#define ENC28J60_ECON2_AUTOINC             0x80                                                 // Битовая маска или значение параметра ECON2_AUTOINC
#define ENC28J60_EIR_RXERIF                0x01                                                 // Битовая маска или значение параметра EIR_RXERIF
#define ENC28J60_EIR_TXERIF                0x02                                                 // Битовая маска или значение параметра EIR_TXERIF
#define ENC28J60_EIR_TXIF                  0x08                                                 // Битовая маска или значение параметра EIR_TXIF
#define ENC28J60_ESTAT_CLKRDY              0x01                                                 // Битовая маска или значение параметра ESTAT_CLKRDY
#define ENC28J60_ESTAT_TXABRT              0x02                                                 // Битовая маска или значение параметра ESTAT_TXABRT
#define ENC28J60_MICMD_MIIRD               0x01                                                 // Битовая маска или значение параметра MICMD_MIIRD
#define ENC28J60_MISTAT_BUSY               0x01                                                 // Битовая маска или значение параметра MISTAT_BUSY
#define ENC28J60_PHCON1_PDPXMD             0x0100                                               // Битовая маска или значение параметра PHCON1_PDPXMD
#define ENC28J60_PHCON2_HDLDIS             0x0100                                               // Битовая маска или значение параметра PHCON2_HDLDIS
#define ENC28J60_PHSTAT2_DPXSTAT           0x0200                                               // Битовая маска или значение параметра PHSTAT2_DPXSTAT
#define ENC28J60_PHSTAT2_LSTAT             0x0400                                               // Битовая маска или значение параметра PHSTAT2_LSTAT
#define ENC28J60_RX_STATUS_OK              0x0080                                               // Битовая маска или значение параметра RX_STATUS_OK

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_RX_BUF_START              0x0000                                               // Граница памяти для приёма
#define ENC28J60_RX_BUF_END                0x19FF                                               // Граница памяти для приёма
#define ENC28J60_TX_BUF_START              0x1A00                                               // Граница памяти для передачи
#define ENC28J60_TX_BUF_END                0x1FFF                                               // Граница памяти для передачи
#define ENC28J60_MACON1_VALUE              0x0D                                                 // Значение для настройки MACON1
#define ENC28J60_MACON3_VALUE              0x32                                                 // Значение для настройки MACON3
#define ENC28J60_MACON3_FULDPX             0x01                                                 // Битовая маска или значение параметра MACON3_FULDPX
#define ENC28J60_MACON4_VALUE              0x40                                                 // Значение для настройки MACON4
#define ENC28J60_MABBIPG_VALUE             0x12                                                 // Значение для настройки MABBIPG
#define ENC28J60_MABBIPG_FULL              0x15                                                 // Битовая маска или значение параметра MABBIPG_FULL
#define ENC28J60_MAIPG_VALUE               0x0C12                                               // Значение для настройки MAIPG
#define ENC28J60_MAIPG_FULL                0x0012                                               // Битовая маска или значение параметра MAIPG_FULL
#define ENC28J60_MAMXFL_VALUE              1518                                                 // Значение для настройки MAMXFL
#define ENC28J60_ERXFCON_VALUE             0xA1                                                 // Значение для настройки ERXFCON
#define ENC28J60_PHCON2_VALUE              0x0100                                               // Значение для настройки PHCON2
#define ENC28J60_MAC_ADDRESS_SIZE          6                                                    // Количество байтов в соответствующем поле
#define ENC28J60_RX_HEADER_SIZE            6                                                    // Количество байтов в соответствующем поле
#define ENC28J60_CRC_SIZE                  4                                                    // Количество байтов в соответствующем поле
#define ENC28J60_RX_FRAME_MAX              1518                                                 // Ограничение размера Ethernet-кадра
#define ENC28J60_TX_FRAME_MAX              1514                                                 // Ограничение размера Ethernet-кадра
#define ENC28J60_FRAME_MIN                 14                                                   // Ограничение размера Ethernet-кадра
#define ENC28J60_SPI_TIMEOUT_MS            100                                                  // Предельное время ожидания операции в миллисекундах
#define ENC28J60_TX_TIMEOUT_MS             100                                                  // Предельное время ожидания операции в миллисекундах
#define ENC28J60_PHY_TIMEOUT_MS            10                                                   // Адрес регистра PHY TIMEOUT_MS
#define ENC28J60_RESET_DELAY_MS            2                                                    // Пауза после сброса в миллисекундах

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Набор возможных значений параметра или результата
{
    ENC28J60_DUPLEX_HALF = 0,                                                                   // Добавляем значение результата или режима
    ENC28J60_DUPLEX_FULL = 1                                                                    // Добавляем значение результата или режима
} ENC28J60_DuplexMode;                                                                          // Имя типа для использования в программе

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Набор возможных значений параметра или результата
{
    ENC28J60_INIT_OK = 0,                                                                       // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_RESET,                                                                    // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_REVISION,                                                                 // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_BUFFERS,                                                                  // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_MAC,                                                                      // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_ADDRESS,                                                                  // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_PHY,                                                                      // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_RX,                                                                       // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_PARAM                                                                     // Добавляем значение результата или режима
} ENC28J60_InitResult;                                                                          // Имя типа для использования в программе

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Набор возможных значений параметра или результата
{
    ENC28J60_TX_OK = 0,                                                                         // Добавляем значение результата или режима
    ENC28J60_TX_NO_LINK,                                                                        // Добавляем значение результата или режима
    ENC28J60_TX_INVALID_FRAME,                                                                  // Добавляем значение результата или режима
    ENC28J60_TX_BUSY,                                                                           // Добавляем значение результата или режима
    ENC28J60_TX_ERROR,                                                                          // Добавляем значение результата или режима
    ENC28J60_TX_TIMEOUT                                                                         // Добавляем значение результата или режима
} ENC28J60_TxResult;                                                                            // Имя типа для использования в программе

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Набор возможных значений параметра или результата
{
    ENC28J60_RX_EMPTY = 0,                                                                      // Добавляем значение результата или режима
    ENC28J60_RX_OK,                                                                             // Добавляем значение результата или режима
    ENC28J60_RX_INVALID_ARG,                                                                    // Добавляем значение результата или режима
    ENC28J60_RX_BAD_POINTER,                                                                    // Добавляем значение результата или режима
    ENC28J60_RX_BAD_FRAME,                                                                      // Добавляем значение результата или режима
    ENC28J60_RX_BUFFER_SMALL,                                                                   // Добавляем значение результата или режима
    ENC28J60_RX_SPI_ERROR                                                                       // Добавляем значение результата или режима
} ENC28J60_RxResult;                                                                            // Имя типа для использования в программе

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_SoftReset(void);                                                               // Интерфейс функции для вызова из программы
uint8_t ENC28J60_ReadRevision(void);                                                            // Интерфейс функции для вызова из программы
uint8_t ENC28J60_ConfigureBuffers(void);                                                        // Интерфейс функции для вызова из программы
uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex);                                      // Интерфейс функции для вызова из программы
uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac);                                       // Интерфейс функции для вызова из программы
uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value);                                         // Интерфейс функции для вызова из программы
uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value);                                         // Интерфейс функции для вызова из программы
uint8_t ENC28J60_StartReceiving(void);                                                          // Интерфейс функции для вызова из программы
ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,                                           // Интерфейс функции для вызова из программы
                                   ENC28J60_DuplexMode duplex);
uint8_t ENC28J60_GetPacketCount(void);                                                          // Интерфейс функции для вызова из программы
ENC28J60_RxResult ENC28J60_ReceiveFrame(                                                        // Интерфейс функции для вызова из программы
    uint8_t *data,                                                                              // Промежуточные значения для выполнения операции
    uint16_t capacity,                                                                          // Промежуточные значения для выполнения операции
    uint16_t *receivedLen                                                                       // Промежуточные значения для выполнения операции
);
ENC28J60_TxResult ENC28J60_TransmitFrame(                                                       // Интерфейс функции для вызова из программы
    uint8_t *data,                                                                              // Промежуточные значения для выполнения операции
    uint16_t size                                                                               // Количество байтов для обмена по SPI
);
uint16_t ENC28J60_GetPHYStatus(void);                                                           // Интерфейс функции для вызова из программы
#endif                                                                                          // Конец защиты заголовочного файла
