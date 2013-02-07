% B1EE

# Introduction

This project is designed to allow any Bluetooth low energy host platform to test against other Bluetooth low energy host platforms over the internet.

## Presentations

A presentation given in Barcelona, October 2012

<a href="/presentations/lowenergyclouds.pdf">![LowEnergyClouds](/presentations/lowenergyclouds.png)</a>

## Todo List

[Todo List](/todolist.html)

## Technical Details

This is done by allowing any host stack connect to a virtual controller on a server. Each host stack connection creates a separate virtual controller that can transmit and receive packets within a virtual physical channel. This allows these host stacks to advertise, scan and discover other devices, and initiate connections. Once connected, they can send L2CAP packets as required for the protocols.

See [Example Client](example_client.html)


## Specification

Simply, this server exposes a virtual controller for each TCP connection to b1ee.com on port 0xb1ee. The TCP stream simulates the data connections for a UART as defined by the Bluetooth Core Specification v4.0, Volume 4, Part A.

See [Example Protocol](example_protocol.html)

See [Example Transactions](example_transactions.html)

## Sniffer

Sniffer Interface is documented [here](sniffer.html).

## Open Source

The source code is available from a fossil server at [b1ee.com:8080](http://b1ee.com:8080/timeline). If you wish to help develop this, please contact robin at heydon dot org
