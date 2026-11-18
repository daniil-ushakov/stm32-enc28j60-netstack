// Отправка и извлечение UDP-данных без больших промежуточных буферов.
#include "net.h"
#include "ethernet.h"
#include "ip.h"
#include "udp.h"
#include <string.h>

ENC28J60_TxResult NET_SendUDP(const uint8_t *sourceMac, const uint8_t *sourceIp,
                              const uint8_t *destMac, const uint8_t *destIp,
                              uint16_t sourcePort, uint16_t destPort,
                              const uint8_t *data, uint16_t dataSize,
                              uint8_t *frame, uint16_t capacity)
{
    static uint16_t packetId = 0;
    uint16_t udpLength, ipLength, frameLength;
    if (sourceMac == 0 || sourceIp == 0 || destMac == 0 || destIp == 0 ||
        frame == 0 || (dataSize != 0 && data == 0) ||
        capacity < ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE ||
        dataSize > 1472U ||
        dataSize > (uint16_t)(capacity - ETH_HEADER_SIZE -
                              IP_HEADER_SIZE - UDP_HEADER_SIZE))
        return ENC28J60_TX_INVALID_FRAME;

    // UDP размещается сразу после заголовков Ethernet и IPv4.
    udpLength = UDP_CreatePacket(sourceIp, destIp, sourcePort, destPort,
                                 data, dataSize,
                                 frame + ETH_HEADER_SIZE + IP_HEADER_SIZE,
                                 (uint16_t)(capacity - ETH_HEADER_SIZE - IP_HEADER_SIZE));
    if (udpLength == 0) return ENC28J60_TX_INVALID_FRAME;
    ipLength = IP_CreatePacket(sourceIp, destIp, IP_PROTOCOL_UDP,
                               frame + ETH_HEADER_SIZE + IP_HEADER_SIZE,
                               udpLength, packetId++,
                               frame + ETH_HEADER_SIZE,
                               (uint16_t)(capacity - ETH_HEADER_SIZE));
    if (ipLength == 0) return ENC28J60_TX_INVALID_FRAME;
    frameLength = ETH_CreateFrame(destMac, sourceMac, ETH_TYPE_IPV4,
                                  frame + ETH_HEADER_SIZE, ipLength,
                                  frame, capacity);
    if (frameLength == 0) return ENC28J60_TX_INVALID_FRAME;
    return ENC28J60_TransmitFrame(frame, frameLength);
}

uint8_t NET_ReceiveUDP(const uint8_t *frame, uint16_t frameSize,
                       const uint8_t *localIp, uint16_t destPort,
                       uint8_t *data, uint16_t capacity,
                       uint16_t *dataSize)
{
    uint16_t etherType;
    IP_PacketInfo ipInfo;
    UDP_PacketInfo udpInfo;
    const uint8_t *udp;
    if (dataSize != 0) *dataSize = 0;
    if (frame == 0 || localIp == 0 || data == 0 || dataSize == 0 ||
        !ETH_GetType(frame, frameSize, &etherType) ||
        etherType != ETH_TYPE_IPV4) return 0;
    if (!IP_ParsePacket(frame + ETH_HEADER_SIZE,
                        (uint16_t)(frameSize - ETH_HEADER_SIZE),
                        localIp, &ipInfo) || ipInfo.protocol != IP_PROTOCOL_UDP)
        return 0;
    udp = ipInfo.data;
    if (!UDP_ParseDatagram(udp, ipInfo.dataLength, &udpInfo) ||
        udpInfo.length != ipInfo.dataLength || udpInfo.destPort != destPort ||
        udpInfo.dataLength > capacity) return 0;
    if (udpInfo.checksum != 0 &&
        UDP_CalcChecksum(ipInfo.sourceIp, ipInfo.destinationIp,
                         udp, udpInfo.length) != 0) return 0;
    if (udpInfo.dataLength != 0) memcpy(data, udpInfo.data, udpInfo.dataLength);
    *dataSize = udpInfo.dataLength;
    return 1;
}
