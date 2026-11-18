// UDP (IPv4), тестовый ответ на порт 33333.
#ifndef UDP_H
#define UDP_H
#include <stdint.h>
#define UDP_HEADER_SIZE 8U
#define UDP_DEMO_PORT 33333U
typedef struct
{
    uint16_t sourcePort;
    uint16_t destPort;
    uint16_t length;
    uint16_t checksum;
    uint16_t dataLength;
    const uint8_t *data;
} UDP_PacketInfo;
uint8_t UDP_ParseDatagram(const uint8_t *packet, uint16_t size, UDP_PacketInfo *info);
uint16_t UDP_CalcChecksum(const uint8_t *sourceIp, const uint8_t *destIp,
                          const uint8_t *udpData, uint16_t udpLength);
uint16_t UDP_CreatePacket(const uint8_t *sourceIp, const uint8_t *destIp,
                          uint16_t sourcePort, uint16_t destPort,
                          const uint8_t *data, uint16_t dataSize,
                          uint8_t *packet, uint16_t capacity);
uint16_t UDP_CreateReply(const uint8_t *request, uint16_t requestSize,
                         const uint8_t *sourceIp, const uint8_t *destIp,
                         uint8_t *reply, uint16_t capacity);
#endif
