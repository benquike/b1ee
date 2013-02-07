////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2013, Robin Heydon
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
////////////////////////////////////////////////////////////////////////////////

#define OGCF(ogf,ocf) ((ogf<<10)|(ocf))

////////////////////////////////////////////////////////////////////////////////
// HCI Command Opcodes

#define HCI_SET_EVENT_MASK_COMMAND                             OGCF(0x03,0x0001)
#define HCI_RESET_COMMAND                                      OGCF(0x03,0x0003)
#define HCI_WRITE_LE_HOST_SUPPORTED_COMMAND                    OGCF(0x03,0x006D)
#define HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND             OGCF(0x04,0x0001)
#define HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND              OGCF(0x04,0x0002)
#define HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND              OGCF(0x04,0x0003)
#define HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND               OGCF(0x04,0x0004)
#define HCI_READ_BUFFER_SIZE_COMMAND                           OGCF(0X04,0X0005)
#define HCI_READ_BD_ADDR_COMMAND                               OGCF(0x04,0x0009)
#define HCI_LE_SET_EVENT_MASK_COMMAND                          OGCF(0x08,0x0001)
#define HCI_LE_READ_BUFFER_SIZE_COMMAND                        OGCF(0x08,0x0002)
#define HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND           OGCF(0x08,0x0003)
#define HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND              OGCF(0x08,0x0006)
#define HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND       OGCF(0x08,0x0007)
#define HCI_LE_SET_ADVERTISING_DATA_COMMAND                    OGCF(0x08,0x0008)
#define HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND                  OGCF(0x08,0x0009)
#define HCI_LE_SET_ADVERTISE_ENABLE_COMMAND                    OGCF(0x08,0x000A)
#define HCI_LE_SET_SCAN_PARAMETERS_COMMAND                     OGCF(0x08,0x000B)
#define HCI_LE_SET_SCAN_ENABLE_COMMAND                         OGCF(0x08,0x000C)
#define HCI_LE_READ_WHITE_LIST_SIZE_COMMAND                    OGCF(0x08,0x000F)
#define HCI_LE_READ_SUPPORTED_STATES_COMMAND                   OGCF(0x08,0x001C)

////////////////////////////////////////////////////////////////////////////////
// HCI Event Codes

#define COMMAND_COMPLETE_EVENT                                              0x0E
#define COMMAND_STATUS_EVENT                                                0x0F
#define NUMBER_OF_COMPLETED_PACKETS_EVENT                                   0x13
#define LE_META_EVENT                                                       0x3E

////////////////////////////////////////////////////////////////////////////////
// HCI LE Subevent Codes

#define LE_CONNECTION_COMPLETE_EVENT                                        0x01
#define LE_ADVERTISING_REPORT_EVENT                                         0x02
#define LE_CONNECTION_UPDATE_COMPLETE_EVENT                                 0x03
#define LE_READ_REMOTE_USED_FEATURES_COMPLETE_EVENT                         0x04
#define LE_LONG_TERM_KEY_REQUEST_EVENT                                      0x05

////////////////////////////////////////////////////////////////////////////////
// HCI Error Codes

#define EC_SUCCESS                                                          0x00
#define EC_UNKNOWN_HCI_COMMAND                                              0x01
#define EC_UNKNOWN_CONNECTION_IDENTIFIER                                    0x02
#define EC_HARDWARE_FAILURE                                                 0X03
#define EC_PAGE_TIMEOUT                                                     0X04
#define EC_AUTHENTICATION_FAILURE                                           0X05
#define EC_PIN_OR_KEY_MISSING                                               0X06
#define EC_MEMORY_CAPACITY_EXCEEDED                                         0X07
#define EC_CONNECTION_TIMEOUT                                               0X08
#define EC_CONNECTION_LIMIT_EXCEEDED                                        0X09
#define EC_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED                0X0A
#define EC_ACL_CONNECTION_ALREADY_EXISTS                                    0X0B
#define EC_COMMAND_DISALLOWED                                               0X0C
#define EC_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES                     0X0D
#define EC_CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS                      0X0E
#define EC_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR                  0X0F
#define EC_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED                               0X10
#define EC_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE                           0X11
#define EC_INVALID_HCI_COMMAND_PARAMETERS                                   0x12
#define EC_REMOTE_DEVICE_TERMINATED_CONNECTION                              0X13
#define EC_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES         0X14
#define EC_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF             0X15
#define EC_CONNECTION_TERMINATED_BY_LOCAL_HOST                              0X16
#define EC_REPEATED_ATTEMPTS                                                0X17
#define EC_PAIRING_NOT_ALLOWED                                              0X18
#define EC_UNKNOWN_LMP_PDU                                                  0X19
#define EC_UNSUPPORTED_REMOTE_FEATURE_UNSUPPORTED_LMP_FEATURE               0X1A
#define EC_SCO_OFFSET_REJECTED                                              0X1B
#define EC_SCO_INTERVAL_REJECTED                                            0x1C
#define EC_SCO_AIR_MODE_REJECTED                                            0x1D
#define EC_INVALID_LMP_PARAMETERS                                           0x1E
#define EC_UNSPECIFIED_ERROR                                                0x1F
#define EC_UNSUPPORTED_LMP_PARAMETER_VALUE                                  0x20
#define EC_ROLE_CHANGE_NOT_ALLOWED                                          0x21
#define EC_LMP_RESPONSE_TIMEOUT_LL_RESPONSE_TIMEOUT                         0x22
#define EC_LMP_ERROR_TRANSACTION_COLLISION                                  0x23
#define EC_LMP_PDU_NOT_ALLOWED                                              0x24
#define EC_ENCRYPTION_MODE_NOT_ACCEPTABLE                                   0x25
#define EC_LINK_KEY_CANNOT_BE_CHANGED                                       0x26
#define EC_REQUESTED_QOS_NOT_SUPPORTED                                      0x27
#define EC_INSTANT_PASSED                                                   0x28
#define EC_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED                              0x29
#define EC_DIFFERENT_TRANSACTION_COLLISION                                  0x2A
#define EC_QOS_UNACCEPTABLE_PARAMETER                                       0x2C
#define EC_QOS_REJECTED                                                     0x2D
#define EC_CHANNEL_ASSESSMENT_NOT_SUPPORTED                                 0x2E
#define EC_INSUFFICIENT_SECURITY                                            0x2F
#define EC_PARAMETER_OUT_OF_MANDATORY_RANGE                                 0x30
#define EC_ROLE_SWITCH_PENDING                                              0x32
#define EC_RESERVED_SLOT_VIOLATION                                          0x34
#define EC_ROLE_SWITCH_FAILED                                               0x35
#define EC_EXTENDED_INQUIRY_RESPONSE_TOO_LARGE                              0x36
#define EC_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST                             0x37
#define EC_HOST_BUSY_PAIRING                                                0x38
#define EC_CONNECTION_REJECTED_DUE_TO_NO_SUITABLE_CHANNEL_FOUND             0x39
#define EC_CONTROLLER_BUSY                                                  0x3A
#define EC_UNACCEPTABLE_CONNECTION_INTERVAL                                 0x3B
#define EC_DIRECTED_ADVERTISING_TIMEOUT                                     0x3C
#define EC_CONNECTION_TERTMINATED_DUE_TO_MIC_FAILURE                        0x3D
#define EC_CONNECTION_FAILED_TO_BE_ESTABLISHED                              0x3E
#define EC_MAC_CONNECTION_FAILED                                            0x3F

////////////////////////////////////////////////////////////////////////////////
