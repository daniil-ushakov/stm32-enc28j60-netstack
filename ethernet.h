// Работа с Ethernet II: формирование, определение типа и обработка кадров.
#ifndef ETHERNET_H
#define ETHERNET_H
#include <stdint.h>
#define ETH_HEADER_SIZE 14U
#define ETH_TYPE_IPV4 0x0800U
#define ETH_TYPE_ARP  0x0806U
uint8_t ETH_GetType(const uint8_t *frame, uint16_t size, uint16_t *type);
uint16_t ETH_CreateFrame(const uint8_t *destMac, const uint8_t *sourceMac,
                         uint16_t etherType, const uint8_t *payload,
                         uint16_t payloadSize, uint8_t *frame, uint16_t capacity);
uint16_t ETH_ProcessFrame(const uint8_t *rxFrame, uint16_t rxSize,
                          const uint8_t *localMac, const uint8_t *localIp,
                          uint8_t *txFrame, uint16_t txCapacity);
#endif
