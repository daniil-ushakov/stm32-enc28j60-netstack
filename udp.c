// UDP: разбор, checksum с IPv4 псевдозаголовком, формирование датаграммы.
#include "udp.h"
#include <string.h>

uint8_t UDP_ParseDatagram(const uint8_t *packet, uint16_t size, UDP_PacketInfo *info)
{
    uint16_t length;
    if (packet == 0 || info == 0 || size < UDP_HEADER_SIZE) return 0;
    length = ((uint16_t)packet[4] << 8) | packet[5];
    if (length < UDP_HEADER_SIZE || length > size) return 0;
    info->sourcePort = ((uint16_t)packet[0] << 8) | packet[1];
    info->destPort = ((uint16_t)packet[2] << 8) | packet[3];
    info->length = length;
    info->checksum = ((uint16_t)packet[6] << 8) | packet[7];
    info->dataLength = (uint16_t)(length - UDP_HEADER_SIZE);
    info->data = packet + UDP_HEADER_SIZE;
    return 1;
}

uint16_t UDP_CalcChecksum(const uint8_t *sourceIp, const uint8_t *destIp,
                          const uint8_t *udpData, uint16_t udpLength)
{
    uint32_t sum = 17U + udpLength;  // Protocol UDP и длина из псевдозаголовка.
    uint16_t i;
    if (sourceIp == 0 || destIp == 0 || udpData == 0) return 0;
    for (i = 0; i < 4U; i = (uint16_t)(i + 2U))
    {
        sum += ((uint16_t)sourceIp[i] << 8) | sourceIp[i + 1U];
        sum += ((uint16_t)destIp[i] << 8) | destIp[i + 1U];
    }
    for (i = 0; (uint32_t)i + 1U < udpLength; i = (uint16_t)(i + 2U))
        sum += ((uint16_t)udpData[i] << 8) | udpData[i + 1U];
    if (i < udpLength) sum += (uint16_t)udpData[i] << 8;
    while (sum >> 16) sum = (sum & 0xFFFFU) + (sum >> 16);
    return (uint16_t)~sum;         // На приёме верная сумма даёт ноль.
}

uint16_t UDP_CreatePacket(const uint8_t *sourceIp, const uint8_t *destIp,
                          uint16_t sourcePort, uint16_t destPort,
                          const uint8_t *data, uint16_t dataSize,
                          uint8_t *packet, uint16_t capacity)
{
    uint16_t length, checksum;
    if (sourceIp == 0 || destIp == 0 || packet == 0 ||
        (dataSize != 0 && data == 0) || capacity < UDP_HEADER_SIZE ||
        dataSize > 1472U || dataSize > (uint16_t)(capacity - UDP_HEADER_SIZE))
        return 0;
    length = (uint16_t)(UDP_HEADER_SIZE + dataSize);
    if (dataSize != 0) memmove(packet + UDP_HEADER_SIZE, data, dataSize);
    packet[0] = (uint8_t)(sourcePort >> 8);
    packet[1] = (uint8_t)sourcePort;
    packet[2] = (uint8_t)(destPort >> 8);
    packet[3] = (uint8_t)destPort;
    packet[4] = (uint8_t)(length >> 8);
    packet[5] = (uint8_t)length;
    packet[6] = 0; packet[7] = 0;
    checksum = UDP_CalcChecksum(sourceIp, destIp, packet, length);
    if (checksum == 0) checksum = 0xFFFFU;
    packet[6] = (uint8_t)(checksum >> 8);
    packet[7] = (uint8_t)checksum;
    return length;
}

uint16_t UDP_CreateReply(const uint8_t *request, uint16_t requestSize,
                         const uint8_t *sourceIp, const uint8_t *destIp,
                         uint8_t *reply, uint16_t capacity)
{
    UDP_PacketInfo info;
    uint16_t i, length, checksum;
    if (request == 0 || sourceIp == 0 || destIp == 0 || reply == 0 ||
        !UDP_ParseDatagram(request, requestSize, &info) ||
        info.length != requestSize || info.destPort != UDP_DEMO_PORT ||
        info.length > capacity) return 0;
    if (info.checksum != 0 &&
        UDP_CalcChecksum(sourceIp, destIp, request, info.length) != 0) return 0;
    // Сохраняем полезные данные, но увеличиваем каждый байт на 1: TEST -> UFTU.
    for (i = 0; i < info.dataLength; ++i)
        reply[UDP_HEADER_SIZE + i] = (uint8_t)(info.data[i] + 1U);
    length = (uint16_t)(UDP_HEADER_SIZE + info.dataLength);
    reply[0] = (uint8_t)(info.destPort >> 8);
    reply[1] = (uint8_t)info.destPort;
    reply[2] = (uint8_t)(info.sourcePort >> 8);
    reply[3] = (uint8_t)info.sourcePort;
    reply[4] = (uint8_t)(length >> 8);
    reply[5] = (uint8_t)length;
    reply[6] = 0; reply[7] = 0;
    checksum = UDP_CalcChecksum(destIp, sourceIp, reply, length);
    if (checksum == 0) checksum = 0xFFFFU;
    reply[6] = (uint8_t)(checksum >> 8);
    reply[7] = (uint8_t)checksum;
    return length;
}
