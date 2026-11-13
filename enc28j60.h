
#ifndef ENC28J60_H
#define ENC28J60_H

#include <stdint.h> // Стандартные целочисленные типы


/* Команды SPI */

#define ENC28J60_CMD_RCR 0x00 // Чтение управляющего регистра
#define ENC28J60_CMD_RBM 0x3A // Чтение из внутренней буферной памяти
#define ENC28J60_CMD_WCR 0x40 // Запись управляющего регистра
#define ENC28J60_CMD_WBM 0x7A // Запись во внутреннюю буферную память
#define ENC28J60_CMD_BFS 0x80 // Установка отдельных битов регистра
#define ENC28J60_CMD_BFC 0xA0 // Сброс отдельных битов регистра
#define ENC28J60_CMD_SRC 0xFF // Программный сброс контроллера


/* Регистры ENC28J60 */

// Младшие 5 бит - адрес регистра
// Биты 5-6 - номер банка
// Бит 7 - признак регистра MAC/MII

#define ENC28J60_REG_ERDPTL   0x00 // Младший байт указателя чтения RAM
#define ENC28J60_REG_ERDPTH   0x01 // Старший байт указателя чтения RAM
#define ENC28J60_REG_EWRPTL   0x02 // Младший байт указателя записи RAM
#define ENC28J60_REG_EWRPTH   0x03 // Старший байт указателя записи RAM
#define ENC28J60_REG_ETXSTL   0x04 // Младший байт начала TX-буфера
#define ENC28J60_REG_ETXSTH   0x05 // Старший байт начала TX-буфера
#define ENC28J60_REG_ETXNDL   0x06 // Младший байт конца TX-кадра
#define ENC28J60_REG_ETXNDH   0x07 // Старший байт конца TX-кадра
#define ENC28J60_REG_ERXSTL   0x08 // Младший байт начала RX-буфера
#define ENC28J60_REG_ERXSTH   0x09 // Старший байт начала RX-буфера
#define ENC28J60_REG_ERXNDL   0x0A // Младший байт конца RX-буфера
#define ENC28J60_REG_ERXNDH   0x0B // Старший байт конца RX-буфера
#define ENC28J60_REG_ERXRDPTL 0x0C // Младший байт границы освобожденной RX-памяти
#define ENC28J60_REG_ERXRDPTH 0x0D // Старший байт границы освобожденной RX-памяти

#define ENC28J60_REG_ERXFCON  0x38 // Банк 1: фильтрация принятых кадров
#define ENC28J60_REG_EPKTCNT  0x39 // Банк 1: число принятых кадров

#define ENC28J60_REG_MACON1   0xC0 // Банк 2, MAC: управление приемом
#define ENC28J60_REG_MACON3   0xC2 // Банк 2, MAC: длина, CRC, дополнение, Duplex
#define ENC28J60_REG_MACON4   0xC3 // Банк 2, MAC: дополнительная логика передачи
#define ENC28J60_REG_MABBIPG  0xC4 // Банк 2, MAC: интервал кадров Back-to-Back
#define ENC28J60_REG_MAIPGL   0xC6 // Банк 2, MAC: младший байт интервала Non-Back-to-Back
#define ENC28J60_REG_MAIPGH   0xC7 // Банк 2, MAC: старший байт интервала Non-Back-to-Back
#define ENC28J60_REG_MAMXFLL  0xCA // Банк 2, MAC: младший байт предельной длины кадра
#define ENC28J60_REG_MAMXFLH  0xCB // Банк 2, MAC: старший байт предельной длины кадра

#define ENC28J60_REG_MICMD    0xD2 // Банк 2, MII: запуск чтения PHY
#define ENC28J60_REG_MIREGADR 0xD4 // Банк 2, MII: адрес регистра PHY
#define ENC28J60_REG_MIWRL    0xD6 // Банк 2, MII: младший байт записи PHY
#define ENC28J60_REG_MIWRH    0xD7 // Банк 2, MII: старший байт записи PHY
#define ENC28J60_REG_MIRDL    0xD8 // Банк 2, MII: младший байт чтения PHY
#define ENC28J60_REG_MIRDH    0xD9 // Банк 2, MII: старший байт чтения PHY

