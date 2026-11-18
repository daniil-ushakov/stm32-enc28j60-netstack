// ARP: формируем ответ на запрос нашего IP-адреса.
#include "arp.h"
#include <string.h>

uint16_t ARP_ProcessPacket(const uint8_t *packet, uint16_t size,
                           const uint8_t *localMac, const uint8_t *localIp,
                           uint8_t *reply, uint16_t capacity)
{
    if (packet == 0 || localMac == 0 || localIp == 0 || reply == 0 ||
        size < ARP_PACKET_SIZE || capacity < ARP_PACKET_SIZE) return 0;
    // Ethernet (1), IPv4 (0x0800), длины MAC=6 и IP=4, операция Request (1).
    if (packet[0] != 0 || packet[1] != 1 || packet[2] != 8 ||
        packet[3] != 0 || packet[4] != 6 || packet[5] != 4 ||
        packet[6] != 0 || packet[7] != 1) return 0;
    if (memcmp(packet + 24, localIp, 4) != 0) return 0;

    reply[0] = 0; reply[1] = 1;       // Hardware type: Ethernet.
    reply[2] = 8; reply[3] = 0;       // Protocol type: IPv4.
    reply[4] = 6; reply[5] = 4;       // Размеры адресов.
    reply[6] = 0; reply[7] = 2;       // ARP Reply.
    memcpy(reply + 8, localMac, 6);  // MAC отправителя (нашей платы).
    memcpy(reply + 14, localIp, 4);  // IP отправителя (нашей платы).
    memcpy(reply + 18, packet + 8, 6);  // MAC адресата.
    memcpy(reply + 24, packet + 14, 4); // IP адресата.
    return ARP_PACKET_SIZE;
}
