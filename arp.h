// ARP для Ethernet/IPv4.
#ifndef ARP_H
#define ARP_H
#include <stdint.h>
#define ARP_PACKET_SIZE 28U
uint16_t ARP_ProcessPacket(const uint8_t *packet, uint16_t size,
                           const uint8_t *localMac, const uint8_t *localIp,
                           uint8_t *reply, uint16_t capacity);
#endif
