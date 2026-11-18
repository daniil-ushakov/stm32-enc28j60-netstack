// Ethernet II: разбор кадра и маршрутизация в ARP/IPv4.
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include <string.h>

uint8_t ETH_GetType(const uint8_t *frame, uint16_t size, uint16_t *type)
{
    if (frame == 0 || type == 0 || size < ETH_HEADER_SIZE) return 0;
    *type = (uint16_t)(((uint16_t)frame[12] << 8) | frame[13]);
    return 1;
}

uint16_t ETH_CreateFrame(const uint8_t *destMac, const uint8_t *sourceMac,
                         uint16_t etherType, const uint8_t *payload,
                         uint16_t payloadSize, uint8_t *frame, uint16_t capacity)
{
    uint16_t length = (uint16_t)(ETH_HEADER_SIZE + payloadSize);
    if (destMac == 0 || sourceMac == 0 || frame == 0 ||
        (payloadSize != 0 && payload == 0) || payloadSize > 1500U ||
        capacity < ETH_HEADER_SIZE || length > capacity) return 0;
    if (payloadSize != 0) memmove(frame + ETH_HEADER_SIZE, payload, payloadSize);
    memcpy(frame, destMac, 6);
    memcpy(frame + 6, sourceMac, 6);
    frame[12] = (uint8_t)(etherType >> 8);
    frame[13] = (uint8_t)etherType;
    return length;
}

uint16_t ETH_ProcessFrame(const uint8_t *rxFrame, uint16_t rxSize,
                          const uint8_t *localMac, const uint8_t *localIp,
                          uint8_t *txFrame, uint16_t txCapacity)
{
    uint16_t type, length;
    static const uint8_t broadcast[6] = {255, 255, 255, 255, 255, 255};
    if (rxFrame == 0 || localMac == 0 || localIp == 0 || txFrame == 0 ||
        rxSize < ETH_HEADER_SIZE || txCapacity < ETH_HEADER_SIZE) return 0;
    if (memcmp(rxFrame, localMac, 6) != 0 &&
        memcmp(rxFrame, broadcast, 6) != 0) return 0;
    if (!ETH_GetType(rxFrame, rxSize, &type)) return 0;
    length = 0;
    if (type == ETH_TYPE_ARP)
        length = ARP_ProcessPacket(rxFrame + ETH_HEADER_SIZE,
                                   (uint16_t)(rxSize - ETH_HEADER_SIZE),
                                   localMac, localIp, txFrame + ETH_HEADER_SIZE,
                                   (uint16_t)(txCapacity - ETH_HEADER_SIZE));
    else if (type == ETH_TYPE_IPV4)
        length = IP_ProcessPacket(rxFrame + ETH_HEADER_SIZE,
                                  (uint16_t)(rxSize - ETH_HEADER_SIZE),
                                  localIp, txFrame + ETH_HEADER_SIZE,
                                  (uint16_t)(txCapacity - ETH_HEADER_SIZE));
    if (length == 0) return 0;
    return ETH_CreateFrame(rxFrame + 6, localMac, type,
                           txFrame + ETH_HEADER_SIZE, length, txFrame, txCapacity);
}