#define ENC28J60_REG_MAADR5   0xE0 // Банк 3, MAC: регистр адреса MAADR5
#define ENC28J60_REG_MAADR6   0xE1 // Банк 3, MAC: регистр адреса MAADR6
#define ENC28J60_REG_MAADR3   0xE2 // Банк 3, MAC: регистр адреса MAADR3
#define ENC28J60_REG_MAADR4   0xE3 // Банк 3, MAC: регистр адреса MAADR4
#define ENC28J60_REG_MAADR1   0xE4 // Банк 3, MAC: регистр адреса MAADR1
#define ENC28J60_REG_MAADR2   0xE5 // Банк 3, MAC: регистр адреса MAADR2

#define ENC28J60_REG_MISTAT   0xEA // Банк 3, MII: состояние операции PHY
#define ENC28J60_REG_EREVID   0x72 // Банк 3: аппаратная ревизия ENC28J60

#define ENC28J60_REG_EIE      0x1B // Общий: разрешения прерываний
#define ENC28J60_REG_EIR      0x1C // Общий: флаги событий и ошибок
#define ENC28J60_REG_ESTAT    0x1D // Общий: состояние контроллера
#define ENC28J60_REG_ECON2    0x1E // Общий: автоинкремент и счетчик пакетов
#define ENC28J60_REG_ECON1    0x1F // Общий: банк, RX и TX

#define ENC28J60_PHY_PHCON1   0x00 // PHY: режим работы
#define ENC28J60_PHY_PHSTAT1  0x01 // PHY: первый регистр состояния
#define ENC28J60_PHY_PHID1    0x02 // PHY: первая часть идентификатора
#define ENC28J60_PHY_PHID2    0x03 // PHY: вторая часть идентификатора
#define ENC28J60_PHY_PHCON2   0x10 // PHY: дополнительные параметры
#define ENC28J60_PHY_PHSTAT2  0x11 // PHY: Link и Duplex


/* Битовые маски */

#define ENC28J60_ECON1_BSEL0       0x01   // Младший бит номера банка
#define ENC28J60_ECON1_BSEL1       0x02   // Старший бит номера банка
#define ENC28J60_ECON1_RXEN        0x04   // Разрешение приема кадров
#define ENC28J60_ECON1_TXRTS       0x08   // Запуск передачи
#define ENC28J60_ECON1_RXRST       0x40   // Сброс приемной логики
#define ENC28J60_ECON1_TXRST       0x80   // Сброс передающей логики

#define ENC28J60_ECON2_PKTDEC      0x40   // Уменьшение счетчика принятых кадров
#define ENC28J60_ECON2_AUTOINC     0x80   // Автоинкремент указателя RAM

#define ENC28J60_EIR_RXERIF        0x01   // Ошибка приема
#define ENC28J60_EIR_TXERIF        0x02   // Ошибка передачи
#define ENC28J60_EIR_TXIF          0x08   // Передача завершилась

#define ENC28J60_ESTAT_CLKRDY      0x01   // Генератор тактового сигнала готов
#define ENC28J60_ESTAT_TXABRT      0x02   // Передача прервана

#define ENC28J60_MICMD_MIIRD       0x01   // Начало чтения PHY
#define ENC28J60_MISTAT_BUSY       0x01   // MII-операция еще выполняется

#define ENC28J60_PHCON1_PDPXMD     0x0100 // Выбор Full Duplex в PHY
#define ENC28J60_PHCON2_HDLDIS     0x0100 // Отключение автоматического обратного соединения PHY
#define ENC28J60_PHSTAT2_DPXSTAT   0x0200 // Работа PHY в Full Duplex
#define ENC28J60_PHSTAT2_LSTAT     0x0400 // Физическое Ethernet-соединение установлено

#define ENC28J60_RX_STATUS_OK      0x0080 // Признак корректного кадра в RX Status Vector


/* Константы настройки */

// Адреса буферной памяти

