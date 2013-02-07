% Example Protocol

Each command, event or data packet is prefixed by a single octet that determines the type of packet being sent.

  Packet Type    Packet Indicator
 -------------- ------------------
     Command           0x01
     Data              0x02
     Event             0x04
     
The packet format of the different types of packets are then as defined by Bluetooth Core Specification v4.0, Volume 2, Part E, Section 5.4.

### Command

  Field                      Offset      Description
  -----------------  -------------- --------------------------
  Opcode                          0 The opcode of the command
  Parameter Length                2 The length of the parameters
  Parameters                 3 to n The command parameters
  
### Data

  Field                             Offset      Description
  ------------------------ -------------- --------------------------
  Handle (12 bits)                      0 The handle identifying the data
  Boundary Flag (4 bits)                0 Start or Continuation Flags
  Data Length                           2 The length of the data
  Data                                  4 to n The data
  
### Event

  Field                      Offset      Description
  -----------------  -------------- --------------------------
  Event Code                      0 The event code of the event
  Parameter Length                1 The length of the parameters
  Parameters                 2 to n The event parameters
  