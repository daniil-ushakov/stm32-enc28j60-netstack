// ICMP: отвечаем на ping, сохраняя identifier, sequence и данные.
#include "icmp.h"
#include "ip.h"
#include <string.h>

// Контрольная сумма ICMP рассчитывается так же, как IPv4.
uint16_t ICMP_CalcChecksum(const uint8_t *data, uint16_t size)
{
    return IP_CalcChecksum(data, size);
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
