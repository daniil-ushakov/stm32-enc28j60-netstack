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
        dataSize > NET_UDP_MAX_PAYLOAD ||
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

uint8_t NET_ParseUDP(const uint8_t *frame, uint16_t frameSize,
                     const uint8_t *localIp, uint16_t destPort,
                     NET_UDPInfo *info)
{
    uint16_t type;
    IP_PacketInfo ip;
    UDP_PacketInfo udp;
    if (frame == 0 || localIp == 0 || info == 0 ||
        !ETH_GetType(frame, frameSize, &type) || type != ETH_TYPE_IPV4)
        return 0;
    if (!IP_ParsePacket(frame + ETH_HEADER_SIZE,
                        (uint16_t)(frameSize - ETH_HEADER_SIZE),
                        localIp, &ip) || ip.protocol != IP_PROTOCOL_UDP)
        return 0;
    if (!UDP_ParseDatagram(ip.data, ip.dataLength, &udp) ||
        udp.length != ip.dataLength || udp.destPort != destPort)
        return 0;
    if (udp.checksum != 0 &&
        UDP_CalcChecksum(ip.sourceIp, ip.destinationIp, ip.data, udp.length) != 0)
        return 0;

    memcpy(info->sourceMac, frame + 6, 6);
    memcpy(info->sourceIp, ip.sourceIp, 4);
    info->sourcePort = udp.sourcePort;
    info->destPort = udp.destPort;
    info->dataSize = udp.dataLength;
    info->data = udp.data;
    return 1;
}

uint8_t NET_ReceiveUDP(const uint8_t *frame, uint16_t frameSize,
                       const uint8_t *localIp, uint16_t destPort,
                       uint8_t *data, uint16_t capacity,
                       uint16_t *dataSize)
{
    NET_UDPInfo udp;
    if (dataSize == 0) return 0;
    *dataSize = 0;
    if (!NET_ParseUDP(frame, frameSize, localIp, destPort, &udp) ||
        udp.dataSize > capacity || (udp.dataSize != 0 && data == 0))
        return 0;
    if (udp.dataSize != 0) memmove(data, udp.data, udp.dataSize);
    *dataSize = udp.dataSize;
    return 1;
}

// Обрабатывает один Ethernet-кадр без дополнительных больших буферов.
NET_PollResult NET_Poll(const NET_Config *config, uint16_t udpPort,
                        uint8_t *frame, uint16_t frameCapacity,
                        uint8_t *udpData, uint16_t udpCapacity,
                        uint16_t *udpSize)
{
    uint16_t frameSize;
    uint16_t replySize;
    NET_UDPInfo info;
    ENC28J60_RxResult rxResult;

    if (config == 0 || frame == 0 || udpSize == 0 ||
        frameCapacity < ETH_HEADER_SIZE)
        return NET_POLL_INVALID_ARG;

    *udpSize = 0;

    // Получаем один готовый Ethernet-кадр, если он есть.
    rxResult = ENC28J60_ReceiveFrame(frame, frameCapacity, &frameSize);
    if (rxResult == ENC28J60_RX_EMPTY) return NET_POLL_EMPTY;
    if (rxResult != ENC28J60_RX_OK) return NET_POLL_RX_ERROR;

    // Полезные данные UDP передаём приложению без обработки формата сообщения.
    if (NET_ParseUDP(frame, frameSize, config->ip, udpPort, &info))
    {
        *udpSize = info.dataSize;
        if (info.dataSize > udpCapacity ||
            (info.dataSize != 0 && udpData == 0))
            return NET_POLL_UDP_TOO_LARGE;

        if (info.dataSize != 0)
            memmove(udpData, info.data, info.dataSize);
        return NET_POLL_UDP;
    }

    // Для ARP-запроса или ICMP Echo Request формируем ответ в том же буфере.
    replySize = ETH_ProcessFrame(frame, frameSize,
                                 config->mac, config->ip,
                                 frame, frameCapacity);
    if (replySize == 0) return NET_POLL_IGNORED;

    if (ENC28J60_TransmitFrame(frame, replySize) != ENC28J60_TX_OK)
        return NET_POLL_TX_ERROR;

    return NET_POLL_REPLY_SENT;
}
