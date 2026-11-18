// Локальные испытания протоколов без HAL и оборудования.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include "net.h"

static const uint8_t macA[6] = {0x02,0x17,0x22,0xED,0xA5,0x01};
static const uint8_t macB[6] = {0x02,0x17,0x22,0xED,0xA5,0x02};
static const uint8_t ipA[4] = {169,254,242,50};
static const uint8_t ipB[4] = {169,254,242,51};
static uint8_t sent[1514];
static uint16_t sentLength;

ENC28J60_TxResult ENC28J60_TransmitFrame(uint8_t *data, uint16_t size)
{
    assert(data != NULL && size <= sizeof(sent));
    memcpy(sent, data, size);
    sentLength = size;
    return ENC28J60_TX_OK;
}

static void test_arp(void)
{
    uint8_t frame[1514] = {0};
    uint8_t *arp = frame + ETH_HEADER_SIZE;
    uint16_t length;
    memset(frame, 0xFF, 6);
    memcpy(frame + 6, macB, 6);
    frame[12] = 8; frame[13] = 6;
    arp[0] = 0; arp[1] = 1; arp[2] = 8; arp[3] = 0;
    arp[4] = 6; arp[5] = 4; arp[6] = 0; arp[7] = 1;
    memcpy(arp + 8, macB, 6);
    memcpy(arp + 14, ipB, 4);
    memcpy(arp + 24, ipA, 4);

    // Проверяем формирование ответа в том же Ethernet-буфере.
    length = ETH_ProcessFrame(frame, 42, macA, ipA, frame, sizeof(frame));
    assert(length == 42);
    assert(memcmp(frame, macB, 6) == 0);
    assert(memcmp(frame + 6, macA, 6) == 0);
    assert(arp[6] == 0 && arp[7] == 2);
    assert(memcmp(arp + 8, macA, 6) == 0);
    assert(memcmp(arp + 14, ipA, 4) == 0);
    assert(memcmp(arp + 18, macB, 6) == 0);
    assert(memcmp(arp + 24, ipB, 4) == 0);
}

static void test_icmp(void)
{
    uint8_t frame[1514] = {0};
    uint8_t echo[13] = {8,0,0,0,0x12,0x34,0,1,'h','e','l','l','o'};
    uint16_t checksum, ipLength, frameLength;
    IP_PacketInfo info;
    checksum = ICMP_CalcChecksum(echo, sizeof(echo));
    echo[2] = (uint8_t)(checksum >> 8);
    echo[3] = (uint8_t)checksum;
    ipLength = IP_CreatePacket(ipB, ipA, IP_PROTOCOL_ICMP,
                               echo, sizeof(echo), 10,
                               frame + ETH_HEADER_SIZE, 1500);
    assert(ipLength != 0);
    frameLength = ETH_CreateFrame(macA, macB, ETH_TYPE_IPV4,
                                  frame + ETH_HEADER_SIZE,
                                  ipLength, frame, sizeof(frame));
    assert(frameLength == ipLength + ETH_HEADER_SIZE);

    // В ответе должны поменяться MAC/IP, тип ICMP и контрольная сумма.
    assert(ETH_ProcessFrame(frame, frameLength, macA, ipA,
                            frame, sizeof(frame)) == frameLength);
    assert(memcmp(frame, macB, 6) == 0);
    assert(memcmp(frame + 6, macA, 6) == 0);
    assert(IP_ParsePacket(frame + ETH_HEADER_SIZE,
                          frameLength - ETH_HEADER_SIZE, ipB, &info));
    assert(info.protocol == IP_PROTOCOL_ICMP && info.dataLength == sizeof(echo));
    assert(info.data[0] == 0 && info.data[1] == 0);
    assert(ICMP_CalcChecksum(info.data, info.dataLength) == 0);
    assert(memcmp(info.data + 8, echo + 8, sizeof(echo) - 8) == 0);
}

static void test_udp(void)
{
    static uint8_t frame[1514], payload[1473], copy[1472];
    const uint16_t sizes[] = {0,1,4,40,127,256,512,1472};
    NET_UDPInfo info;
    uint16_t i, j, received;
    for (i = 0; i < sizeof(payload); ++i)
        payload[i] = (uint8_t)(i ^ (i >> 3));

    for (j = 0; j < sizeof(sizes)/sizeof(sizes[0]); ++j)
    {
        uint16_t length = sizes[j];
        assert(NET_SendUDP(macA, ipA, macB, ipB, 12345, 33333,
                           payload, length, frame, sizeof(frame)) == ENC28J60_TX_OK);
        assert(sentLength == (uint16_t)(NET_UDP_FRAME_HEADER + length));
        assert(NET_ParseUDP(sent, sentLength, ipB, 33333, &info));
        assert(info.dataSize == length && info.sourcePort == 12345);
        assert(info.data == sent + NET_UDP_FRAME_HEADER);
        assert(memcmp(info.sourceMac, macA, 6) == 0);
        assert(memcmp(info.sourceIp, ipA, 4) == 0);
        assert(memcmp(info.data, payload, length) == 0);
        assert(NET_ReceiveUDP(sent, sentLength, ipB, 33333,
                              copy, sizeof(copy), &received));
        assert(received == length && memcmp(copy, payload, length) == 0);

        // Неправильный порт, усечённый пакет и малый буфер отклоняются.
        assert(!NET_ParseUDP(sent, sentLength, ipB, 33334, &info));
        assert(!NET_ParseUDP(sent, sentLength - 1, ipB, 33333, &info));
        if (length)
            assert(!NET_ReceiveUDP(sent, sentLength, ipB, 33333,
                                   copy, length - 1, &received));

        // Автоматического UDP-ответа в универсальном стеке нет.
        assert(ETH_ProcessFrame(sent, sentLength, macB, ipB,
                                frame, sizeof(frame)) == 0);
        if (length)
        {
            sent[NET_UDP_FRAME_HEADER] ^= 1;
            assert(!NET_ParseUDP(sent, sentLength, ipB, 33333, &info));
            sent[NET_UDP_FRAME_HEADER] ^= 1;
        }
    }
    assert(NET_SendUDP(macA, ipA, macB, ipB, 12345, 33333,
                       payload, 1473, frame, sizeof(frame)) ==
           ENC28J60_TX_INVALID_FRAME);
    assert(NET_SendUDP(macA, ipA, macB, ipB, 12345, 33333,
                       NULL, 0, frame, sizeof(frame)) == ENC28J60_TX_OK);
    assert(NET_ReceiveUDP(sent, sentLength, ipB, 33333,
                          NULL, 0, &received) && received == 0);
}

int main(void)
{
    test_arp();
    test_icmp();
    test_udp();
    puts("Проверены ARP, ICMP, UDP, контрольные суммы и размеры пакетов.");
    return 0;
}
