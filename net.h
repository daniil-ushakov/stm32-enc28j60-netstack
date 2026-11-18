// Верхний уровень: конфигурация сети, отправка и приём UDP.
#ifndef NET_H
#define NET_H
#include <stdint.h>
#include "enc28j60.h"
#define NET_UDP_MAX_PAYLOAD 1472U // Ethernet MTU 1500 - IPv4 20 - UDP 8.
#define NET_UDP_FRAME_HEADER 42U // Ethernet 14 + IPv4 20 + UDP 8.

typedef struct
{
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t gateway[4];
} NET_Config;

// Указатель data ссылается на полезные данные внутри переданного frame.
typedef struct
{
    uint8_t sourceMac[6];
    uint8_t sourceIp[4];
    uint16_t sourcePort;
    uint16_t destPort;
    uint16_t dataSize;
    const uint8_t *data;
} NET_UDPInfo;
ENC28J60_TxResult NET_SendUDP(const uint8_t *sourceMac, const uint8_t *sourceIp,
                              const uint8_t *destMac, const uint8_t *destIp,
                              uint16_t sourcePort, uint16_t destPort,
                              const uint8_t *data, uint16_t dataSize,
                              uint8_t *frame, uint16_t capacity);
// Приём UDP без копирования. data действителен, пока не изменён исходный frame.
uint8_t NET_ParseUDP(const uint8_t *frame, uint16_t frameSize,
                     const uint8_t *localIp, uint16_t destPort,
                     NET_UDPInfo *info);
// Приём UDP с копированием в предоставленный буфер.
uint8_t NET_ReceiveUDP(const uint8_t *frame, uint16_t frameSize,
                       const uint8_t *localIp, uint16_t destPort,
                       uint8_t *data, uint16_t capacity,
                       uint16_t *dataSize);

// Результат обработки одного входящего Ethernet-кадра.
typedef enum
{
    NET_POLL_EMPTY = 0,          // В очереди ENC28J60 нет кадров.
    NET_POLL_UDP,                // UDP-данные скопированы в массив приложения.
    NET_POLL_REPLY_SENT,         // Автоматически отправлен ответ ARP или ICMP.
    NET_POLL_IGNORED,            // Пакет не требует обработки или адресован другому порту.
    NET_POLL_UDP_TOO_LARGE,      // UDP-данные не помещаются; udpSize сообщает нужный размер.
    NET_POLL_RX_ERROR,           // Ошибка приёма или повреждённый Ethernet-кадр.
    NET_POLL_TX_ERROR,           // Не удалось передать ответ ARP или ICMP.
    NET_POLL_INVALID_ARG         // Ошибка аргументов функции.
} NET_PollResult;

// Считывает один кадр ENC28J60, сохраняет UDP или автоматически отвечает на ARP/ICMP.
// frame — общий RX/TX Ethernet-буфер; udpData — буфер приложения.
// При NET_POLL_UDP значение *udpSize — число сохранённых байтов.
// При NET_POLL_UDP_TOO_LARGE значение *udpSize — требуемая ёмкость.
// При других результатах *udpSize = 0.
NET_PollResult NET_Poll(const NET_Config *config, uint16_t udpPort,
                        uint8_t *frame, uint16_t frameCapacity,
                        uint8_t *udpData, uint16_t udpCapacity,
                        uint16_t *udpSize);
#endif
