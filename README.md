Bluetooth Low Energy Virtual Controller Server
----------------------------------------------

This project is a server that exposes virtual controllers that implement the
Bluetooth low energy specification.

The virtual controllers are instantiated when a TCP connection is established
to port 0xb1ee on the machine that runs the server.

The TCP connection implements the UART H:4 protocol over TCP where a command
is prefixed by 0x01, data is prefixed by 0x02, and events are prefixed by 0x04.

The host on the client can then send HCI commands, receive HCI events, and
once in a connection send HCI data between the multiple devices connected.
