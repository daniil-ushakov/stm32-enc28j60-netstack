// IPv4: проверка заголовка, формирование ответа ICMP или тестового UDP.
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include <string.h>

uint16_t IP_CalcChecksum(const uint8_t *data, uint16_t size)
{
    uint32_t sum = 0;
    uint16_t i;
    if (data == 0) return 0;
    for (i = 0; (uint32_t)i + 1U < size; i = (uint16_t)(i + 2U))
        sum += ((uint16_t)data[i] << 8) | data[i + 1U];
    if (i < size) sum += (uint16_t)data[i] << 8;
    while (sum >> 16) sum = (sum & 0xFFFFU) + (sum >> 16);
    return (uint16_t)~sum;
}

uint8_t IP_ParsePacket(const uint8_t *packet, uint16_t size,
                       const uint8_t *localIp, IP_PacketInfo *info)
{
    uint8_t headerLength;
    uint16_t totalLength, fragment;
    static const uint8_t broadcast[4] = {255, 255, 255, 255};
    if (packet == 0 || localIp == 0 || info == 0 || size < IP_HEADER_SIZE)
        return 0;
    if ((packet[0] >> 4) != 4U || (packet[0] & 15U) < 5U) return 0;
    headerLength = (uint8_t)((packet[0] & 15U) * 4U);
    if (headerLength > size) return 0;
    totalLength = ((uint16_t)packet[2] << 8) | packet[3];
    if (totalLength < headerLength || totalLength > size) return 0;
    if (IP_CalcChecksum(packet, headerLength) != 0) return 0;
    // Фрагментированные пакеты пока не поддерживаются.
    fragment = ((uint16_t)packet[6] << 8) | packet[7];
    if ((fragment & 0x3FFFU) != 0) return 0;
    if (memcmp(packet + 16, localIp, 4) != 0 &&
        memcmp(packet + 16, broadcast, 4) != 0) return 0;
    info->version = 4;
    info->headerLength = headerLength;
    info->totalLength = totalLength;
    info->dataLength = (uint16_t)(totalLength - headerLength);
    info->protocol = packet[9];
    info->identification = ((uint16_t)packet[4] << 8) | packet[5];
    info->flagsOffset = fragment;
    memcpy(info->sourceIp, packet + 12, 4);
    memcpy(info->destinationIp, packet + 16, 4);
    info->data = packet + headerLength;
    return 1;
}

uint16_t IP_CreatePacket(const uint8_t *sourceIp, const uint8_t *destIp,
                         uint8_t protocol, const uint8_t *payload,
                         uint16_t payloadSize, uint16_t identification,
                         uint8_t *packet, uint16_t capacity)
{
    uint16_t length, checksum;
    if (sourceIp == 0 || destIp == 0 || packet == 0 ||
        (payloadSize != 0 && payload == 0) ||
        payloadSize > 1480U || capacity < IP_HEADER_SIZE ||
        payloadSize > (uint16_t)(capacity - IP_HEADER_SIZE)) return 0;
    length = (uint16_t)(IP_HEADER_SIZE + payloadSize);
    if (payloadSize != 0) memmove(packet + IP_HEADER_SIZE, payload, payloadSize);
    packet[0] = 0x45;              // IPv4, IHL = 5.
    packet[1] = 0;                 // DSCP, ECN.
    packet[2] = (uint8_t)(length >> 8);
    packet[3] = (uint8_t)length;
    packet[4] = (uint8_t)(identification >> 8);
    packet[5] = (uint8_t)identification;
    packet[6] = 0x40; packet[7] = 0; // DF=1, фрагментации нет.
    packet[8] = 64;                // TTL.
    packet[9] = protocol;
    packet[10] = 0; packet[11] = 0;
    memcpy(packet + 12, sourceIp, 4);
    memcpy(packet + 16, destIp, 4);
    checksum = IP_CalcChecksum(packet, IP_HEADER_SIZE);
    packet[10] = (uint8_t)(checksum >> 8);
    packet[11] = (uint8_t)checksum;
    return length;
}

uint16_t IP_CreateReply(const uint8_t *request, uint16_t requestSize,
                        const uint8_t *payload, uint16_t payloadSize,
                        uint8_t *reply, uint16_t capacity)
{
    uint8_t protocol;
    uint16_t identification, headerLength;
    if (request == 0 || requestSize < IP_HEADER_SIZE ||
        (request[0] >> 4) != 4U || (request[0] & 15U) < 5U) return 0;
    headerLength = (uint16_t)((request[0] & 15U) * 4U);
    if (requestSize < headerLength) return 0;
    protocol = request[9];
    identification = ((uint16_t)request[4] << 8) | request[5];
    return IP_CreatePacket(request + 16, request + 12, protocol,
                           payload, payloadSize, (uint16_t)(identification + 1U),
                           reply, capacity);
}

uint16_t IP_ProcessPacket(const uint8_t *request, uint16_t requestSize,
                          const uint8_t *localIp, uint8_t *reply,
                          uint16_t capacity)
{
    IP_PacketInfo info;
    uint16_t dataLength;
    static const uint8_t broadcast[4] = {255, 255, 255, 255};
    if (reply == 0 || capacity < IP_HEADER_SIZE ||
        !IP_ParsePacket(request, requestSize, localIp, &info)) return 0;
    // Не отправляем автоматические ответы на broadcast.
    if (memcmp(info.destinationIp, broadcast, 4) == 0) return 0;
    dataLength = 0;
    if (info.protocol == IP_PROTOCOL_ICMP)
        dataLength = ICMP_ProcessPacket(info.data, info.dataLength,
                                      reply + IP_HEADER_SIZE,
                                      (uint16_t)(capacity - IP_HEADER_SIZE));
    else if (info.protocol == IP_PROTOCOL_UDP)
        dataLength = UDP_CreateReply(info.data, info.dataLength,
                                     info.sourceIp, info.destinationIp,
                                     reply + IP_HEADER_SIZE,
                                     (uint16_t)(capacity - IP_HEADER_SIZE));
    if (dataLength == 0) return 0;
    return IP_CreatePacket(localIp, info.sourceIp, info.protocol,
                           reply + IP_HEADER_SIZE, dataLength,
                           (uint16_t)(info.identification + 1U), reply, capacity);
}
