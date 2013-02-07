"""
Copyright (c) 2012, Robin Heydon
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

################################################################################
################################################################################
################################################################################

import struct

### Packet Indicator

HCI_COMMAND = 0x01
HCI_DATA = 0x02
HCI_EVENT = 0x04


### Command OGF,OCF

def mk_ogf_ocf (ogf, ocf):
	return ((ogf << 10) & 0xFC00) | (ocf & 0x3FF)


### Command Opcodes

HCI_DISCONNECT_COMMAND = mk_ogf_ocf (0x01, 0x0006)
HCI_SET_EVENT_MASK_COMMAND = mk_ogf_ocf (0x03, 0x0001)
HCI_RESET_COMMAND = mk_ogf_ocf (0x03, 0x0003)
HCI_READ_LE_HOST_SUPPORTED_COMMAND = mk_ogf_ocf (0x03, 0x006c)
HCI_WRITE_LE_HOST_SUPPORTED_COMMAND = mk_ogf_ocf (0x03, 0x006d)
HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND = mk_ogf_ocf (0x04, 0x0001)
HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND = mk_ogf_ocf (0x04, 0x0002)
HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND = mk_ogf_ocf (0x04, 0x0003)
HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND = mk_ogf_ocf (0x04, 0x0004)
HCI_READ_BUFFER_SIZE_COMMAND = mk_ogf_ocf (0x04, 0x0005)
HCI_READ_BD_ADDR_COMMAND = mk_ogf_ocf (0x04, 0x0009)
HCI_LE_SET_EVENT_MASK_COMMAND = mk_ogf_ocf (0x08, 0x0001)
HCI_LE_READ_BUFFER_SIZE_COMMAND = mk_ogf_ocf (0x08, 0x0002)
HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND = mk_ogf_ocf (0x08, 0x0003)
HCI_LE_SET_RANDOM_ADDRESS_COMMAND = mk_ogf_ocf (0x08, 0x0005)
HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND = mk_ogf_ocf (0x08, 0x0006)
HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND = mk_ogf_ocf (0x08, 0x0007)
HCI_LE_SET_ADVERTISING_DATA_COMMAND = mk_ogf_ocf (0x08, 0x0008)
HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND = mk_ogf_ocf (0x08, 0x0009)
HCI_LE_SET_ADVERTISING_ENABLE_COMMAND = mk_ogf_ocf (0x08, 0x000A)
HCI_LE_SET_SCAN_PARAMETERS_COMMAND = mk_ogf_ocf (0x08, 0x000B)
HCI_LE_SET_SCAN_ENABLE_COMMAND = mk_ogf_ocf (0x08, 0x000C)
HCI_LE_CREATE_CONNECTION_COMMAND = mk_ogf_ocf (0x08, 0x000D)
HCI_LE_CREATE_CONNECTION_CANCEL_COMMAND = mk_ogf_ocf (0x08, 0x000E)
HCI_LE_READ_WHITE_LIST_SIZE_COMMAND = mk_ogf_ocf (0x08, 0x000F)
HCI_LE_READ_SUPPORTED_STATES_COMMAND = mk_ogf_ocf (0x08, 0x001C)


### Event Codes

HCI_DISCONNECTION_COMPLETE_EVENT = 0x05
HCI_COMMAND_COMPLETE_EVENT = 0x0E
HCI_COMMAND_STATUS_EVENT = 0x0F
HCI_HARDWARE_ERROR_EVENT = 0x10
HCI_LE_META_EVENT = 0x3E

HCI_LE_CONNECTION_COMPLETE_EVENT = 0x01
HCI_LE_ADVERTISING_REPORT_EVENT = 0x02
HCI_LE_CONNECTION_UPDATE_COMPLETE_EVENT = 0x03
HCI_LE_READ_REMOTE_USED_FEATURES_EVENT = 0x04
HCI_LE_LONG_TERM_KEY_REQUEST_EVENT = 0x05


### Event Status

EC_SUCCESS = 0x00
EC_UNKNOWN_HCI_COMMAND = 0x01
EC_UNKNOWN_CONNECTION_IDENTIFIER = 0x02
EC_REMOTE_USER_TERMINATED_CONNECTION = 0x13

################################################################################
################################################################################
################################################################################

class BD_ADDR:
	def __init__ (self, address):
		if isinstance (address, bytes):
			self.addr = struct.unpack ('>Q', b'\x00\x00' + address)[0]
		elif isinstance (address, int):
			self.addr = address
		else:
			raise (TypeError ("must be a str or int"))
	
	def asshort (self):
		return self.addr & 0xFFFF

	def asint (self):
		return self.addr
	
	def get_high (self):
		return (self.addr >> 32) & 0xFFFF
	
	def get_low (self):
		return self.addr & 0xFFFFFFFF
	
	def asbytes (self):
		return struct.pack ('>Q', self.addr)[2:]
	
	def __repr__ (self):
		return "%012x" % self.addr

################################################################################
################################################################################
################################################################################

class Packet:
	def __init__ (self, packet_indicator):
		self.packet_indicator = packet_indicator

	def encode (self, contents):
		packet = struct.pack (b'<B', self.packet_indicator) + contents
		return packet

################################################################################
################################################################################
################################################################################

def Command_Factory (opcode, contents):
	opcode_class = {
		HCI_DISCONNECT_COMMAND: Disconnect_Command,
		HCI_SET_EVENT_MASK_COMMAND: Set_Event_Mask_Command,
		HCI_RESET_COMMAND: Reset_Command,
		HCI_READ_LE_HOST_SUPPORTED_COMMAND: Read_LE_Host_Supported_Command,
		HCI_WRITE_LE_HOST_SUPPORTED_COMMAND: Write_LE_Host_Supported_Command,
		HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND: Read_Local_Version_Information_Command,
		HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND: Read_Local_Supported_Commands_Command,
		HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND: Read_Local_Supported_Features_Command,
		HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND: Read_Local_Extended_Features_Command,
		HCI_READ_BD_ADDR_COMMAND: Read_BD_ADDR_Command,
		HCI_READ_BUFFER_SIZE_COMMAND: Read_Buffer_Size_Command,
		HCI_LE_SET_EVENT_MASK_COMMAND: LE_Set_Event_Mask_Command,
		HCI_LE_READ_BUFFER_SIZE_COMMAND: LE_Read_Buffer_Size_Command,
		HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND: LE_Read_Local_Supported_Features_Command,
		HCI_LE_SET_RANDOM_ADDRESS_COMMAND: LE_Set_Random_Address_Command,
		HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND: LE_Set_Advertising_Parameters_Command,
		HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND: LE_Read_Advertising_Channel_Tx_Power_Command,
		HCI_LE_SET_ADVERTISING_DATA_COMMAND: LE_Set_Advertising_Data_Command,
		HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND: LE_Set_Scan_Response_Data_Command,
		HCI_LE_SET_ADVERTISING_ENABLE_COMMAND: LE_Set_Advertising_Enable_Command,
		HCI_LE_SET_SCAN_PARAMETERS_COMMAND: LE_Set_Scan_Parameters_Command,
		HCI_LE_SET_SCAN_ENABLE_COMMAND: LE_Set_Scan_Enable_Command,
		HCI_LE_CREATE_CONNECTION_COMMAND: LE_Create_Connection_Command,
		HCI_LE_CREATE_CONNECTION_CANCEL_COMMAND: LE_Create_Connection_Cancel_Command,
		HCI_LE_READ_WHITE_LIST_SIZE_COMMAND: LE_Read_White_List_Size_Command,
		HCI_LE_READ_SUPPORTED_STATES_COMMAND: LE_Read_Supported_States_Command
	}

	if opcode in opcode_class:
		command = opcode_class[opcode] ()
		command.decode (contents)
		return command
	
	
	return None

################################################################################

class Command (Packet):
	def __init__ (self, opcode):
		super ().__init__ (HCI_COMMAND)
		self.opcode = opcode

	def encode (self, contents = b''):
		packet = struct.pack (b'<HB', self.opcode, len (contents)) + contents
		return super ().encode (packet)

	def decode (self, contents):
		pass

################################################################################

class Disconnect_Command (Command):
	def __init__ (self, connection_handle = None, reason = None):
		super ().__init__ (HCI_DISCONNECT_COMMAND)
		self.connection_handle = connection_handle
		self.reason = reason

	def encode (self, contents = b''):
		packet = struct.pack (b'<HB', self.connection_handle, self.reason) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.connection_handle, self.reason = struct.unpack (b'<HB', contents)

	def __repr__ (self):
		return "<Disconnect_Command %03x %02x>" % (self.connection_handle, self.reason)

################################################################################

class Set_Event_Mask_Command (Command):
	def __init__ (self, event_mask = None):
		super ().__init__ (HCI_SET_EVENT_MASK_COMMAND)
		self.event_mask = event_mask

	def encode (self, contents = b''):
		packet = struct.pack (b'<Q', self.event_mask) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.event_mask = struct.unpack (b'<Q', contents)[0]

	def __repr__ (self):
		return "<Set_Event_Mask_Command %016x>" % self.event_mask

################################################################################

class Reset_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_RESET_COMMAND)

	def __repr__ (self):
		return "<Reset_Command>"

################################################################################

class Read_LE_Host_Supported_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_READ_LE_HOST_SUPPORTED_COMMAND)

	def __repr__ (self):
		return "<Read_LE_Host_Supported_Command>"

################################################################################

class Write_LE_Host_Supported_Command (Command):
	def __init__ (self, le_supported_host = 0x00, simultaneous_le_host = 0x00):
		super ().__init__ (HCI_WRITE_LE_HOST_SUPPORTED_COMMAND)
		self.le_supported_host = le_supported_host
		self.simultaneous_le_host = simultaneous_le_host

	def encode (self, contents = b''):
		packet = struct.pack (b'<BB', self.le_supported_host, self.simultaneous_le_host) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.le_supported_host, self.simultaneous_le_host = struct.unpack (b'<BB', contents)

	def __repr__ (self):
		return "<Write_LE_Host_Supported_Command %d %d>" % (self.le_supported_host, self.simultaneous_le_host)

################################################################################

class Read_Local_Version_Information_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND)
	
	def __repr__ (self):
		return "<Read_Local_Version_Information_Command>"

################################################################################

class Read_Local_Supported_Commands_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND)
	
	def __repr__ (self):
		return "<Read_Local_Supported_Commands_Command>"

################################################################################

class Read_Local_Supported_Features_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND)
	
	def __repr__ (self):
		return "<Read_Local_Supported_Features_Command>"

################################################################################

class Read_Local_Extended_Features_Command (Command):
	def __init__ (self, page_number = None):
		super ().__init__ (HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND)
		self.page_number = page_number

	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.page_number) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.page_number = struct.unpack (b'<B', contents[:1])[0]
	
	def __repr__ (self):
		return "<Read_Local_Extended_Features_Command (%d)>" % self.page_number

################################################################################

class Read_Buffer_Size_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_READ_BUFFER_SIZE_COMMAND)
	
	def __repr__ (self):
		return "<Read_Buffer_Size_Command>"

################################################################################

class Read_BD_ADDR_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_READ_BD_ADDR_COMMAND)
	
	def __repr__ (self):
		return "<Read_BD_ADDR_Command>"

################################################################################

class LE_Set_Event_Mask_Command (Command):
	def __init__ (self, event_mask = None):
		super ().__init__ (HCI_LE_SET_EVENT_MASK_COMMAND)
		self.event_mask = event_mask

	def encode (self, contents = b''):
		packet = struct.pack (b'<Q', self.event_mask) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.event_mask = struct.unpack (b'<Q', contents)[0]

	def __repr__ (self):
		return "<LE_Set_Event_Mask_Command %016x>" % self.event_mask

################################################################################

class LE_Read_Buffer_Size_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_LE_READ_BUFFER_SIZE_COMMAND)

	def __repr__ (self):
		return "<LE_Read_Buffer_Size_Command>"

################################################################################

class LE_Read_Local_Supported_Features_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND)

	def __repr__ (self):
		return "<LE_Read_Local_Supported_Features_Command>"

################################################################################

class LE_Set_Random_Address_Command (Command):
	def __init__ (self, random_addr = None):
		super ().__init__ (HCI_LE_SET_RANDOM_ADDRESS_COMMAND)
		self.random_addr = random_addr

	def encode (self, contents = b''):
		packet = struct.pack (b'<Q', self.random_addr)[:6] + contents
		return super ().encode (packet)

	def decode (self, contents):
		bd_addr = struct.unpack (b'<IH', contents)
		self.random_addr = (bd_addr[1] << 32) | bd_addr[0]

	def __repr__ (self):
		return "<LE_Set_Random_Address_Command %012x>" % self.random_addr

################################################################################

class LE_Set_Advertising_Parameters_Command (Command):
	def __init__ (self, adv_int_min = 0x0800, adv_int_max = 0x0800, adv_type = 0x00, own_addr_type = 0x00, dir_addr_type = 0x00, daddr = 0, adv_chmap = 0x07, afp = 0x00):
		super ().__init__ (HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND)
		self.adv_int_min = adv_int_min
		self.adv_int_max = adv_int_max
		self.adv_type = adv_type
		self.dir_addr_type = dir_addr_type
		self.own_addr_type = own_addr_type
		self.daddr = daddr
		self.adv_chmap = adv_chmap
		self.afp = afp

	def encode (self, contents = b''):
		packet = struct.pack (b'<HHBBBIHBB', self.adv_int_min, self.adv_int_max, self.adv_type, self.own_addr_type, self.dir_addr_type, self.daddr & 0xFFFFFFFF, self.daddr >> 32, self.adv_chmap, self.afp) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.adv_int_min, self.adv_int_max, self.adv_type, self.own_addr_type, self.dir_addr_type, daddrl, daddrh, self.adv_chmap, self.afp = struct.unpack (b'<HHBBBIHBB', contents)
		self.daddr = (daddrh << 32) | daddrl

	def __repr__ (self):
		return "<LE_Set_Advertising_Parameters_Command (%04x, %04x, %02x, %02x, %02x, %012x, %02x, %02x>" % (self.adv_int_min, self.adv_int_max, self.adv_type, self.own_addr_type, self.dir_addr_type, self.daddr, self.adv_chmap, self.afp)

################################################################################

class LE_Read_Advertising_Channel_Tx_Power_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND)

	def __repr__ (self):
		return "<LE_Read_Advertising_Channel_Tx_Power_Command>"

################################################################################

class LE_Set_Advertising_Data_Command (Command):
	def __init__ (self, adv_len = None, adv_data = None):
		super ().__init__ (HCI_LE_SET_ADVERTISING_DATA_COMMAND)
		self.adv_len = adv_len
		self.adv_data = adv_data

	def encode (self, contents = b''):
		adv_data = self.adv_data + b'\x00' * 31
		adv_data = adv_data[:31]
		packet = struct.pack (b'<B', self.adv_len) + adv_data + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.adv_len = struct.unpack (b'<B', contents[:1])[0]
		self.adv_data = contents[1:]

	def __repr__ (self):
		return "<LE_Set_Advertising_Data_Command (%d, %s)>" % (self.adv_len, self.adv_data)

################################################################################

class LE_Set_Scan_Response_Data_Command (Command):
	def __init__ (self, scr_len = None, scr_data = None):
		super ().__init__ (HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND)
		self.scr_len = scr_len
		self.scr_data = scr_data

	def encode (self, contents = b''):
		scr_data = self.scr_data + b'\x00' * 31
		scr_data = scr_data[:31]
		packet = struct.pack (b'<B', self.scr_len) + scr_data + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.scr_len = struct.unpack (b'<B', contents[:1])[0]
		self.scr_data = contents[1:]

	def __repr__ (self):
		return "<LE_Set_Scan_Response_Data_Command (%d, %s)>" % (self.scr_len, self.scr_data)

################################################################################

class LE_Set_Advertising_Enable_Command (Command):
	def __init__ (self, enable = None):
		super ().__init__ (HCI_LE_SET_ADVERTISING_ENABLE_COMMAND)
		self.enable = enable

	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.enable) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.enable = struct.unpack (b'<B', contents[:1])[0]

	def __repr__ (self):
		return "<LE_Set_Advertising_Enable_Command (%d)>" % (self.enable)

################################################################################

class LE_Set_Scan_Parameters_Command (Command):
	def __init__ (self, scan_type = 0x00, scan_int = 0x0010, scan_window = 0x0010, own_addr_type = 0x00, sfp = 0x00):
		super ().__init__ (HCI_LE_SET_SCAN_PARAMETERS_COMMAND)
		self.scan_type = scan_type
		self.scan_int = scan_int
		self.scan_window = scan_window
		self.own_addr_type = own_addr_type
		self.sfp = sfp

	def encode (self, contents = b''):
		packet = struct.pack (b'<BHHBB', self.scan_type, self.scan_int, self.scan_window, self.own_addr_type, self.sfp) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.scan_type, self.scan_int, self.scan_window, self.own_addr_type, self.sfp = struct.unpack (b'<BHHBB', contents[:7])

	def __repr__ (self):
		return "<LE_Set_Scan_Parameters_Command (%d, %d, %d, %d, %d)>" % (self.scan_type, self.scan_int, self.scan_window, self.own_addr_type, self.sfp)

################################################################################

class LE_Set_Scan_Enable_Command (Command):
	def __init__ (self, enable = True, filter_duplicates = False):
		super ().__init__ (HCI_LE_SET_SCAN_ENABLE_COMMAND)
		self.enable = enable
		self.filter_duplicates = filter_duplicates

	def encode (self, contents = b''):
		packet = struct.pack (b'<BB', self.enable, self.filter_duplicates) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.enable, self.filter_duplicates = struct.unpack (b'<BB', contents[:2])

	def __repr__ (self):
		return "<LE_Set_Scan_Enable_Command (%d, %d)>" % (self.enable, self.filter_duplicates)

################################################################################

class LE_Create_Connection_Command (Command):
	def __init__ (self, scan_int = 0x0100, scan_win = 0x0100, ifp = 0x00, peer_address_type = 0x00, peer_address = 0, own_address_type = 0x00, conn_int_min = 0x0100, conn_int_max = 0x0400, conn_latency = 0x04, supervision_timeout = 0x0C80, min_ce_len = 0x0004, max_ce_len = 0x0008):
		super ().__init__ (HCI_LE_CREATE_CONNECTION_COMMAND)
		self.scan_int = scan_int
		self.scan_win = scan_win
		self.ifp = ifp
		self.peer_address_type = peer_address_type
		self.peer_address = peer_address
		self.own_address_type = own_address_type
		self.conn_int_min = conn_int_min
		self.conn_int_max = conn_int_max
		self.conn_latency = conn_latency
		self.supervision_timeout = supervision_timeout
		self.min_ce_len = min_ce_len
		self.max_ce_len = max_ce_len

	def encode (self, contents = b''):
		packet = struct.pack (b'<HHBBIHBHHHHHH', self.scan_int, self.scan_win, self.ifp, self.peer_address_type, self.peer_address & 0xFFFFFFFF, self.peer_address >> 32, self.own_address_type, self.conn_int_min, self.conn_int_max, self.conn_latency, self.supervision_timeout, self.min_ce_len, self.max_ce_len) + contents
		return super ().encode (packet)

	def decode (self, contents):
		self.scan_int, self.scan_win, self.ifp, self.peer_address_type, addrl, addrh, self.own_address_type, self.conn_int_min, self.conn_int_max, self.conn_latency, self.supervision_timeout, self.min_ce_len, self.max_ce_len = struct.unpack (b'<HHBBIHBHHHHHH', contents)
		self.peer_address = (addrh << 32) | addrl

	def __repr__ (self):
		return "<LE_Create_Connection_Command (%d, %d, %02x, %02x, %012x, %02x, %d, %d, %d, %d, %d, %d)>" % (self.scan_int, self.scan_win, self.ifp, self.peer_address_type, self.peer_address, self.own_address_type, self.conn_int_min, self.conn_int_max, self.conn_latency, self.supervision_timeout, self.min_ce_len, self.max_ce_len)

################################################################################

class LE_Create_Connection_Cancel_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_LE_CREATE_CONNECTION_CANCEL_COMMAND)

	def __repr__ (self):
		return "<LE_Create_Connection_Cancel_Command ()>"

################################################################################

class LE_Read_White_List_Size_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_LE_READ_WHITE_LIST_SIZE_COMMAND)

	def __repr__ (self):
		return "<LE_Read_White_List_Size_Command ()>"

################################################################################

class LE_Read_Supported_States_Command (Command):
	def __init__ (self):
		super ().__init__ (HCI_LE_READ_SUPPORTED_STATES_COMMAND)

	def __repr__ (self):
		return "<LE_Read_Supported_States_Command ()>"

################################################################################
################################################################################
################################################################################

def Event_Factory (event_code, contents):
	event_code_class = {
		HCI_DISCONNECTION_COMPLETE_EVENT: Disconnection_Complete_Event,
		HCI_COMMAND_STATUS_EVENT: Command_Status_Event,
		HCI_HARDWARE_ERROR_EVENT: Hardware_Error_Event,
	}

	if event_code in event_code_class:
		event = event_code_class[event_code] ()
		event.decode (contents)
		return event
	
	if event_code == HCI_COMMAND_COMPLETE_EVENT:
		command_opcode = struct.unpack ('<H', contents[1:3])[0]

		opcode_class = {
			HCI_SET_EVENT_MASK_COMMAND: Command_Complete_Set_Event_Mask_Event,
			HCI_RESET_COMMAND: Command_Complete_Reset_Event,
			HCI_READ_LE_HOST_SUPPORTED_COMMAND: Command_Complete_Read_LE_Host_Supported_Event,
			HCI_WRITE_LE_HOST_SUPPORTED_COMMAND: Command_Complete_Write_LE_Host_Supported_Event,
			HCI_READ_BUFFER_SIZE_COMMAND: Command_Complete_Read_Buffer_Size_Event,
			HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND: Command_Complete_Read_Local_Version_Information_Event,
			HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND: Command_Complete_Read_Local_Supported_Commands_Event,
			HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND: Command_Complete_Read_Local_Supported_Features_Event,
			HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND: Command_Complete_Read_Local_Extended_Features_Event,
			HCI_READ_BD_ADDR_COMMAND: Command_Complete_Read_BD_ADDR_Event,
			HCI_LE_SET_EVENT_MASK_COMMAND: Command_Complete_LE_Set_Event_Mask_Event,
			HCI_LE_READ_BUFFER_SIZE_COMMAND: Command_Complete_LE_Read_Buffer_Size_Event,
			HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND: Command_Complete_LE_Read_Local_Supported_Features_Event,
			HCI_LE_SET_RANDOM_ADDRESS_COMMAND: Command_Complete_LE_Set_Random_Address_Event,
			HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND: Command_Complete_LE_Set_Advertising_Parameters_Event,
			HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND: Command_Complete_LE_Read_Advertising_Channel_Tx_Power_Event,
			HCI_LE_SET_ADVERTISING_DATA_COMMAND: Command_Complete_LE_Set_Advertising_Data_Event,
			HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND: Command_Complete_LE_Set_Scan_Response_Data_Event,
			HCI_LE_SET_ADVERTISING_ENABLE_COMMAND: Command_Complete_LE_Set_Advertising_Enable_Event,
			HCI_LE_SET_SCAN_PARAMETERS_COMMAND: Command_Complete_LE_Set_Scan_Parameters_Event,
			HCI_LE_SET_SCAN_ENABLE_COMMAND: Command_Complete_LE_Set_Scan_Enable_Event,
			HCI_LE_CREATE_CONNECTION_CANCEL_COMMAND: Command_Complete_LE_Create_Connection_Cancel_Event,
			HCI_LE_READ_WHITE_LIST_SIZE_COMMAND: Command_Complete_LE_Read_White_List_Size_Event,
			HCI_LE_READ_SUPPORTED_STATES_COMMAND: Command_Complete_LE_Read_Supported_States_Event,
		}

		if command_opcode in opcode_class:
			event = opcode_class[command_opcode] ()
			event.decode (contents)
			return event
	
	if event_code == HCI_LE_META_EVENT:
		subevent_code = struct.unpack ('<B', contents[0:1])[0]

		le_event_class = {
			HCI_LE_CONNECTION_COMPLETE_EVENT: LE_Connection_Complete_Event,
			HCI_LE_ADVERTISING_REPORT_EVENT: LE_Advertising_Report_Event,
		}

		if subevent_code in le_event_class:
			event = le_event_class[subevent_code] ()
			event.decode (contents)
			return event
		
	return None

################################################################################

class Event (Packet):
	def __init__ (self, event_code = None):
		super ().__init__ (HCI_EVENT)
		self.event_code = event_code
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BB', self.event_code, len (contents)) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		pass
	
################################################################################

class Disconnection_Complete_Event (Event):
	def __init__ (self, status = None, connection_handle = None, reason = None):
		super ().__init__ (event_code = HCI_DISCONNECTION_COMPLETE_EVENT)
		self.status = status
		self.connection_handle = connection_handle
		self.reason = reason

	def encode (self, contents = b''):
		packet = struct.pack (b'<BHB', self.status, self.connection_handle, self.reason) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status, self.connection_handle, self.reason = struct.unpack (b'<BHB', contents)

	def __repr__ (self):
		return "<Disconnection Complete Event (status=%02x, connection_handle=%03x, reason=%02x)>" % (self.status, self.connection_handle, self.reason)

################################################################################

class Command_Status_Event (Event):
	def __init__ (self, status = None, opcode = None):
		super ().__init__ (event_code = HCI_COMMAND_STATUS_EVENT)
		self.status = status
		self.num_hci_commands = 1
		self.command_opcode = opcode

	def encode (self, contents = b''):
		packet = struct.pack (b'<BBH', self.status, self.num_hci_commands, self.command_opcode) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status, self.num_hci_commands, self.command_opcode = struct.unpack (b'<BBH', contents)

	def __repr__ (self):
		return "<Command Status Event (status=%02x, numhci=%02x, opcode=%04x)>" % (self.status, self.num_hci_commands, self.command_opcode)
	
################################################################################

class Command_Complete_Event (Event):
	def __init__ (self, command_opcode = None):
		super ().__init__ (event_code = HCI_COMMAND_COMPLETE_EVENT)
		self.command_opcode = command_opcode
		self.num_hci_commands = 1
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BH', self.num_hci_commands, self.command_opcode) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.num_hci_commands, self.command_opcode = struct.unpack (b'<BH', contents[0:3])
	
	def __repr__ (self):
		return "<Command Complete Event (numhci=%02x, opcode=%04x)>" % (self.num_hci_commands, self.command_opcode)
	
################################################################################

class Command_Complete_Status_Event (Command_Complete_Event):
	def __init__ (self, command_opcode = None, status = None):
		super ().__init__ (command_opcode = command_opcode)
		self.status = status
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.status) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status = struct.unpack (b'<B', contents[3:4])[0]
	
	def __repr__ (self):
		return "<Command Complete Status Event (numhci=%02x, opcode=%04x, status=%02x)>" % (self.num_hci_commands, self.command_opcode, self.status)
	
################################################################################

class Command_Complete_Set_Event_Mask_Event (Command_Complete_Status_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_SET_EVENT_MASK_COMMAND, status = status)
	
	def __repr__ (self):
		return "<Command Complete Event (Set_Event_Mask, status=%02x)>" % self.status

################################################################################

class Command_Complete_Reset_Event (Command_Complete_Status_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_RESET_COMMAND, status = status)
	
	def __repr__ (self):
		return "<Command Complete Event (Reset, status=%02x)>" % self.status

################################################################################

class Command_Complete_Read_LE_Host_Supported_Event (Command_Complete_Event):
	def __init__ (self, status = None, le_supported_host = None, simultaneous_le_host = None):
		super ().__init__ (command_opcode = HCI_READ_LE_HOST_SUPPORTED_COMMAND)
		self.status = status
		self.le_supported_host = le_supported_host
		self.simultaneous_le_host = simultaneous_le_host

	def encode (self, contents = b''):
		packet = struct.pack (b'<BBB', self.status, self.le_supported_host, self.simultaneous_le_host) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status, self.le_supported_host, self.simultaneous_le_host = struct.unpack (b'<BBB', contents[3:6])

	def __repr__ (self):
		return "<Command Complete Event (Read_LE_Host_Supported, status=%02x, le_supported_host=%02x, simultaneous_le_host=%02x)>" % (self.status, self.le_supported_host, self.simultaneous_le_host)

################################################################################

class Command_Complete_Write_LE_Host_Supported_Event (Command_Complete_Status_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_WRITE_LE_HOST_SUPPORTED_COMMAND, status = status)
	
	def __repr__ (self):
		return "<Command Complete Event (Write_LE_Host_Supported, status=%02x)>" % self.status

################################################################################

class Command_Complete_Read_Local_Version_Information_Event (Command_Complete_Event):
	def __init__ (self, status = None, hci_version = None, hci_revision = None, lmp_version = None, manufacturer_name = None, lmp_subversion = None):
		super ().__init__ (command_opcode = HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND)
		self.status = status
		self.hci_version = hci_version
		self.hci_revision = hci_revision
		self.lmp_version = lmp_version
		self.manufacturer_name = manufacturer_name
		self.lmp_subversion = lmp_subversion
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BBHBHH', self.status, self.hci_version, self.hci_revision, self.lmp_version, self.manufacturer_name, self.lmp_subversion) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status, self.hci_version, self.hci_revision, self.lmp_version, self.manufacturer_name, self.lmp_subversion = struct.unpack (b'<BBHBHH', contents[3:12])

	def __repr__ (self):
		return "<Command Complete Event (Read_Local_Version_Information, status=%02x, %02x, %04x, %02x, %04x, %04x)>" % (self.status, self.hci_version, self.hci_revision, self.lmp_version, self.manufacturer_name, self.lmp_subversion)

################################################################################

class Command_Complete_Read_Local_Supported_Commands_Event (Command_Complete_Event):
	def __init__ (self, status = None, supported_commands = None):
		super ().__init__ (command_opcode = HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND)
		self.status = status
		self.supported_commands = supported_commands
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.status) + self.supported_commands[0:64] + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status = struct.unpack (b'<B', contents[3:4])
		self.supported_commands = contents[4:68]

	def __repr__ (self):
		response = "<Command Complete Event (Read_Local_Supported_Commands, status=%02x, supported_commands=" % self.status
		for o in self.supported_commands:
			response += "%02x" % o
		response += ")>"
		return response

################################################################################

class Command_Complete_Read_Local_Supported_Features_Event (Command_Complete_Event):
	def __init__ (self, status = None, features = None):
		super ().__init__ (command_opcode = HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND)
		self.status = status
		self.features = features
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BQ', self.status, self.features) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status, self.features = struct.unpack (b'<BQ', contents[3:12])

	def __repr__ (self):
		return "<Command Complete Event (Read_Local_Supported_Features, status=%02x, features=%016x)>" % (self.status, self.features)

################################################################################

class Command_Complete_Read_Local_Extended_Features_Event (Command_Complete_Event):
	def __init__ (self, status = None, page_number = None, max_page_number = None, features = None):
		super ().__init__ (command_opcode = HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND)
		self.status = status
		self.page_number = page_number
		self.max_page_number = max_page_number
		self.features = features
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BBBQ', self.status, self.page_number, self.max_page_number, self.features) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status, self.page_number, self.max_page_number, self.features = struct.unpack (b'<BBBQ', contents[3:14])

	def __repr__ (self):
		return "<Command Complete Event (Read_Local_Extended_Features, status=%02x, page_number=%02x, max_page_number=%02x, features=%016x)>" % (self.status, self.page_number, self.max_page_number, self.features)

################################################################################

class Command_Complete_Read_Buffer_Size_Event (Command_Complete_Event):
	def __init__ (self, status = None, acl_length = None, sco_length = None, acl_num = None, sco_num = None):
		super ().__init__ (command_opcode = HCI_READ_BUFFER_SIZE_COMMAND)
		self.status = status
		self.acl_length = acl_length
		self.sco_length = sco_length
		self.acl_num = acl_num
		self.sco_num = sco_num
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BHBHH', self.status, self.acl_length, self.sco_length, self.acl_num, self.sco_num) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status, self.acl_length, self.sco_length, self.acl_num, self.sco_num = struct.unpack (b'<BHBHH', contents[3:11])

	def __repr__ (self):
		return "<Command Complete Event (Read_Buffer_Size, status=%02x, acl=%dx%d sco=%dx%d)>" % (self.status, self.acl_num, self.acl_length, self.sco_num, self.acl_length)

################################################################################

class Command_Complete_Read_BD_ADDR_Event (Command_Complete_Event):
	def __init__ (self, status = None, bd_addr = None):
		super ().__init__ (command_opcode = HCI_READ_BD_ADDR_COMMAND)
		self.status = status
		self.bd_addr = bd_addr
	
	def encode (self, contents = b''):
		bd_addr = struct.pack (b'<IH', self.bd_addr & 0xFFFFFFFF, self.bd_addr >> 32)[0:6]
		packet = struct.pack (b'<B', self.status) + bd_addr + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status = struct.unpack (b'<B', contents[3:4])[0]
		bd_addr = struct.unpack (b'<IH', contents[4:10])
		self.bd_addr = (bd_addr[1] << 32) | bd_addr[0]

	def __repr__ (self):
		return "<Command Complete Event (Read_BD_ADDR, status=%02x, bd_addr=%012x)>" % (self.status, self.bd_addr)

################################################################################

class Command_Complete_LE_Set_Event_Mask_Event (Command_Complete_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_EVENT_MASK_COMMAND)
		self.status = status
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.status) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status = struct.unpack (b'<B', contents[3:4])[0]

	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Event_Mask, status=%02x)>" % self.status

################################################################################

class Command_Complete_LE_Read_Buffer_Size_Event (Command_Complete_Event):
	def __init__ (self, status = None, le_length = None, le_num = None):
		super ().__init__ (command_opcode = HCI_LE_READ_BUFFER_SIZE_COMMAND)
		self.status = status
		self.le_length = le_length
		self.le_num = le_num
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BHB', self.status, self.le_length, self.le_num) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status, self.le_length, self.le_num = struct.unpack (b'<BHB', contents[3:7])

	def __repr__ (self):
		return "<Command Complete Event (LE_Read_Buffer_Size, status=%02x, le=%dx%d)>" % (self.status, self.le_num, self.le_length)

################################################################################

class Command_Complete_LE_Read_Local_Supported_Features_Event (Command_Complete_Event):
	def __init__ (self, status = None, le_features = None):
		super ().__init__ (command_opcode = HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND)
		self.status = status
		self.le_features = le_features
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BQ', self.status, self.le_features) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status, self.le_features = struct.unpack (b'<BQ', contents[3:12])

	def __repr__ (self):
		return "<Command Complete Event (LE_Read_Local_Supported_Features, status=%02x, le_features=%016x)>" % (self.status, self.le_features)

################################################################################

class Command_Complete_LE_Set_Random_Address_Event (Command_Complete_Status_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_RANDOM_ADDRESS_COMMAND, status = status)
	
	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Random_Address, status=%02x)>" % self.status

################################################################################

class Command_Complete_LE_Set_Advertising_Parameters_Event (Command_Complete_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND)
		self.status = status
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.status) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status = struct.unpack (b'<B', contents[3:4])[0]

	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Advertising_Parameters, status=%02x)>" % self.status

################################################################################

class Command_Complete_LE_Read_Advertising_Channel_Tx_Power_Event (Command_Complete_Event):
	def __init__ (self, status = None, transmit_power_level = None):
		super ().__init__ (command_opcode = HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND)
		self.status = status
		self.transmit_power_level = transmit_power_level
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<Bb', self.status, self.transmit_power_level) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status, self.transmit_power_level = struct.unpack (b'<Bb', contents[3:5])

	def __repr__ (self):
		return "<Command Complete Event (LE_Read_Advertising_Channel_Tx_Power, status=%02x, tx=%d)>" % (self.status, self.transmit_power_level)

################################################################################

class Command_Complete_LE_Set_Advertising_Data_Event (Command_Complete_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_ADVERTISING_DATA_COMMAND)
		self.status = status
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.status) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status = struct.unpack (b'<B', contents[3:4])[0]

	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Advertising_Data, status=%02x)>" % (self.status)

################################################################################

class Command_Complete_LE_Set_Scan_Response_Data_Event (Command_Complete_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND)
		self.status = status
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.status) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status = struct.unpack (b'<B', contents[3:4])[0]

	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Scan_Response_Data, status=%02x)>" % (self.status)

################################################################################

class Command_Complete_LE_Set_Advertising_Enable_Event (Command_Complete_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_ADVERTISING_ENABLE_COMMAND)
		self.status = status
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.status) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status = struct.unpack (b'<B', contents[3:4])[0]

	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Advertising_Enable, status=%02x)>" % (self.status)

################################################################################

class Command_Complete_LE_Set_Scan_Parameters_Event (Command_Complete_Status_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_SCAN_PARAMETERS_COMMAND, status = status)
	
	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Scan_Parameters, status=%02x)>" % self.status

################################################################################

class Command_Complete_LE_Set_Scan_Enable_Event (Command_Complete_Status_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_SET_SCAN_ENABLE_COMMAND, status = status)
	
	def __repr__ (self):
		return "<Command Complete Event (LE_Set_Scan_Enable, status=%02x)>" % self.status

################################################################################

class Command_Complete_LE_Create_Connection_Cancel_Event (Command_Complete_Status_Event):
	def __init__ (self, status = None):
		super ().__init__ (command_opcode = HCI_LE_CREATE_CONNECTION_CANCEL_COMMAND, status = status)
	
	def __repr__ (self):
		return "<Command Complete Event (LE_Create_Connection_Cancel, status=%02x)>" % self.status

################################################################################

class Command_Complete_LE_Read_White_List_Size_Event (Command_Complete_Event):
	def __init__ (self, status = None, white_list_size = None):
		super ().__init__ (command_opcode = HCI_LE_READ_WHITE_LIST_SIZE_COMMAND)
		self.status = status
		self.white_list_size = white_list_size
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BB', self.status, self.white_list_size) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status, self.white_list_size = struct.unpack (b'<BB', contents[3:5])

	def __repr__ (self):
		return "<Command Complete Event (LE_Read_White_List_Size, status=%02x, white_list_size=%d)>" % (self.status, self.white_list_size)

################################################################################

class Command_Complete_LE_Read_Supported_States_Event (Command_Complete_Event):
	def __init__ (self, status = None, supported_states = None):
		super ().__init__ (command_opcode = HCI_LE_READ_SUPPORTED_STATES_COMMAND)
		self.status = status
		self.supported_states = supported_states
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<BQ', self.status, self.supported_states) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		self.status, self.supported_states = struct.unpack (b'<BQ', contents[3:12])

	def __repr__ (self):
		response = "<Command Complete Event (LE_Read_Local_Supported_Commands, status=%02x, supported_commands=%016x)>" % (self.status, self.supported_states)
		return response

################################################################################

class Hardware_Error_Event (Event):
	def __init__ (self, hardware_code = None):
		super ().__init__ (event_code = HCI_HARDWARE_ERROR_EVENT)
		self.hardware_code = hardware_code
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.hardware_code) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.hardware_code = struct.unpack (b'<B', contents)[0]

	def __repr__ (self):
		return "<Hardware Error Event (code=%d)>" % self.hardware_code

################################################################################

class LE_Meta_Event (Event):
	def __init__ (self, subevent_code = None):
		super ().__init__ (event_code = HCI_LE_META_EVENT)
		self.subevent_code = subevent_code
	
	def encode (self, contents = b''):
		packet = struct.pack (b'<B', self.subevent_code) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.subevent_code = struct.unpack (b'<B', contents[0:1])[0]

	def __repr__ (self):
		return "<LE Meta Event (subevent_code=%d)>" % self.subevent_code

################################################################################

class LE_Connection_Complete_Event (LE_Meta_Event):
	def __init__ (self, status = 0, connection_handle = 0, role = 0, peer_address_type = 0, peer_address = 0, conn_interval = 0, conn_latency = 0, supervision_timeout = 0, mca = 0):
		super ().__init__ (subevent_code = HCI_LE_CONNECTION_COMPLETE_EVENT)
		self.status = status
		self.connection_handle = connection_handle
		self.role = role
		self.peer_address_type = peer_address_type
		self.peer_address = peer_address
		self.conn_interval = conn_interval
		self.conn_latency = conn_latency
		self.supervision_timeout = supervision_timeout
		self.mca = mca
	
	def encode (self, contents = b''):
		addrh = (self.peer_address >> 32) & 0xFFFF
		addrl = self.peer_address & 0xFFFFFFFF
		packet = struct.pack (b'<BHBBIHHHHB', self.status, self.connection_handle, self.role, self.peer_address_type, addrl, addrh, self.conn_interval, self.conn_latency, self.supervision_timeout, self.mca) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.status, self.connection_handle, self.role, self.peer_address_type, addrl, addrh, self.conn_interval, self.conn_latency, self.supervision_timeout, self.mca = struct.unpack (b'<BHBBIHHHHB', contents[1:])
		self.peer_address = (addrh << 32) | addrl

	def __repr__ (self):
		return "<LE Connection Complete Event (%02x, %03x, %02x, %02x, %012x, %d, %d, %d, %02x)>" % (self.status, self.connection_handle, self.role, self.peer_address_type, self.peer_address, self.conn_interval, self.conn_latency, self.supervision_timeout, self.mca)

################################################################################

class LE_Advertising_Report_Event (LE_Meta_Event):
	def __init__ (self, num_reports = 1, event_type = None, address_type = None, address = None, len_data = None, data = None, rssi = None):
		super ().__init__ (subevent_code = HCI_LE_ADVERTISING_REPORT_EVENT)
		self.num_reports = num_reports
		self.event_type = event_type
		self.address_type = address_type
		self.address = address
		self.len_data = len_data
		self.data = data
		self.rssi = rssi
	
	def encode (self, contents = b''):
		addrh = (self.address >> 32) & 0xFFFF
		addrl = self.address & 0xFFFFFFFF
		packet = struct.pack (b'<BBBIHB', self.num_reports, self.event_type, self.address_type, addrl, addrh, self.len_data) + self.data + struct.pack (b'<b', self.rssi) + contents
		return super ().encode (packet)
	
	def decode (self, contents):
		super ().decode (contents)
		self.num_reports, self.event_type, self.address_type, addrl, addrh = struct.unpack (b'<BBBIH', contents[1:10])
		self.address = (addrh << 32) | addrl
		self.len_data = struct.unpack (b'<B', contents[10:11])[0]
		self.data = contents[11:11+self.len_data]
		self.rssi = struct.unpack (b'<b', contents[11+self.len_data:12+self.len_data])[0]

	def __repr__ (self):
		return "<LE Advertising Report Event (%d, %d, %d, %012x, %s, %d)>" % (self.num_reports, self.event_type, self.address_type, self.address, self.data, self.rssi)

################################################################################
################################################################################
################################################################################
