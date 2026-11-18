// UDP (IPv4): любые порты и переменная длина полезных данных.
#ifndef UDP_H
#define UDP_H
#include <stdint.h>
#define UDP_HEADER_SIZE 8U
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
#endif
