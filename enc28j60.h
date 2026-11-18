// Библиотека для работы с модулем ENC28J60
// Автор: Ушаков Д.Д.
// Контроллер STM32F103, библиотека HAL

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ENC28J60_H                                                                              // Проверяем, не подключён ли заголовок ранее
#define ENC28J60_H                                                                              // Отмечаем, что заголовок уже подключён
#include <stdint.h>                                                                             // Подключаем типы uint8_t и uint16_t

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_CMD_RCR                   0x00                                                 // Читает значение управляющего регистра по SPI
#define ENC28J60_CMD_RBM                   0x3A                                                 // Читает байты из внутренней RAM по SPI
#define ENC28J60_CMD_WCR                   0x40                                                 // Записывает значение управляющего регистра по SPI
#define ENC28J60_CMD_WBM                   0x7A                                                 // Записывает байты во внутреннюю RAM по SPI
#define ENC28J60_CMD_BFS                   0x80                                                 // Устанавливает выбранные биты регистра
#define ENC28J60_CMD_BFC                   0xA0                                                 // Сбрасывает выбранные биты регистра
#define ENC28J60_CMD_SRC                   0xFF                                                 // Выполняет программный сброс ENC28J60

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_REG_ERDPTL                0x00                                                 // Младший байт адреса чтения внутренней RAM
#define ENC28J60_REG_ERDPTH                0x01                                                 // Старший байт адреса чтения внутренней RAM
#define ENC28J60_REG_EWRPTL                0x02                                                 // Младший байт адреса записи внутренней RAM
#define ENC28J60_REG_EWRPTH                0x03                                                 // Старший байт адреса записи внутренней RAM
#define ENC28J60_REG_ETXSTL                0x04                                                 // Младший байт начала передаваемого кадра
#define ENC28J60_REG_ETXSTH                0x05                                                 // Старший байт начала передаваемого кадра
#define ENC28J60_REG_ETXNDL                0x06                                                 // Младший байт последнего байта передаваемого кадра
#define ENC28J60_REG_ETXNDH                0x07                                                 // Старший байт последнего байта передаваемого кадра
#define ENC28J60_REG_ERXSTL                0x08                                                 // Младший байт начала кольцевого RX-буфера
#define ENC28J60_REG_ERXSTH                0x09                                                 // Старший байт начала кольцевого RX-буфера
#define ENC28J60_REG_ERXNDL                0x0A                                                 // Младший байт конца кольцевого RX-буфера
#define ENC28J60_REG_ERXNDH                0x0B                                                 // Старший байт конца кольцевого RX-буфера
#define ENC28J60_REG_ERXRDPTL              0x0C                                                 // Младший байт границы освобождённой RX-памяти
#define ENC28J60_REG_ERXRDPTH              0x0D                                                 // Старший байт границы освобождённой RX-памяти
#define ENC28J60_REG_ERXFCON               0x38                                                 // Выбор фильтров принимаемых Ethernet-кадров
#define ENC28J60_REG_EPKTCNT               0x39                                                 // Количество кадров, ожидающих обработки
#define ENC28J60_REG_MACON1                0xC0                                                 // Разрешение приёма и обработки кадров MAC
#define ENC28J60_REG_MACON3                0xC2                                                 // Настройка CRC, дополнения кадров и Duplex
#define ENC28J60_REG_MACON4                0xC3                                                 // Дополнительные настройки передачи MAC
#define ENC28J60_REG_MABBIPG               0xC4                                                 // Пауза между кадрами при непрерывной передаче
#define ENC28J60_REG_MAIPGL                0xC6                                                 // Младший байт паузы между отдельными кадрами
#define ENC28J60_REG_MAIPGH                0xC7                                                 // Старший байт паузы между отдельными кадрами
#define ENC28J60_REG_MAMXFLL               0xCA                                                 // Младший байт максимальной длины Ethernet-кадра
#define ENC28J60_REG_MAMXFLH               0xCB                                                 // Старший байт максимальной длины Ethernet-кадра
#define ENC28J60_REG_MICMD                 0xD2                                                 // Команда чтения регистра PHY через MII
#define ENC28J60_REG_MIREGADR              0xD4                                                 // Адрес регистра PHY для обмена через MII
#define ENC28J60_REG_MIWRL                 0xD6                                                 // Младший байт записываемого значения PHY
#define ENC28J60_REG_MIWRH                 0xD7                                                 // Старший байт записываемого значения PHY
#define ENC28J60_REG_MIRDL                 0xD8                                                 // Младший байт прочитанного значения PHY
#define ENC28J60_REG_MIRDH                 0xD9                                                 // Старший байт прочитанного значения PHY
#define ENC28J60_REG_MAADR5                0xE0                                                 // Регистр MAADR5, часть аппаратного MAC-адреса
#define ENC28J60_REG_MAADR6                0xE1                                                 // Регистр MAADR6, часть аппаратного MAC-адреса
#define ENC28J60_REG_MAADR3                0xE2                                                 // Регистр MAADR3, часть аппаратного MAC-адреса
#define ENC28J60_REG_MAADR4                0xE3                                                 // Регистр MAADR4, часть аппаратного MAC-адреса
#define ENC28J60_REG_MAADR1                0xE4                                                 // Регистр MAADR1, часть аппаратного MAC-адреса
#define ENC28J60_REG_MAADR2                0xE5                                                 // Регистр MAADR2, часть аппаратного MAC-адреса
#define ENC28J60_REG_MISTAT                0xEA                                                 // Состояние обмена с PHY через MII
#define ENC28J60_REG_EREVID                0x72                                                 // Номер аппаратной ревизии ENC28J60
#define ENC28J60_REG_EIE                   0x1B                                                 // Разрешение источников прерываний
#define ENC28J60_REG_EIR                   0x1C                                                 // Флаги событий и ошибок контроллера
#define ENC28J60_REG_ESTAT                 0x1D                                                 // Текущее состояние контроллера
#define ENC28J60_REG_ECON2                 0x1E                                                 // Управление автоинкрементом RAM и счётчиком RX
#define ENC28J60_REG_ECON1                 0x1F                                                 // Выбор банка и управление приёмом и передачей

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_PHY_PHCON1                0x00                                                 // Выбор режима Duplex в PHY
#define ENC28J60_PHY_PHSTAT1               0x01                                                 // Основные признаки состояния PHY
#define ENC28J60_PHY_PHID1                 0x02                                                 // Старшая часть идентификатора PHY
#define ENC28J60_PHY_PHID2                 0x03                                                 // Младшая часть идентификатора PHY
#define ENC28J60_PHY_PHCON2                0x10                                                 // Управление петлёй обратной связи PHY
#define ENC28J60_PHY_PHSTAT2               0x11                                                 // Состояние Ethernet-соединения и Duplex

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_ECON1_BSEL0               0x01                                                 // Младший бит номера выбранного банка
#define ENC28J60_ECON1_BSEL1               0x02                                                 // Старший бит номера выбранного банка
#define ENC28J60_ECON1_RXEN                0x04                                                 // Разрешает приём Ethernet-кадров
#define ENC28J60_ECON1_TXRTS               0x08                                                 // Запускает передачу подготовленного кадра
#define ENC28J60_ECON1_RXRST               0x40                                                 // Сбрасывает внутреннюю логику приёма
#define ENC28J60_ECON1_TXRST               0x80                                                 // Сбрасывает внутреннюю логику передачи
#define ENC28J60_ECON2_PKTDEC              0x40                                                 // Освобождает один обработанный RX-пакет
#define ENC28J60_ECON2_AUTOINC             0x80                                                 // Автоматически сдвигает указатель RAM
#define ENC28J60_EIR_RXERIF                0x01                                                 // Показывает ошибку приёма кадра
#define ENC28J60_EIR_TXERIF                0x02                                                 // Показывает ошибку передачи кадра
#define ENC28J60_EIR_TXIF                  0x08                                                 // Показывает завершение передачи кадра
#define ENC28J60_ESTAT_CLKRDY              0x01                                                 // Показывает готовность тактового генератора
#define ENC28J60_ESTAT_TXABRT              0x02                                                 // Показывает прерывание передачи из-за ошибки
#define ENC28J60_MICMD_MIIRD               0x01                                                 // Запускает чтение выбранного регистра PHY
#define ENC28J60_MISTAT_BUSY               0x01                                                 // Показывает, что обмен с PHY ещё идёт
#define ENC28J60_PHCON1_PDPXMD             0x0100                                               // Включает Full Duplex в PHY
#define ENC28J60_PHCON2_HDLDIS             0x0100                                               // Отключает автоматическую петлю PHY
#define ENC28J60_PHSTAT2_DPXSTAT           0x0200                                               // Показывает работу PHY в Full Duplex
#define ENC28J60_PHSTAT2_LSTAT             0x0400                                               // Показывает наличие физического соединения
#define ENC28J60_RX_STATUS_OK              0x0080                                               // Показывает, что принятый кадр корректен

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ENC28J60_RX_BUF_START              0x0000                                               // Первый адрес кольцевого RX-буфера
#define ENC28J60_RX_BUF_END                0x19FF                                               // Последний адрес кольцевого RX-буфера
#define ENC28J60_TX_BUF_START              0x1A00                                               // Первый адрес буфера передачи
#define ENC28J60_TX_BUF_END                0x1FFF                                               // Последний адрес буфера передачи
#define ENC28J60_MACON1_VALUE              0x0D                                                 // Разрешены приём MAC и обработка PAUSE-кадров
#define ENC28J60_MACON3_VALUE              0x32                                                 // Half Duplex, CRC и дополнение коротких кадров
#define ENC28J60_MACON3_FULDPX             0x01                                                 // Включает Full Duplex в регистре MACON3
#define ENC28J60_MACON4_VALUE              0x40                                                 // Включает ожидание свободной среды при передаче
#define ENC28J60_MABBIPG_VALUE             0x12                                                 // Межкадровый интервал для Half Duplex
#define ENC28J60_MABBIPG_FULL              0x15                                                 // Межкадровый интервал для Full Duplex
#define ENC28J60_MAIPG_VALUE               0x0C12                                               // Пауза между кадрами для Half Duplex
#define ENC28J60_MAIPG_FULL                0x0012                                               // Пауза между кадрами для Full Duplex
#define ENC28J60_MAMXFL_VALUE              1518                                                 // Максимальная длина кадра вместе с FCS, байт
#define ENC28J60_ERXFCON_VALUE             0xA1                                                 // Принимаем Unicast, Broadcast и кадры с верным CRC
#define ENC28J60_PHCON2_VALUE              0x0100                                               // Отключаем автоматическую петлю PHY
#define ENC28J60_MAC_ADDRESS_SIZE          6                                                    // Длина аппаратного MAC-адреса, байт
#define ENC28J60_RX_HEADER_SIZE            6                                                    // Длина служебного заголовка RX-пакета, байт
#define ENC28J60_CRC_SIZE                  4                                                    // Длина контрольной суммы Ethernet FCS, байт
#define ENC28J60_RX_FRAME_MAX              1518                                                 // Максимальная длина RX-кадра вместе с FCS, байт
#define ENC28J60_TX_FRAME_MAX              1514                                                 // Максимальная длина TX-кадра без FCS, байт
#define ENC28J60_FRAME_MIN                 14                                                   // Минимальная длина заголовка Ethernet, байт
#define ENC28J60_SPI_TIMEOUT_MS            100                                                  // Тайм-аут одной операции SPI, мс
#define ENC28J60_TX_TIMEOUT_MS             100                                                  // Тайм-аут завершения передачи кадра, мс
#define ENC28J60_PHY_TIMEOUT_MS            10                                                   // Тайм-аут обмена с PHY через MII, мс
#define ENC28J60_RESET_DELAY_MS            2                                                    // Задержка после программного сброса, мс

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Перечисление допустимых режимов и результатов
{
    ENC28J60_DUPLEX_HALF = 0,                                                                   // Передача и приём по очереди
    ENC28J60_DUPLEX_FULL = 1                                                                    // Одновременная передача и приём
} ENC28J60_DuplexMode;                                                                          // Тип для передачи выбранного режима или результата

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Перечисление допустимых режимов и результатов
{
    ENC28J60_INIT_OK = 0,                                                                       // Контроллер успешно настроен
    ENC28J60_INIT_ERR_RESET,                                                                    // Не выполнен программный сброс
    ENC28J60_INIT_ERR_REVISION,                                                                 // Не прочитан допустимый номер ревизии
    ENC28J60_INIT_ERR_BUFFERS,                                                                  // Не настроены буферы RAM
    ENC28J60_INIT_ERR_MAC,                                                                      // Не настроены регистры MAC
    ENC28J60_INIT_ERR_ADDRESS,                                                                  // Не записан аппаратный MAC-адрес
    ENC28J60_INIT_ERR_PHY,                                                                      // Не настроен или не отвечает PHY
    ENC28J60_INIT_ERR_RX,                                                                       // Не удалось запустить приём кадров
    ENC28J60_INIT_ERR_PARAM                                                                     // Передан неверный MAC-адрес или Duplex
} ENC28J60_InitResult;                                                                          // Тип для передачи выбранного режима или результата

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Перечисление допустимых режимов и результатов
{
    ENC28J60_TX_OK = 0,                                                                         // Кадр успешно передан
    ENC28J60_TX_NO_LINK,                                                                        // Кабель не подключён или нет Link
    ENC28J60_TX_INVALID_FRAME,                                                                  // Неверный адрес данных или длина кадра
    ENC28J60_TX_BUSY,                                                                           // Передатчик занят предыдущим кадром
    ENC28J60_TX_ERROR,                                                                          // Ошибка передачи или работы контроллера
    ENC28J60_TX_TIMEOUT                                                                         // Передача не завершилась за отведённое время
} ENC28J60_TxResult;                                                                            // Тип для передачи выбранного режима или результата

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum                                                                                    // Перечисление допустимых режимов и результатов
{
    ENC28J60_RX_EMPTY = 0,                                                                      // В очереди нет принятых кадров
    ENC28J60_RX_OK,                                                                             // Кадр успешно считан в память STM32
    ENC28J60_RX_INVALID_ARG,                                                                    // Переданы неверные указатели или размер буфера
    ENC28J60_RX_BAD_POINTER,                                                                    // Повреждён адрес следующего RX-пакета
    ENC28J60_RX_BAD_FRAME,                                                                      // Кадр повреждён или имеет неверную длину
    ENC28J60_RX_BUFFER_SMALL,                                                                   // Буфер STM32 не вмещает принятый кадр
    ENC28J60_RX_SPI_ERROR                                                                       // Не удалось выполнить обмен по SPI
} ENC28J60_RxResult;                                                                            // Тип для передачи выбранного режима или результата

