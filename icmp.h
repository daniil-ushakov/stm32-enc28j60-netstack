// ICMP Echo Request / Echo Reply.
#ifndef ICMP_H
#define ICMP_H
#include <stdint.h>
uint16_t ICMP_CalcChecksum(const uint8_t *data, uint16_t size);
uint16_t ICMP_ProcessPacket(const uint8_t *request, uint16_t size,
                            uint8_t *reply, uint16_t capacity);
#endif
