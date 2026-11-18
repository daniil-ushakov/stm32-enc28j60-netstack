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
static uint8_t incoming[1514];
static uint16_t incomingLength;
static ENC28J60_RxResult incomingStatus = ENC28J60_RX_EMPTY;
static ENC28J60_TxResult mockTxResult = ENC28J60_TX_OK;

ENC28J60_RxResult ENC28J60_ReceiveFrame(uint8_t *buffer,
                                        uint16_t capacity,
                                        uint16_t *receivedLength)
{
    ENC28J60_RxResult status = incomingStatus;
    if (buffer == NULL || receivedLength == NULL) return ENC28J60_RX_INVALID_ARG;
    *receivedLength = 0;
    incomingStatus = ENC28J60_RX_EMPTY;
    if (status != ENC28J60_RX_OK) return status;
    if (incomingLength > capacity) return ENC28J60_RX_BUFFER_SMALL;
    memcpy(buffer, incoming, incomingLength);
    *receivedLength = incomingLength;
    return ENC28J60_RX_OK;
}

ENC28J60_TxResult ENC28J60_TransmitFrame(uint8_t *data, uint16_t size)
{
    assert(data != NULL && size <= sizeof(sent));
    memcpy(sent, data, size);
    sentLength = size;
    return mockTxResult;
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


static void queue_received_frame(const uint8_t *frame, uint16_t size)
{
    assert(size <= sizeof(incoming));
    memcpy(incoming, frame, size);
    incomingLength = size;
    incomingStatus = ENC28J60_RX_OK;
}

static void test_poll(void)
{
    NET_Config local = {{0}, {0}, {255,255,255,0}, {0}};
    uint8_t frame[1514] = {0};
    uint8_t data[64] = {0};
    const uint8_t payload[5] = {0, 1, 0xAA, 0xFF, 0x42};
    uint8_t ping[9] = {8,0,0,0,0,1,0,1,0x4A};
    uint16_t length, replySize, checksum;
    IP_PacketInfo ip;

    memcpy(local.mac, macA, 6);
    memcpy(local.ip, ipA, 4);
    assert(NET_Poll(&local, 33333, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_EMPTY);
    assert(length == 0);
    assert(NET_Poll(NULL, 33333, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_INVALID_ARG);

    // UDP любой длины: заголовок и данные формируются передающим узлом.
    assert(NET_SendUDP(macB, ipB, macA, ipA, 12345, 33333,
                       payload, sizeof(payload), frame, sizeof(frame)) == ENC28J60_TX_OK);
    queue_received_frame(sent, sentLength);
    sentLength = 0;
    assert(NET_Poll(&local, 33333, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_UDP);
    assert(length == sizeof(payload) && memcmp(data, payload, length) == 0);
    assert(sentLength == 0); // На UDP не отправляем автоматический ответ.

    queue_received_frame(frame, NET_UDP_FRAME_HEADER + sizeof(payload));
    assert(NET_Poll(&local, 33333, frame, sizeof(frame),
                    data, 4, &length) == NET_POLL_UDP_TOO_LARGE);
    assert(length == 5);

    queue_received_frame(frame, NET_UDP_FRAME_HEADER + sizeof(payload));
    assert(NET_Poll(&local, 11111, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_IGNORED);
    assert(length == 0);

    // Стандартный ARP-запрос: автоматический ответ через ENC28J60.
    memset(frame, 0, 42);
    memset(frame, 0xFF, 6);
    memcpy(frame + 6, macB, 6);
    frame[12] = 8;
    frame[13] = 6;
    frame[14] = 0; frame[15] = 1;
    frame[16] = 8; frame[17] = 0;
    frame[18] = 6; frame[19] = 4;
    frame[20] = 0; frame[21] = 1;
    memcpy(frame + 22, macB, 6);
    memcpy(frame + 28, ipB, 4);
    memcpy(frame + 38, ipA, 4);
    queue_received_frame(frame, 42);
    assert(NET_Poll(&local, 33333, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_REPLY_SENT);
    assert(length == 0 && sentLength == 42);
    assert(sent[20] == 0 && sent[21] == 2); // Ответ ARP.

    // Echo Request: ICMP Echo Reply должен появиться автоматически.
    checksum = ICMP_CalcChecksum(ping, sizeof(ping));
    ping[2] = (uint8_t)(checksum >> 8);
    ping[3] = (uint8_t)checksum;
    length = IP_CreatePacket(ipB, ipA, IP_PROTOCOL_ICMP, ping, sizeof(ping),
                             7, frame + ETH_HEADER_SIZE,
                             sizeof(frame) - ETH_HEADER_SIZE);
    assert(length);
    replySize = ETH_CreateFrame(macA, macB, ETH_TYPE_IPV4,
                                frame + ETH_HEADER_SIZE, length,
                                frame, sizeof(frame));
    assert(replySize == ETH_HEADER_SIZE + length);
    queue_received_frame(frame, replySize);
    assert(NET_Poll(&local, 33333, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_REPLY_SENT);
    assert(IP_ParsePacket(sent + ETH_HEADER_SIZE, sentLength - ETH_HEADER_SIZE,
                          ipB, &ip));
    assert(ip.data[0] == 0 && ICMP_CalcChecksum(ip.data, ip.dataLength) == 0);

    // Ошибку передачи ответа сообщаем вызывающей программе.
    queue_received_frame(incoming, incomingLength);
    mockTxResult = ENC28J60_TX_ERROR;
    assert(NET_Poll(&local, 33333, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_TX_ERROR);
    mockTxResult = ENC28J60_TX_OK;

    incomingStatus = ENC28J60_RX_BAD_FRAME;
    assert(NET_Poll(&local, 33333, frame, sizeof(frame),
                    data, sizeof(data), &length) == NET_POLL_RX_ERROR);
}

int main(void)
{
    test_arp();
    test_icmp();
    test_udp();
    test_poll();
    puts("Проверены ARP, ICMP, UDP, NET_Poll и контрольные суммы.");
    return 0;
}
