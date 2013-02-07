% Sniffer Interface

# Connection

Connect to b1ee.com using port 0xb1ef

# Commands

	Opcode : Parameter

## Opcodes

	0x01 : BD_ADDR - Sniff Physical Layer

Where BD_ADDR is a 7 octet value - the most significant octet is either 0 or 1 depending on whether this is a public or random address.

If BD_ADDR is all zeros, then all Physical Layer packets are sniffed.

This means each command is a fixed size 8 octet packet.

# Data Format

	Type : Length : Data

 * Type : One Octet
 * Length : Two Octets
 * Data : Length Octets

## Types

	0x01 : Physical Layer Packet

### Physical Layer Packet

The physical layer packet contains everything from the preamble to the CRC. The format is therefore:

	link layer channel : timestamp : preamble : access address : headers : payload : crc

 * link layer channel : One Octet
 * timestamp : Eight Octets (nanoseconds)
 * preamble : One Octet
 * access address : Four Octets
 * headers : Two Octets
 * payload : (header.length) Octets
 * crc : Three Octets