#define ENC28J60_RX_BUF_START       0x0000 // Первый адрес кольцевого приемного буфера
#define ENC28J60_RX_BUF_END         0x19FF // Последний адрес RX, нечетный по errata B7
#define ENC28J60_TX_BUF_START       0x1A00 // Первый адрес буфера передачи
#define ENC28J60_TX_BUF_END         0x1FFF // Последний адрес памяти передачи

// Настройки MAC

#define ENC28J60_MACON1_VALUE      0x0D   // Прием MAC: MARXEN, RXPAUS, TXPAUS
#define ENC28J60_MACON3_VALUE      0x32   // Базовая конфигурация MACON3 (Half Duplex)
#define ENC28J60_MACON3_FULDPX     0x01   // Выбор Full Duplex в MACON3
#define ENC28J60_MACON4_VALUE      0x40   // DEFER: ожидание освобождения среды передачи
#define ENC28J60_MABBIPG_VALUE     0x12   // Интервал Back-to-Back для Half Duplex
#define ENC28J60_MABBIPG_FULL      0x15   // Интервал Back-to-Back для Full Duplex
#define ENC28J60_MAIPG_VALUE       0x0C12 // Интервал Non-Back-to-Back для Half Duplex
#define ENC28J60_MAIPG_FULL        0x0012 // Интервал Non-Back-to-Back для Full Duplex
#define ENC28J60_MAMXFL_VALUE      1518   // Максимальный Ethernet-кадр с FCS

// Настройки приема и PHY

#define ENC28J60_ERXFCON_VALUE     0xA1   // Фильтр CRC, Broadcast и Unicast
#define ENC28J60_PHCON2_VALUE      0x0100 // Отключение автоматического loopback PHY

// Размеры и ограничения

#define ENC28J60_MAC_ADDRESS_SIZE  6      // Шесть байтов MAC-адреса
#define ENC28J60_RX_HEADER_SIZE    6      // Служебный заголовок приема: указатель и RSV
#define ENC28J60_CRC_SIZE          4      // Длина Ethernet FCS
#define ENC28J60_RX_FRAME_MAX      1518   // Максимальная длина принятого кадра с FCS (без VLAN)
#define ENC28J60_TX_FRAME_MAX      1514   // Максимальная длина кадра без FCS (без VLAN)
#define ENC28J60_FRAME_MIN         14     // Минимум для заголовка Ethernet до дополнения

// Тайм-ауты и задержки

#define ENC28J60_SPI_TIMEOUT_MS    100    // Время ожидания одной операции SPI
#define ENC28J60_TX_TIMEOUT_MS     100    // Время ожидания завершения передачи
#define ENC28J60_PHY_TIMEOUT_MS    10     // Время ожидания операции MII
#define ENC28J60_RESET_DELAY_MS    2      // Пауза после программного сброса


/* Режим работы Ethernet */

// ENC28J60 не поддерживает автоматическое согласование Half/Full Duplex.
typedef enum
{
    ENC28J60_DUPLEX_HALF = 0, // Полудуплексный режим
    ENC28J60_DUPLEX_FULL = 1  // Полнодуплексный режим

} ENC28J60_DuplexMode;


/* Коды результатов */

// Результаты инициализации контроллера

typedef enum
{
    ENC28J60_INIT_OK = 0,       // Все этапы инициализации выполнены
    ENC28J60_INIT_ERR_RESET,    // Не удалось выполнить сброс
    ENC28J60_INIT_ERR_REVISION, // Некорректная ревизия или нет ответа по SPI
    ENC28J60_INIT_ERR_BUFFERS,  // Ошибка настройки кольцевой и TX-памяти
    ENC28J60_INIT_ERR_MAC,      // Ошибка параметров MAC
    ENC28J60_INIT_ERR_ADDRESS,  // Ошибка записи аппаратного MAC-адреса
    ENC28J60_INIT_ERR_PHY,      // Ошибка работы с PHY
    ENC28J60_INIT_ERR_RX,       // Не удалось разрешить прием
    ENC28J60_INIT_ERR_PARAM     // Некорректный MAC-адрес или Duplex

} ENC28J60_InitResult;

// Результаты передачи Ethernet-кадра

