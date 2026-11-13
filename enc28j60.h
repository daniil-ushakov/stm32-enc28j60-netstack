// Библиотека для работы с модулем ENC28J60
// Автор: Ушаков Д.Д.
// Контроллер STM32F103, библиотека HAL
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef ENC28J60_H                                                                              // Проверяем, подключался ли файл раньше
#define ENC28J60_H                                                                              // Защищаем заголовочный файл от повторного подключения
#include <stdint.h>                                                                             // Подключаем нужный заголовочный файл
///////////////////////////////////////////////////////////////////////////////////////////////////
#define ENC28J60_CMD_RCR                   0x00                                                 // Задаём значение cmd rcr
#define ENC28J60_CMD_RBM                   0x3A                                                 // Задаём значение cmd rbm
#define ENC28J60_CMD_WCR                   0x40                                                 // Задаём значение cmd wcr
#define ENC28J60_CMD_WBM                   0x7A                                                 // Задаём значение cmd wbm
#define ENC28J60_CMD_BFS                   0x80                                                 // Задаём значение cmd bfs
#define ENC28J60_CMD_BFC                   0xA0                                                 // Задаём значение cmd bfc
#define ENC28J60_CMD_SRC                   0xFF                                                 // Задаём значение cmd src
///////////////////////////////////////////////////////////////////////////////////////////////////
#define ENC28J60_REG_ERDPTL                0x00                                                 // Задаём значение reg erdptl
#define ENC28J60_REG_ERDPTH                0x01                                                 // Задаём значение reg erdpth
#define ENC28J60_REG_EWRPTL                0x02                                                 // Задаём значение reg ewrptl
#define ENC28J60_REG_EWRPTH                0x03                                                 // Задаём значение reg ewrpth
#define ENC28J60_REG_ETXSTL                0x04                                                 // Задаём значение reg etxstl
#define ENC28J60_REG_ETXSTH                0x05                                                 // Задаём значение reg etxsth
#define ENC28J60_REG_ETXNDL                0x06                                                 // Задаём значение reg etxndl
#define ENC28J60_REG_ETXNDH                0x07                                                 // Задаём значение reg etxndh
#define ENC28J60_REG_ERXSTL                0x08                                                 // Задаём значение reg erxstl
#define ENC28J60_REG_ERXSTH                0x09                                                 // Задаём значение reg erxsth
#define ENC28J60_REG_ERXNDL                0x0A                                                 // Задаём значение reg erxndl
#define ENC28J60_REG_ERXNDH                0x0B                                                 // Задаём значение reg erxndh
#define ENC28J60_REG_ERXRDPTL              0x0C                                                 // Задаём значение reg erxrdptl
#define ENC28J60_REG_ERXRDPTH              0x0D                                                 // Задаём значение reg erxrdpth
#define ENC28J60_REG_ERXFCON               0x38                                                 // Задаём значение reg erxfcon
#define ENC28J60_REG_EPKTCNT               0x39                                                 // Задаём значение reg epktcnt
#define ENC28J60_REG_MACON1                0xC0                                                 // Задаём значение reg macon1
#define ENC28J60_REG_MACON3                0xC2                                                 // Задаём значение reg macon3
#define ENC28J60_REG_MACON4                0xC3                                                 // Задаём значение reg macon4
#define ENC28J60_REG_MABBIPG               0xC4                                                 // Задаём значение reg mabbipg
#define ENC28J60_REG_MAIPGL                0xC6                                                 // Задаём значение reg maipgl
#define ENC28J60_REG_MAIPGH                0xC7                                                 // Задаём значение reg maipgh
#define ENC28J60_REG_MAMXFLL               0xCA                                                 // Задаём значение reg mamxfll
#define ENC28J60_REG_MAMXFLH               0xCB                                                 // Задаём значение reg mamxflh
#define ENC28J60_REG_MICMD                 0xD2                                                 // Задаём значение reg micmd
#define ENC28J60_REG_MIREGADR              0xD4                                                 // Задаём значение reg miregadr
#define ENC28J60_REG_MIWRL                 0xD6                                                 // Задаём значение reg miwrl
#define ENC28J60_REG_MIWRH                 0xD7                                                 // Задаём значение reg miwrh
#define ENC28J60_REG_MIRDL                 0xD8                                                 // Задаём значение reg mirdl
#define ENC28J60_REG_MIRDH                 0xD9                                                 // Задаём значение reg mirdh
#define ENC28J60_REG_MAADR5                0xE0                                                 // Задаём значение reg maadr5
#define ENC28J60_REG_MAADR6                0xE1                                                 // Задаём значение reg maadr6
#define ENC28J60_REG_MAADR3                0xE2                                                 // Задаём значение reg maadr3
#define ENC28J60_REG_MAADR4                0xE3                                                 // Задаём значение reg maadr4
#define ENC28J60_REG_MAADR1                0xE4                                                 // Задаём значение reg maadr1
#define ENC28J60_REG_MAADR2                0xE5                                                 // Задаём значение reg maadr2
#define ENC28J60_REG_MISTAT                0xEA                                                 // Задаём значение reg mistat
#define ENC28J60_REG_EREVID                0x72                                                 // Задаём значение reg erevid
#define ENC28J60_REG_EIE                   0x1B                                                 // Задаём значение reg eie
#define ENC28J60_REG_EIR                   0x1C                                                 // Задаём значение reg eir
#define ENC28J60_REG_ESTAT                 0x1D                                                 // Задаём значение reg estat
#define ENC28J60_REG_ECON2                 0x1E                                                 // Задаём значение reg econ2
#define ENC28J60_REG_ECON1                 0x1F                                                 // Задаём значение reg econ1
///////////////////////////////////////////////////////////////////////////////////////////////////
#define ENC28J60_PHY_PHCON1                0x00                                                 // Задаём значение phy phcon1
#define ENC28J60_PHY_PHSTAT1               0x01                                                 // Задаём значение phy phstat1
#define ENC28J60_PHY_PHID1                 0x02                                                 // Задаём значение phy phid1
#define ENC28J60_PHY_PHID2                 0x03                                                 // Задаём значение phy phid2
#define ENC28J60_PHY_PHCON2                0x10                                                 // Задаём значение phy phcon2
#define ENC28J60_PHY_PHSTAT2               0x11                                                 // Задаём значение phy phstat2
///////////////////////////////////////////////////////////////////////////////////////////////////
#define ENC28J60_ECON1_BSEL0               0x01                                                 // Задаём значение econ1 bsel0
#define ENC28J60_ECON1_BSEL1               0x02                                                 // Задаём значение econ1 bsel1
#define ENC28J60_ECON1_RXEN                0x04                                                 // Задаём значение econ1 rxen
#define ENC28J60_ECON1_TXRTS               0x08                                                 // Задаём значение econ1 txrts
#define ENC28J60_ECON1_RXRST               0x40                                                 // Задаём значение econ1 rxrst
#define ENC28J60_ECON1_TXRST               0x80                                                 // Задаём значение econ1 txrst
#define ENC28J60_ECON2_PKTDEC              0x40                                                 // Задаём значение econ2 pktdec
#define ENC28J60_ECON2_AUTOINC             0x80                                                 // Задаём значение econ2 autoinc
#define ENC28J60_EIR_RXERIF                0x01                                                 // Задаём значение eir rxerif
#define ENC28J60_EIR_TXERIF                0x02                                                 // Задаём значение eir txerif
#define ENC28J60_EIR_TXIF                  0x08                                                 // Задаём значение eir txif
#define ENC28J60_ESTAT_CLKRDY              0x01                                                 // Задаём значение estat clkrdy
#define ENC28J60_ESTAT_TXABRT              0x02                                                 // Задаём значение estat txabrt
#define ENC28J60_MICMD_MIIRD               0x01                                                 // Задаём значение micmd miird
#define ENC28J60_MISTAT_BUSY               0x01                                                 // Задаём значение mistat busy
#define ENC28J60_PHCON1_PDPXMD             0x0100                                               // Задаём значение phcon1 pdpxmd
#define ENC28J60_PHCON2_HDLDIS             0x0100                                               // Задаём значение phcon2 hdldis
#define ENC28J60_PHSTAT2_DPXSTAT           0x0200                                               // Задаём значение phstat2 dpxstat
#define ENC28J60_PHSTAT2_LSTAT             0x0400                                               // Задаём значение phstat2 lstat
#define ENC28J60_RX_STATUS_OK              0x0080                                               // Задаём значение rx status ok
///////////////////////////////////////////////////////////////////////////////////////////////////
#define ENC28J60_RX_BUF_START              0x0000                                               // Задаём значение rx buf start
#define ENC28J60_RX_BUF_END                0x19FF                                               // Задаём значение rx buf end
#define ENC28J60_TX_BUF_START              0x1A00                                               // Задаём значение tx buf start
#define ENC28J60_TX_BUF_END                0x1FFF                                               // Задаём значение tx buf end
#define ENC28J60_MACON1_VALUE              0x0D                                                 // Задаём значение macon1 value
#define ENC28J60_MACON3_VALUE              0x32                                                 // Задаём значение macon3 value
#define ENC28J60_MACON3_FULDPX             0x01                                                 // Задаём значение macon3 fuldpx
#define ENC28J60_MACON4_VALUE              0x40                                                 // Задаём значение macon4 value
#define ENC28J60_MABBIPG_VALUE             0x12                                                 // Задаём значение mabbipg value
#define ENC28J60_MABBIPG_FULL              0x15                                                 // Задаём значение mabbipg full
#define ENC28J60_MAIPG_VALUE               0x0C12                                               // Задаём значение maipg value
#define ENC28J60_MAIPG_FULL                0x0012                                               // Задаём значение maipg full
#define ENC28J60_MAMXFL_VALUE              1518                                                 // Задаём значение mamxfl value
#define ENC28J60_ERXFCON_VALUE             0xA1                                                 // Задаём значение erxfcon value
#define ENC28J60_PHCON2_VALUE              0x0100                                               // Задаём значение phcon2 value
#define ENC28J60_MAC_ADDRESS_SIZE          6                                                    // Задаём значение mac address size
#define ENC28J60_RX_HEADER_SIZE            6                                                    // Задаём значение rx header size
#define ENC28J60_CRC_SIZE                  4                                                    // Задаём значение crc size
#define ENC28J60_RX_FRAME_MAX              1518                                                 // Задаём значение rx frame max
#define ENC28J60_TX_FRAME_MAX              1514                                                 // Задаём значение tx frame max
#define ENC28J60_FRAME_MIN                 14                                                   // Задаём значение frame min
#define ENC28J60_SPI_TIMEOUT_MS            100                                                  // Задаём значение spi timeout ms
#define ENC28J60_TX_TIMEOUT_MS             100                                                  // Задаём значение tx timeout ms
#define ENC28J60_PHY_TIMEOUT_MS            10                                                   // Задаём значение phy timeout ms
#define ENC28J60_RESET_DELAY_MS            2                                                    // Задаём значение reset delay ms
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum                                                                                    // Объявляем список возможных значений
{                                                                                               // Начало блока команд
    ENC28J60_DUPLEX_HALF = 0,                                                                   // Добавляем значение результата или режима
    ENC28J60_DUPLEX_FULL = 1                                                                    // Добавляем значение результата или режима
} ENC28J60_DuplexMode;                                                                          // Завершаем описание типа
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum                                                                                    // Объявляем список возможных значений
{                                                                                               // Начало блока команд
    ENC28J60_INIT_OK = 0,                                                                       // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_RESET,                                                                    // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_REVISION,                                                                 // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_BUFFERS,                                                                  // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_MAC,                                                                      // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_ADDRESS,                                                                  // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_PHY,                                                                      // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_RX,                                                                       // Добавляем значение результата или режима
    ENC28J60_INIT_ERR_PARAM                                                                     // Добавляем значение результата или режима
} ENC28J60_InitResult;                                                                          // Завершаем описание типа
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum                                                                                    // Объявляем список возможных значений
{                                                                                               // Начало блока команд
    ENC28J60_TX_OK = 0,                                                                         // Добавляем значение результата или режима
    ENC28J60_TX_NO_LINK,                                                                        // Добавляем значение результата или режима
    ENC28J60_TX_INVALID_FRAME,                                                                  // Добавляем значение результата или режима
    ENC28J60_TX_BUSY,                                                                           // Добавляем значение результата или режима
    ENC28J60_TX_ERROR,                                                                          // Добавляем значение результата или режима
    ENC28J60_TX_TIMEOUT                                                                         // Добавляем значение результата или режима
} ENC28J60_TxResult;                                                                            // Завершаем описание типа
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum                                                                                    // Объявляем список возможных значений
{                                                                                               // Начало блока команд
    ENC28J60_RX_EMPTY = 0,                                                                      // Добавляем значение результата или режима
    ENC28J60_RX_OK,                                                                             // Добавляем значение результата или режима
    ENC28J60_RX_INVALID_ARG,                                                                    // Добавляем значение результата или режима
    ENC28J60_RX_BAD_POINTER,                                                                    // Добавляем значение результата или режима
    ENC28J60_RX_BAD_FRAME,                                                                      // Добавляем значение результата или режима
    ENC28J60_RX_BUFFER_SMALL,                                                                   // Добавляем значение результата или режима
    ENC28J60_RX_SPI_ERROR                                                                       // Добавляем значение результата или режима
} ENC28J60_RxResult;                                                                            // Завершаем описание типа
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ENC28J60_SoftReset(void);                                                               // Объявляем функцию для работы с ENC28J60
uint8_t ENC28J60_ReadRevision(void);                                                            // Объявляем функцию для работы с ENC28J60
uint8_t ENC28J60_ConfigureBuffers(void);                                                        // Объявляем функцию для работы с ENC28J60
uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex);                                      // Объявляем функцию для работы с ENC28J60
uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac);                                       // Объявляем функцию для работы с ENC28J60
uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value);                                         // Объявляем функцию для работы с ENC28J60
uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value);                                         // Объявляем функцию для работы с ENC28J60
uint8_t ENC28J60_StartReceiving(void);                                                          // Объявляем функцию для работы с ENC28J60
ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,                                           // Объявляем функцию для работы с ENC28J60
                                   ENC28J60_DuplexMode duplex);                                 // Создаём переменную для работы функции
uint8_t ENC28J60_GetPacketCount(void);                                                          // Объявляем функцию для работы с ENC28J60
ENC28J60_RxResult ENC28J60_ReceiveFrame(                                                        // Объявляем функцию для работы с ENC28J60
    uint8_t *data,                                                                              // Создаём переменную для работы функции
    uint16_t capacity,                                                                          // Создаём переменную для работы функции
    uint16_t *receivedLen                                                                       // Создаём переменную для работы функции
);                                                                                              // Завершаем вызов функции
ENC28J60_TxResult ENC28J60_TransmitFrame(                                                       // Объявляем функцию для работы с ENC28J60
    uint8_t *data,                                                                              // Создаём переменную для работы функции
    uint16_t size                                                                               // Создаём переменную для работы функции
);                                                                                              // Завершаем вызов функции
uint16_t ENC28J60_GetPHYStatus(void);                                                           // Объявляем функцию для работы с ENC28J60
#endif                                                                                          // Конец защиты заголовочного файла

