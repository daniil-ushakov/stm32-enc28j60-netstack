// IPv4 без фрагментации; фиксированный заголовок при формировании.
#ifndef IP_H
#define IP_H
#include <stdint.h>
#define IP_HEADER_SIZE 20U
#define IP_PROTOCOL_ICMP 1U
#define IP_PROTOCOL_UDP 17U
typedef struct
{
    uint8_t version;
    uint8_t headerLength;          // В байтах, от 20 до 60.
    uint8_t protocol;
    uint16_t totalLength;
    uint16_t dataLength;
    uint16_t identification;
    uint16_t flagsOffset;
    uint8_t sourceIp[4];
    uint8_t destinationIp[4];
    const uint8_t *data;          // Указатель на полезные данные IPv4.
} IP_PacketInfo;
uint16_t IP_CalcChecksum(const uint8_t *data, uint16_t size);
uint8_t IP_ParsePacket(const uint8_t *packet, uint16_t size,
                       const uint8_t *localIp, IP_PacketInfo *info);
uint16_t IP_CreatePacket(const uint8_t *sourceIp, const uint8_t *destIp,
                         uint8_t protocol, const uint8_t *payload,
                         uint16_t payloadSize, uint16_t identification,
                         uint8_t *packet, uint16_t capacity);
uint16_t IP_CreateReply(const uint8_t *request, uint16_t requestSize,
                        const uint8_t *payload, uint16_t payloadSize,
                        uint8_t *reply, uint16_t capacity);
uint16_t IP_ProcessPacket(const uint8_t *request, uint16_t requestSize,
                          const uint8_t *localIp, uint8_t *reply,
                          uint16_t capacity);
#endif
