# stm32-enc28j60-netstack

Компактный сетевой стек Ethernet II / ARP / IPv4 / ICMP / UDP для STM32F103C8T6 с ENC28J60 (STM32 HAL).

Поддерживается произвольная длина полезных данных UDP от **0 до 1472 байт** без IP-фрагментации. Размер задаётся параметром `dataSize` при отправке, а при приёме определяется по UDP-заголовку.

## Работа с UDP

- `NET_SendUDP(..., data, dataSize, frame, capacity)` формирует и отправляет датаграмму без промежуточных больших буферов.
- `NET_ParseUDP(frame, frameSize, localIp, destPort, &info)` возвращает данные **без копирования**. Поля `info.data`, `info.dataSize`, `info.sourcePort`, `info.sourceMac`, `info.sourceIp` описывают сообщение.
- `NET_ReceiveUDP(frame, frameSize, localIp, destPort, buffer, capacity, &size)` копирует данные в отдельный массив, если нужно сохранить их после повторного использования `frame`.
- `ETH_ProcessFrame()` формирует ARP и ICMP Echo Reply; на UDP автоматически не отвечает. Прикладной протокол обрабатывается в `main.c` или своей библиотеке.

```c
uint8_t payload[] = {0x00, 0x01, 0xFF};
NET_SendUDP(netConfig.mac, netConfig.ip, remoteMac, remoteIp,
            33333, 33333, payload, sizeof(payload),
            frame, sizeof(frame));

NET_UDPInfo incoming;
if (NET_ParseUDP(frame, frameSize, netConfig.ip, 33333, &incoming))
{
    // Полезные данные — incoming.data, количество байт — incoming.dataSize.
    // Данные доступны до следующего изменения исходного frame.
}
```

## Экономия памяти

Можно использовать один массив `uint8_t frame[1514]` как общий RX/TX-буфер. Для ответа ARP/ICMP разрешён вызов `ETH_ProcessFrame(frame, rxLength, localMac, localIp, frame, sizeof(frame))`: выходной кадр размещается поверх принятого. При `NET_ParseUDP()` отдельный массив для полезных данных не требуется.

Для передачи с полезными данными длиной `N` достаточно `42 + N` байт в TX-буфере. Максимальная длина кадра без FCS — 1514 байт.

## Ограничения

IPv4 без фрагментации, заголовок исходящего IPv4 без опций, отсутствие TCP/DHCP/DNS/ARP-кэша и маршрутизации. MAC-адрес получателя указывается приложением. Режим Duplex выбирается через `ENC28J60_Init(mac, duplex)`.
