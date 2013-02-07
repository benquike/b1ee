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

import sys
import argparse
import socket
import time
import queue
import threading
import struct
import collections
import random
import pprint

import hci
import asyncsocket

################################################################################
################################################################################
################################################################################

ADT_FLAGS = 0x01
ADT_PART_SRV_16 = 0x02
ADT_FULL_SRV_16 = 0x03
ADT_PART_SRV_128 = 0x06
ADT_FULL_SRV_128 = 0x07
ADT_PART_NAME = 0x08
ADT_FULL_NAME = 0x09
ADT_TX_POWER = 0x0A
ADT_SRV_DATA = 0x16

################################################################################
################################################################################
################################################################################

class CommandQueue:
	def __init__ (self, max_priorities = 3):
		self.lock = threading.Lock ()
		self.lock.acquire ()
		self.max_priorities = max_priorities
		self.queue = []
		for i in range (self.max_priorities):
			self.queue.append (collections.deque ())
		self.lock.release ()
		self.not_empty = threading.Condition (self.lock)

	def put (self, priority, obj):
		self.lock.acquire ()
		self._put (priority, obj)
		self.not_empty.notify ()
		self.lock.release ()

	def get (self):
		self.not_empty.acquire ()
		try:
			while not self._qsize ():
				self.not_empty.wait ()

			obj = self._get ()

			return obj
		finally:
			self.not_empty.release ()
	
	def _qsize (self):
		size = 0
		for i in range (self.max_priorities):
			size += len (self.queue[i])
		return size

	def _get (self):
		for i in range (self.max_priorities-1, -1, -1):
			if len (self.queue[i]):
				return self.queue[i].popleft ()
	
	def _put (self, priority, obj):
		self.queue[priority].append (obj)

################################################################################
################################################################################
################################################################################

class UpperHCI (asyncsocket.AsyncSocket):
	def __init__ (self, args):
		s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
		s.connect ((args.host, args.post))
		self.socket = s
		asyncsocket.AsyncSocket.__init__ (self, s)
		self.async = asyncsocket.AsynchronousSockets ()
		self.running = True
		self.input_buffer = b''

		self.command_queue = CommandQueue ()
		self.callback_queue = queue.Queue ()
		self.command_event = threading.Event ()
		self.num_hci_commands = 1

		self.event_callback = {}

		self.thread = threading.Thread (target = self.process_command_queue)
		self.thread.daemon = True
		self.thread.start ()

	def handle_input (self, buffer):
		self.input_buffer += buffer

		while True:
			input_len = len (self.input_buffer)
			if input_len >= 1:
				packet_indicator = self.input_buffer[0]
				if packet_indicator == hci.HCI_EVENT:
					if input_len >= 3:
						event_code, event_length = struct.unpack ('<BB', self.input_buffer[1:3])
						if input_len >= 3 + event_length:
							contents = self.input_buffer[3:3+event_length]
							self.input_buffer = self.input_buffer[3+event_length:]
							event = hci.Event_Factory (event_code, contents)
							self.process_event (event)
							self.command_event.set ()
					else:
						break
				else:
					break
			else:
				break
	
	def handle_close (self):
		print ('HCI Transport Lost')
		super ().handle_close ()
		self.running = False

	def process_event (self, event):
		print ('Recv:', event)
		if isinstance (event, hci.Command_Complete_Event) or isinstance (event, hci.Command_Status_Event):
			if self.callback_queue.qsize ():
				callback = self.callback_queue.get ()
				callback (event)

			if event.num_hci_commands >= 1:
				self.num_hci_commands = 1
			else:
				self.num_hci_commands = 0

			self.command_event.set ()

		else:
			event_code = event.event_code
			if event_code == hci.HCI_LE_META_EVENT:
				event_code |= (event.subevent_code << 8)

			if event_code in self.event_callback:
				self.event_callback[event_code] (event)

	def process_command_queue (self):
		while True:
			if self.num_hci_commands > 0:
				command = self.command_queue.get ()
				if command[0]:
					self.num_hci_commands -= 1
					print ('Send:', command[0])
					if command[1]:
						self.callback_queue.put (command[1])
					self.socket.sendall (command[0].encode ())

				else:
					if command[1]:
						command[1] (None)
			else:
				self.command_event.wait ()

	def send_command (self, command, priority = 0, callback = None):
		self.command_queue.put (priority, (command, callback))
		self.command_event.set ()
	
	def register_callback_on (self, event, callback):
		event_code = event.event_code
		if event_code == hci.HCI_LE_META_EVENT:
			event_code |= (event.subevent_code << 8)
		self.event_callback[event_code] = callback
	
	def unregister_callback_on (self, event):
		event_code = event.event_code
		if event_code == hci.HCI_LE_META_EVENT:
			event_code |= (event.subevent_code << 8)
		del (self.event_callback[event_code])
	
	def stop (self):
		self.running = False
	
	def run (self):
		while self.running:
			self.async.select ()