typedef enum
{
    ENC28J60_TX_OK = 0,        // Передача завершилась без ошибки
    ENC28J60_TX_NO_LINK,       // Не установлено физическое соединение
    ENC28J60_TX_INVALID_FRAME, // Некорректный буфер или длина кадра
    ENC28J60_TX_BUSY,          // Передатчик уже занят
    ENC28J60_TX_ERROR,         // Контроллер сообщил об ошибке передачи
    ENC28J60_TX_TIMEOUT        // Не дождались завершения передачи

} ENC28J60_TxResult;

// Результаты приема Ethernet-кадра

typedef enum
{
    ENC28J60_RX_EMPTY = 0,      // Нет ожидающих кадров
    ENC28J60_RX_OK,             // Кадр скопирован в память STM32
    ENC28J60_RX_INVALID_ARG,    // Некорректные параметры вызова
    ENC28J60_RX_BAD_POINTER,    // Некорректный указатель следующего пакета
    ENC28J60_RX_BAD_FRAME,      // Ошибка CRC или некорректная длина
    ENC28J60_RX_BUFFER_SMALL,   // Недостаточно места в выходном буфере
    ENC28J60_RX_SPI_ERROR       // Ошибка чтения данных по SPI

} ENC28J60_RxResult;


/* Прототипы функций */

// Выполняет программный сброс контроллера.
// Возвращает 1 при успехе, 0 при ошибке SPI.
uint8_t ENC28J60_SoftReset(void);

// Возвращает значение EREVID, то есть номер аппаратной ревизии.
uint8_t ENC28J60_ReadRevision(void);

// Задает адреса кольцевого RX-буфера и TX-области.
// Возвращает 1 при успехе, 0 при ошибке.
uint8_t ENC28J60_ConfigureBuffers(void);

// Настраивает MAC, межкадровые интервалы и максимальную длину.
// Возвращает 1 при успешной настройке.
uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex);

// Записывает MAC-адрес из массива mac[6] и проверяет чтением.
// Возвращает 1 при успешной записи.
uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac);

// Считывает PHY-регистр reg в переменную по указателю value.
// reg - адрес регистра PHY.
// value - указатель для сохранения прочитанного значения.
// Возвращает 1 при успехе, 0 при ошибке или тайм-ауте.
uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value);

// Записывает 16-битное значение value в PHY-регистр reg.
// reg - адрес регистра PHY.
// value - записываемое значение.
// Возвращает 1 при успехе, 0 при ошибке.
uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value);

// Включает фильтр входящих кадров и разрешает прием RXEN.
// Возвращает 1 при успешном запуске.
uint8_t ENC28J60_StartReceiving(void);

// Выполняет полную инициализацию контроллера.
// mac - указатель на массив из шести байтов MAC-адреса.
// duplex - режим ENC28J60_DUPLEX_HALF или ENC28J60_DUPLEX_FULL.
// Возвращает код результата ENC28J60_InitResult.
ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,
                                   ENC28J60_DuplexMode duplex);

// Возвращает количество ожидающих пакетов из регистра EPKTCNT.
uint8_t ENC28J60_GetPacketCount(void);

// Считывает один кадр из RX-буфера ENC28J60.
// data - массив для сохранения кадра.
// capacity - размер массива в байтах.
// receivedLen - указатель для сохранения длины кадра без FCS.
// Возвращает код результата ENC28J60_RxResult.
ENC28J60_RxResult ENC28J60_ReceiveFrame(
    uint8_t *data,
    uint16_t capacity,
    uint16_t *receivedLen
);

// Передает Ethernet-кадр через ENC28J60.
// data - указатель на готовый Ethernet-кадр.
// size - длина кадра без FCS в байтах.
// Возвращает код результата ENC28J60_TxResult.
ENC28J60_TxResult ENC28J60_TransmitFrame(
    uint8_t *data,
    uint16_t size
);

// Считывает регистр PHSTAT2 для диагностики Link и Duplex.
// Возвращает содержимое PHSTAT2, при ошибке чтения - 0.
uint16_t ENC28J60_GetPHYStatus(void);

#endif // ENC28J60_H