//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ENC28J60_SoftReset(void);                                                               // Сбрасывает ENC28J60; возвращает 1 при успехе
uint8_t ENC28J60_ReadRevision(void);                                                            // Возвращает аппаратную ревизию ENC28J60
uint8_t ENC28J60_ConfigureBuffers(void);                                                        // Задаёт границы буферов RX и TX
uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex);                                      // Настраивает MAC для выбранного Duplex
uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac);                                       // Записывает и проверяет MAC-адрес
uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value);                                         // Читает 16-битный регистр PHY
uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value);                                         // Записывает 16-битный регистр PHY
uint8_t ENC28J60_StartReceiving(void);                                                          // Включает фильтрацию и приём кадров
ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,                                           // Настраивает ENC28J60 по MAC и режиму Duplex
                                   ENC28J60_DuplexMode duplex);                                 // Выбранный режим Half или Full Duplex
uint8_t ENC28J60_GetPacketCount(void);                                                          // Возвращает число ожидающих RX-пакетов
ENC28J60_RxResult ENC28J60_ReceiveFrame(                                                        // Читает один кадр из памяти контроллера
    uint8_t *data,                                                                              // Адрес буфера для Ethernet-кадра
    uint16_t capacity,                                                                          // Размер буфера приёма в байтах
    uint16_t *receivedLen                                                                       // Адрес переменной для длины принятого кадра
);
ENC28J60_TxResult ENC28J60_TransmitFrame(                                                       // Передаёт один готовый Ethernet-кадр
    uint8_t *data,                                                                              // Адрес буфера для Ethernet-кадра
    uint16_t size                                                                               // Длина передаваемого кадра без FCS
);
uint16_t ENC28J60_GetPHYStatus(void);                                                           // Возвращает состояние Link и Duplex
#endif                                                                                          // Завершаем защиту от повторного подключения