################################################################################
################################################################################
################################################################################

class ConnectionManager (UpperHCI):
	def __init__ (self, args):
		self.ctrl_bd_addr = 0
		self.ctrl_lsf = 0
		self.ctrl_lelsf = 0
		self.ctrl_adv_txp = 0
		self.ctrl_num_buffers = 0
		self.ctrl_buffer_size = 0
		self.adv_data = b''
		self.scr_data = b''

		super ().__init__ (args)
	
	def reset_adv_data (self):
		self.adv_data = b''
	
	def reset_scr_data (self):
		self.scr_data = b''
	
	def add_adv_data (self, ad_type, ad_data):
		self.adv_data += struct.pack ('<BB', len (ad_data) + 1, ad_type) + ad_data
	
	def add_scr_data (self, ad_type, ad_data):
		self.scr_data += struct.pack ('<BB', len (ad_data) + 1, ad_type) + ad_data
	
	def cm_got_bd_addr (self, event):
		self.ctrl_bd_addr = event.bd_addr
	
	def cm_got_lsf (self, event):
		self.ctrl_lsf = event.features

	def cm_got_lelsf (self, event):
		self.ctrl_lelsf = event.le_features
	
	def cm_got_actxp (self, event):
		self.ctrl_adv_txp = event.transmit_power_level

		self.reset_adv_data ()
		self.add_adv_data (ADT_TX_POWER, struct.pack ('<b', self.ctrl_adv_txp))
		self.add_adv_data (ADT_PART_NAME, b'Robin')

		self.reset_scr_data ()
		self.add_scr_data (ADT_FULL_NAME, b'Robin\'s Test Advertiser')

		# Higher priority to put these in front of any other commands already queued
		self.send_command (hci.LE_Set_Advertising_Data_Command (adv_len = len (self.adv_data), adv_data = self.adv_data), priority = 1)
		self.send_command (hci.LE_Set_Scan_Response_Data_Command (scr_len = len (self.scr_data), scr_data = self.scr_data), priority = 1)
		self.send_command (hci.LE_Set_Advertising_Enable_Command (enable = True), priority = 1)

	def cm_got_actxp2 (self, event):
		self.ctrl_adv_txp = event.transmit_power_level

		self.reset_adv_data ()
		self.add_adv_data (ADT_TX_POWER, struct.pack ('<b', self.ctrl_adv_txp))
		self.add_adv_data (ADT_PART_NAME, b'Other')

		self.reset_scr_data ()
		self.add_scr_data (ADT_FULL_NAME, b'Robin\'s Other Advertiser')

		# Higher priority to put these in front of any other commands already queued
		self.send_command (hci.LE_Set_Advertising_Data_Command (adv_len = len (self.adv_data), adv_data = self.adv_data), priority = 1)
		self.send_command (hci.LE_Set_Scan_Response_Data_Command (scr_len = len (self.scr_data), scr_data = self.scr_data), priority = 1)
		self.send_command (hci.LE_Set_Advertising_Enable_Command (enable = True), priority = 1)

	def cm_got_done (self, event):
		self.stop ()

	def cm_reset (self):
		self.send_command (hci.Reset_Command ())
		self.send_command (hci.Read_BD_ADDR_Command (), callback = self.cm_got_bd_addr)
		self.send_command (hci.Read_Local_Version_Information_Command ())
		self.send_command (hci.Read_Local_Supported_Commands_Command ())
		self.send_command (hci.Read_Local_Supported_Features_Command (), callback = self.cm_got_lsf)
		self.send_command (hci.Read_Local_Extended_Features_Command (page_number = 0x01))
		self.send_command (hci.LE_Read_Local_Supported_Features_Command (), callback = self.cm_got_lelsf)
		self.send_command (hci.Set_Event_Mask_Command (0x2000000000008090))
		self.send_command (hci.Write_LE_Host_Supported_Command (0x01, 0x00))
		self.send_command (hci.LE_Set_Event_Mask_Command (0x000000000000001F))
		self.send_command (hci.LE_Read_Buffer_Size_Command ())
		self.send_command (hci.Read_Buffer_Size_Command ())
		self.send_command (hci.LE_Read_White_List_Size_Command ())
		self.send_command (hci.LE_Read_Supported_States_Command ())

	def cm_connection_complete (self, event):
		pass
		
	def cm_disconnection_complete (self, event):
		self.send_command (hci.LE_Set_Advertising_Enable_Command (enable = True), priority = 1)

	def cm_advertise (self):
		self.send_command (hci.LE_Set_Advertising_Parameters_Command (adv_int_min = 0x0640, adv_int_max = 0x0640))
		self.send_command (hci.LE_Read_Advertising_Channel_Tx_Power_Command (), callback = self.cm_got_actxp)
		self.register_callback_on (hci.LE_Connection_Complete_Event (), self.cm_connection_complete)
		self.register_callback_on (hci.Disconnection_Complete_Event (), self.cm_disconnection_complete)

	def cm_advertise2 (self):
		self.send_command (hci.LE_Set_Random_Address_Command (random_addr = random.randint (0, 2 ** 48 - 1)))
		self.send_command (hci.LE_Set_Advertising_Parameters_Command (adv_type = 0x00, own_addr_type = 1, adv_int_min = 0x0400, adv_int_max = 0x1000))
		self.send_command (hci.LE_Read_Advertising_Channel_Tx_Power_Command (), callback = self.cm_got_actxp2)

	def cm_done (self):
		self.send_command (None, callback = self.cm_got_done)

