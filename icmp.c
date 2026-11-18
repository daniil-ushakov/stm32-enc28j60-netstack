// ICMP: отвечаем на ping, сохраняя identifier, sequence и данные.
#include "icmp.h"
#include <string.h>

uint16_t ICMP_CalcChecksum(const uint8_t *data, uint16_t size)
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

uint16_t ICMP_ProcessPacket(const uint8_t *request, uint16_t size,
                            uint8_t *reply, uint16_t capacity)
{
    uint16_t checksum;
    if (request == 0 || reply == 0 || size < 8U || size > capacity ||
        request[0] != 8U || request[1] != 0U) return 0;
    if (ICMP_CalcChecksum(request, size) != 0) return 0;
    memmove(reply, request, size);
    reply[0] = 0;                   // Echo Reply.
    reply[2] = 0; reply[3] = 0;     // Пересчитываем ICMP checksum.
    checksum = ICMP_CalcChecksum(reply, size);
    reply[2] = (uint8_t)(checksum >> 8);
    reply[3] = (uint8_t)checksum;
    return size;
}