################################################################################
################################################################################
################################################################################

class Host (ConnectionManager):
	def __init__ (self, args):
		super ().__init__ (args)
	
	def parse_advertising_data (self, data):
		response = {}
		index = 0
		while index < len (data):
			tag_len = data[index]
			tag_type = data[index + 1]
			tag_data = data[index+2:index+1+tag_len]
			index += tag_len + 1

			if tag_type == ADT_FLAGS:
				response['flags'] = struct.unpack ('<B', tag_data)[0]
			elif tag_type == ADT_PART_NAME:
				response['partname'] = tag_data
			elif tag_type == ADT_FULL_NAME:
				response['fullname'] = tag_data
			elif tag_type == ADT_TX_POWER:
				response['tx_power'] = struct.unpack ('<b', tag_data)[0]

		return response

################################################################################
################################################################################
################################################################################

class ScannerHost (Host):
	def __init__ (self, args):
		super ().__init__ (args)
		self.found_devices = {}
		self.connect_list = []
		self.connection_handle = None

	def sh_got_advertising_report (self, event):
		tag_data = self.parse_advertising_data (event.data)
		addr = event.address
		if event.address_type:
			addr |= 1<<48

		if addr not in self.found_devices:
			self.found_devices[addr] = {'address': addr}

		self.found_devices[addr].update (tag_data)
		self.found_devices[addr].update ({'rssi': event.rssi})

	def sh_disconnect (self):
		self.send_command (hci.Disconnect_Command (connection_handle = self.connection_handle, reason = hci.EC_REMOTE_USER_TERMINATED_CONNECTION))

	def sh_send_data (self):
		self.supervision_timeout = threading.Timer (10, self.sh_disconnect)
		self.supervision_timeout.daemon = True;
		self.supervision_timeout.start ()

	def sh_cancel_connect (self):
		self.send_command (hci.LE_Create_Connection_Cancel_Command ())

	def sh_wait_connection_complete (self, event):
		self.connect_timeout = threading.Timer (10, self.sh_cancel_connect)
		self.connect_timeout.daemon = True;
		self.connect_timeout.start ()
	
	def sh_got_connection (self, event):
		self.connect_timeout.cancel ()
		if event.status == hci.EC_SUCCESS:
			self.connection_handle = event.connection_handle
			self.sh_send_data ()
		else:
			self.sh_connect_next ()
	
	def sh_disconnect_complete (self, event):
		self.supervision_timeout.cancel ()
		self.sh_connect_next ()
	
	def sh_connect_next (self):
		if len (self.connect_list) == 0:
			if random.randint (0, 1):
				return self.sh_active_scan ()
			else:
				return self.sh_passive_scan ()

		next_device = self.connect_list.pop ()

		addr = next_device['address']
		if addr & 0x1000000000000:
			pat = True
		else:
			pat = False
		addr &= 0xFFFFFFFFFFFF

		self.register_callback_on (hci.LE_Connection_Complete_Event (), self.sh_got_connection)
		self.register_callback_on (hci.Disconnection_Complete_Event (), self.sh_disconnect_complete)
		self.send_command (hci.LE_Create_Connection_Command (peer_address_type = pat, peer_address = addr), callback = self.sh_wait_connection_complete)

	def sh_connect_each (self, event):
		for addr in self.found_devices:
			self.connect_list.append (self.found_devices[addr])
		self.sh_connect_next ()
		self.found_devices = {}
	
	def sh_cancel_scan (self):
		# if len (self.found_devices) != 0:
			# self.send_command (hci.LE_Set_Scan_Enable_Command (enable = False), callback = self.sh_connect_each)
		# else:
		self.send_command (hci.LE_Set_Scan_Enable_Command (enable = False))
		if random.randint (0, 1):
			return self.sh_active_scan ()
		else:
			return self.sh_passive_scan ()
		t = threading.Timer (15, self.sh_cancel_scan)
		t.daemon = True
		t.start ()

	def sh_passive_scan (self):
		self.register_callback_on (hci.LE_Advertising_Report_Event (), self.sh_got_advertising_report)
		self.send_command (hci.LE_Set_Scan_Parameters_Command (scan_type = 0x00, scan_int = 0x0180, scan_window = 0x0100))
		self.send_command (hci.LE_Set_Scan_Enable_Command (enable = True, filter_duplicates = True))
		t = threading.Timer (15, self.sh_cancel_scan)
		t.daemon = True
		t.start ()

	def sh_active_scan (self):
		self.register_callback_on (hci.LE_Advertising_Report_Event (), self.sh_got_advertising_report)
		self.send_command (hci.LE_Set_Scan_Parameters_Command (scan_type = 0x01, scan_int = 0x0180, scan_window = 0x0100))
		self.send_command (hci.LE_Set_Scan_Enable_Command (enable = True, filter_duplicates = False))
		t = threading.Timer (15, self.sh_cancel_scan)
		t.daemon = True
		t.start ()

################################################################################
################################################################################
################################################################################

def main ():
	parser = argparse.ArgumentParser (description = 'Bluetooth Low Energy Client')

	parser.add_argument ('host',
		metavar='host',
		type=str,
		nargs='?',
		help='host address of server',
		default='b1ee.com')

	parser.add_argument ('-p', dest='post',
		metavar='port',
		type=int,
		nargs=1,
		help='port number of server',
		default=0xb1ee)

	parser.add_argument ('-adv', dest='adv',
		action = 'store_true',
		help='Start as an advertiser',
		default=False)

	parser.add_argument ('-adv2', dest='adv2',
		action = 'store_true',
		help='Start as an advertiser2',
		default=False)

	parser.add_argument ('-scan', dest='scan',
		action = 'store_true',
		help='Start as an scanner',
		default=False)

	args = parser.parse_args ()

	if args.adv:
		host = Host (args)
		host.cm_reset ()
		host.cm_advertise ()

	if args.adv2:
		host = Host (args)
		host.cm_reset ()
		host.cm_advertise2 ()
		t = threading.Timer (30, host.cm_done)
		t.daemon = True
		t.start ()

	if args.scan:
		host = ScannerHost (args)
		host.cm_reset ()
		host.sh_passive_scan ()

	host.run ()

################################################################################
################################################################################
################################################################################

if __name__ == "__main__":
	main ()

################################################################################
################################################################################
################################################################################
