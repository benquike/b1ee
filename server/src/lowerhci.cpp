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

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#include "controller.h"
#include "hci.h"
#include "log.h"

////////////////////////////////////////////////////////////////////////////////

LowerHCI::LowerHCI ()
{
	log (LOG_LOWERHCI, "LowerHCI");

	reset ();
}

////////////////////////////////////////////////////////////////////////////////

LowerHCI::~LowerHCI ()
{
	log (LOG_LOWERHCI, "~LowerHCI");
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::reset (void)
{
	log (LOG_LOWERHCI, "LowerHCI::reset");

	num_hci_command_packets = 1;
	hci_event_mask = 0x00001FFFFFFFFFFF;
	hci_le_event_mask = 0x000000000000001F;

	hci_le_acl_data_packet_length = 27;
	hci_total_num_le_acl_data_packets = 4;

	hci_acl_data_packet_length = 0;
	hci_acl_data_packet_length = 0;
	hci_synchronous_data_packet_length = 0;
	hci_total_num_acl_data_packets = 0;
	hci_total_num_acl_data_packets = 0;
	hci_total_num_synchronous_data_packets = 0;
	hci_total_num_synchronous_data_packets = 0;

	memset (hci_supported_commands, 0, sizeof (hci_supported_commands));

	hci_supported_commands[5] |= (1 << 6); // Set Event Mask
	hci_supported_commands[5] |= (1 << 7); // Reset
	hci_supported_commands[14] |= (1 << 3); // Read Local Version Information
	hci_supported_commands[14] |= (1 << 5); // Read Local Supported Features
	hci_supported_commands[14] |= (1 << 6); // Read Local Extended Features
	hci_supported_commands[14] |= (1 << 7); // Read Buffer Size
	hci_supported_commands[15] |= (1 << 1); // Read BD_ADDR
	hci_supported_commands[24] |= (1 << 5); // Read LE Host Supported
	hci_supported_commands[24] |= (1 << 6); // Write LE Host Supported
	hci_supported_commands[25] |= (1 << 0); // LE Set Event Mask
	hci_supported_commands[25] |= (1 << 1); // LE Read Buffer Size
	hci_supported_commands[25] |= (1 << 2); // LE Read Local Supported Features
	hci_supported_commands[25] |= (1 << 4); // LE Set Random Address
	hci_supported_commands[25] |= (1 << 5); // LE Set Advertising Parameters
	hci_supported_commands[25] |= (1 << 6); // LE Read Advertising Channel Tx Power
	hci_supported_commands[25] |= (1 << 7); // LE Set Advertising Data
	hci_supported_commands[26] |= (1 << 0); // LE Set Scan Response Data
	hci_supported_commands[26] |= (1 << 1); // LE Set Advertising Enable
	hci_supported_commands[26] |= (1 << 2); // LE Set Scan Parameters
	hci_supported_commands[26] |= (1 << 3); // LE Set Scan Enable
	hci_supported_commands[26] |= (1 << 4); // LE Create Connection
	hci_supported_commands[26] |= (1 << 5); // LE Create Connection Cancel

};

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_set_event_mask_command (int parameter_len, char *parameters)
{
	char buffer[1];


	log (LOG_LOWERHCI, "HCI Set Event Mask Command");

	if (parameter_len == 8)
	{
		hci_event_mask =  (((uint64) parameters[0]) & 0xFF);
		hci_event_mask |= (((uint64) parameters[1]) & 0xFF) << 8;
		hci_event_mask |= (((uint64) parameters[2]) & 0xFF) << 16;
		hci_event_mask |= (((uint64) parameters[3]) & 0xFF) << 24;
		hci_event_mask |= (((uint64) parameters[4]) & 0xFF) << 32;
		hci_event_mask |= (((uint64) parameters[5]) & 0xFF) << 40;
		hci_event_mask |= (((uint64) parameters[6]) & 0xFF) << 48;
		hci_event_mask |= (((uint64) parameters[7]) & 0xFF) << 56;
		hci_event_mask |= (1 << (COMMAND_COMPLETE_EVENT -1));
		hci_event_mask |= (1 << (COMMAND_STATUS_EVENT - 1));
		hci_event_mask |= (1 << (NUMBER_OF_COMPLETED_PACKETS_EVENT - 1));

		buffer[0] = EC_SUCCESS;
	}
	else
	{
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}

	send_command_complete_event (HCI_SET_EVENT_MASK_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_reset_command (int parameter_len, char *parameters)
{
	char buffer[1];


	log (LOG_LOWERHCI, "HCI Reset Command");

	PhysicalLayer::reset ();
	LinkLayer::reset ();
	LowerHCI::reset ();

	buffer[0] = EC_SUCCESS;

	send_command_complete_event (HCI_RESET_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_write_le_host_supported_command (int parameter_len, char *parameters)
{
	char buffer[1];
	int le_supported_host;
	int simultaneous_le_host;


	log (LOG_LOWERHCI, "HCI Write Le Host Supported Command");

	if (parameter_len == 2)
	{
		le_supported_host = parameters[0];
		simultaneous_le_host = parameters[1];

		ll_set_host_supports (le_supported_host, simultaneous_le_host);

		buffer[0] = EC_SUCCESS;
	}
	else
	{
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}

	send_command_complete_event (HCI_WRITE_LE_HOST_SUPPORTED_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_read_local_version_information_command (int parameter_len, char *parameters)
{
	char buffer[9];
	uint16 hci_revision;
	uint16 manufacturer;
	uint16 ll_subversion;


	log (LOG_LOWERHCI, "HCI Read Local Version Information Command");

	hci_revision = hci_get_revision ();
	manufacturer = hci_get_manufacturer ();
	ll_subversion = ll_get_subversion ();

	buffer[0] = EC_SUCCESS;
	buffer[1] = (hci_get_version ()) & 0xFF;
	buffer[2] = (hci_revision) & 0xFF;
	buffer[3] = (hci_revision >> 8) & 0xFF;
	buffer[4] = (ll_get_version ()) & 0xFF;
	buffer[5] = (manufacturer) & 0xFF;
	buffer[6] = (manufacturer >> 8) & 0xFF;
	buffer[7] = (ll_subversion) & 0xFF;
	buffer[8] = (ll_subversion >> 8) & 0xFF;

	send_command_complete_event (HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND, 9, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_read_local_supported_commands_command (int parameter_len, char *parameters)
{
	char buffer[65];


	log (LOG_LOWERHCI, "HCI Read Local Supported Commands Command");

	buffer[0] = EC_SUCCESS;
	memcpy (&buffer[1], hci_supported_commands, 64);

	send_command_complete_event (HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND, 65, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_read_local_supported_features_command (int parameter_len, char *parameters)
{
	char buffer[9];
	uint64 features;


	log (LOG_LOWERHCI, "HCI Read Local Supported Features Command");

	features = ll_get_extended_features (0);

	buffer[0] = EC_SUCCESS;
	buffer[1] = (features);
	buffer[2] = (features >> 8);
	buffer[3] = (features >> 16);
	buffer[4] = (features >> 24);
	buffer[5] = (features >> 32);
	buffer[6] = (features >> 40);
	buffer[7] = (features >> 48);
	buffer[8] = (features >> 56);

	send_command_complete_event (HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND, 9, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_read_local_extended_features_command (int parameter_len, char *parameters)
{
	char buffer[11];
	uint64 features;
	int page_number;


	log (LOG_LOWERHCI, "HCI Read Local Supported Features Command");

	if (parameter_len == 1)
	{
		page_number = parameters[0] & 0xFF;
		buffer[0] = EC_SUCCESS;
	}
	else
	{
		page_number = 1;
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}

	features = ll_get_extended_features (page_number);

	buffer[1] = page_number;
	buffer[2] = ll_get_maximum_features_page_number ();
	buffer[3] = (features);
	buffer[4] = (features >> 8);
	buffer[5] = (features >> 16);
	buffer[6] = (features >> 24);
	buffer[7] = (features >> 32);
	buffer[8] = (features >> 40);
	buffer[9] = (features >> 48);
	buffer[10] = (features >> 56);

	send_command_complete_event (HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND, 11, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_read_buffer_size_command (int parameter_len, char *parameters)
{
	char buffer[8];


	log (LOG_LOWERHCI, "HCI Read Buffer Size Command");

	buffer[0] = EC_SUCCESS;
	buffer[1] = (hci_acl_data_packet_length) & 0xff;
	buffer[2] = (hci_acl_data_packet_length >> 8) & 0xff;
	buffer[3] = (hci_synchronous_data_packet_length) & 0xff;
	buffer[4] = (hci_total_num_acl_data_packets) & 0xff;
	buffer[5] = (hci_total_num_acl_data_packets >> 8) & 0xff;
	buffer[6] = (hci_total_num_synchronous_data_packets) & 0xff;
	buffer[7] = (hci_total_num_synchronous_data_packets >> 8) & 0xff;

	send_command_complete_event (HCI_READ_BUFFER_SIZE_COMMAND, 8, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_read_bd_addr_command (int parameter_len, char *parameters)
{
	char buffer[7];
	uint64 bd_addr;


	log (LOG_LOWERHCI, "HCI Read BD_ADDR Command");

	bd_addr = ll_get_bd_addr ();

	buffer[0] = EC_SUCCESS;
	buffer[1] = (bd_addr) & 0xFF;
	buffer[2] = (bd_addr >> 8) & 0xFF;
	buffer[3] = (bd_addr >> 16) & 0xFF;
	buffer[4] = (bd_addr >> 24) & 0xFF;
	buffer[5] = (bd_addr >> 32) & 0xFF;
	buffer[6] = (bd_addr >> 40) & 0xFF;

	send_command_complete_event (HCI_READ_BD_ADDR_COMMAND, 7, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_set_event_mask_command (int parameter_len, char *parameters)
{
	char buffer[1];
	uint64 hci_le_event_mask;


	log (LOG_LOWERHCI, "HCI LE Set Event Mask Command");

	if (parameter_len == 8)
	{
		hci_le_event_mask =  (((uint64) parameters[0]) & 0xFF);
		hci_le_event_mask |= (((uint64) parameters[1]) & 0xFF) << 8;
		hci_le_event_mask |= (((uint64) parameters[2]) & 0xFF) << 16;
		hci_le_event_mask |= (((uint64) parameters[3]) & 0xFF) << 24;
		hci_le_event_mask |= (((uint64) parameters[4]) & 0xFF) << 32;
		hci_le_event_mask |= (((uint64) parameters[5]) & 0xFF) << 40;
		hci_le_event_mask |= (((uint64) parameters[6]) & 0xFF) << 48;
		hci_le_event_mask |= (((uint64) parameters[7]) & 0xFF) << 56;

		buffer[0] = EC_SUCCESS;
	}
	else
	{
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}
	
	send_command_complete_event (HCI_LE_SET_EVENT_MASK_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_read_buffer_size_command (int parameter_len, char *parameters)
{
	char buffer[4];


	buffer[0] = EC_SUCCESS;
	buffer[1] = (hci_le_acl_data_packet_length) & 0xFF;
	buffer[2] = (hci_le_acl_data_packet_length >> 8) & 0xFF;
	buffer[3] = hci_total_num_le_acl_data_packets;

	send_command_complete_event (HCI_LE_READ_BUFFER_SIZE_COMMAND, 4, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_read_local_supported_features_command (int parameter_len, char *parameters)
{
	char buffer[9];
	uint64 features;


	log (LOG_LOWERHCI, "HCI LE Read Local Supported Features Command");

	features = ll_get_le_features ();

	buffer[0] = EC_SUCCESS;
	buffer[1] = (features);
	buffer[2] = (features >> 8);
	buffer[3] = (features >> 16);
	buffer[4] = (features >> 24);
	buffer[5] = (features >> 32);
	buffer[6] = (features >> 40);
	buffer[7] = (features >> 48);
	buffer[8] = (features >> 56);

	send_command_complete_event (HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND, 9, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_set_advertising_parameters_command (int parameter_len, char *parameters)
{
	char buffer[1];
	int advertising_interval_min;
	int advertising_interval_max;
	int advertising_type;
	int own_address_type;
	int direct_address_type;
	uint64 direct_address;
	int advertising_channel_map;
	int advertising_filter_policy;


	log (LOG_LOWERHCI, "HCI LE Set Advertising Parameters Command");

	if (parameter_len == 15)
	{
		advertising_interval_min = (parameters[0] & 0xFF) | ((parameters[1] & 0xFF) << 8);
		advertising_interval_max = (parameters[2] & 0xFF) | ((parameters[3] & 0xFF) << 8);
		advertising_type = parameters[4] & 0xFF;
		own_address_type = parameters[5] & 0xFF;
		direct_address_type = parameters[6] & 0xFF;
		direct_address =  ((uint64) parameters[7]) & 0xFF;
		direct_address |= ((uint64) (parameters[8] & 0xFF)) << 8;
		direct_address |= ((uint64) (parameters[9] & 0xFF)) << 16;
		direct_address |= ((uint64) (parameters[10] & 0xFF)) << 24;
		direct_address |= ((uint64) (parameters[11] & 0xFF)) << 32;
		direct_address |= ((uint64) (parameters[12] & 0xFF)) << 40;
		advertising_channel_map = parameters[13];
		advertising_filter_policy = parameters[14];

		ll_set_advertising_parameters (advertising_interval_min, advertising_interval_max, advertising_type, own_address_type, direct_address_type, direct_address, advertising_channel_map, advertising_filter_policy);

		buffer[0] = EC_SUCCESS;
	}
	else
	{
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}

	send_command_complete_event (HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_read_advertising_channel_tx_power_command (int parameter_len, char *parameters)
{
	char buffer[2];


	log (LOG_LOWERHCI, "HCI LE Read Advertising Channel Tx Power Command");

	buffer[0] = EC_SUCCESS;
	buffer[1] = 0x00;

	send_command_complete_event (HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND, 2, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_set_advertising_data_command (int parameter_len, char *parameters)
{
	char buffer[1];
	int advertising_data_length;
	char *advertising_data;


	log (LOG_LOWERHCI, "HCI LE Set Advertising Data Command");

	advertising_data_length = parameters[0];
	advertising_data = &parameters[1];

	ll_set_advertising_data (advertising_data_length, advertising_data);

	buffer[0] = EC_SUCCESS;

	send_command_complete_event (HCI_LE_SET_ADVERTISING_DATA_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_set_scan_response_data_command (int parameter_len, char *parameters)
{
	char buffer[1];
	int scan_response_data_length;
	char *scan_response_data;


	log (LOG_LOWERHCI, "HCI LE Set Advertising Data Command");

	scan_response_data_length = parameters[0];
	scan_response_data = &parameters[1];

	ll_set_scan_response_data (scan_response_data_length, scan_response_data);

	buffer[0] = EC_SUCCESS;

	send_command_complete_event (HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_set_advertise_enable_command (int parameter_len, char *parameters)
{
	char buffer[1];


	log (LOG_LOWERHCI, "HCI LE Set Advertise Enable Command");

	if (ll_set_advertising_enable (parameters[0]))
	{
		buffer[0] = EC_SUCCESS;
	}
	else
	{
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}

	send_command_complete_event (HCI_LE_SET_ADVERTISE_ENABLE_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_set_scan_parameters_command (int parameter_len, char *parameters)
{
	char buffer[1];
	int scan_type;
	int scan_interval;
	int scan_window;
	int own_address_type;
	int scanning_filter_policy;


	log (LOG_LOWERHCI, "HCI LE Set Scan Parameters Command");

	if (parameter_len == 7)
	{
		scan_type = parameters[0];
		scan_interval = (parameters[1] & 0xFF) | ((parameters[2] & 0xFF) << 8);
		scan_window = (parameters[3] & 0xFF) | ((parameters[4] & 0xFF) << 8);
		own_address_type = parameters[5] & 0xFF;
		scanning_filter_policy = parameters[6] & 0xFF;

		ll_set_scan_parameters (scan_type, scan_interval, scan_window, own_address_type, scanning_filter_policy);

		buffer[0] = EC_SUCCESS;
	}
	else
	{
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}

	send_command_complete_event (HCI_LE_SET_SCAN_PARAMETERS_COMMAND, 1, buffer);	
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_set_scan_enable_command (int parameter_len, char *parameters)
{
	char buffer[1];


	log (LOG_LOWERHCI, "HCI LE Set Scan Enable Command");

	if (ll_set_scan_enable (parameters[0], parameters[1]))
	{
		buffer[0] = EC_SUCCESS;
	}
	else
	{
		buffer[0] = EC_INVALID_HCI_COMMAND_PARAMETERS;
	}

	send_command_complete_event (HCI_LE_SET_SCAN_ENABLE_COMMAND, 1, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_read_white_list_size_command (int parameter_len, char *parameters)
{
	char buffer[2];


	log (LOG_LOWERHCI, "HCI LE Read White List Size Command");

	buffer[0] = EC_SUCCESS;
	buffer[1] = ll_get_maximum_number_of_white_list_entries ();

	send_command_complete_event (HCI_LE_READ_WHITE_LIST_SIZE_COMMAND, 2, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_le_read_supported_states_command (int parameter_len, char *parameters)
{
	char buffer[9];
	uint64 states;


	log (LOG_LOWERHCI, "HCI LE Read Supported States Command");

	states = ll_get_supported_states ();

	buffer[0] = EC_SUCCESS;
	buffer[1] = (states);
	buffer[2] = (states >> 8);
	buffer[3] = (states >> 16);
	buffer[4] = (states >> 24);
	buffer[5] = (states >> 32);
	buffer[6] = (states >> 40);
	buffer[7] = (states >> 48);
	buffer[8] = (states >> 56);

	send_command_complete_event (HCI_LE_READ_SUPPORTED_STATES_COMMAND, 9, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::hci_unsupported_command (int opcode)
{
	char buffer[4];

	log (LOG_LOWERHCI, "LowerHCI::unsupported_command %04X", opcode);

	buffer[0] = EC_UNKNOWN_HCI_COMMAND;
	buffer[1] = (unsigned char) num_hci_command_packets;
	buffer[2] = (opcode) & 0xFF;
	buffer[3] = (opcode >> 8) & 0xFF;

	send_event (COMMAND_STATUS_EVENT, 4, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::process_command (int opcode, int parameter_len, char *parameters)
{
	log_start (LOG_LOWERHCI, "HCI Command %04X (%d) ", opcode, parameter_len);
	for (int index = 0; index < parameter_len; index ++)
	{
		log_continuation ("%02X", parameters[index] & 0xFF);
	}
	log_end ();

	switch (opcode)
	{
		case HCI_SET_EVENT_MASK_COMMAND:
			hci_set_event_mask_command (parameter_len, parameters); break;

		case HCI_RESET_COMMAND:
			hci_reset_command (parameter_len, parameters); break;

		case HCI_WRITE_LE_HOST_SUPPORTED_COMMAND:
			hci_write_le_host_supported_command (parameter_len, parameters); break;

		case HCI_READ_LOCAL_VERSION_INFORMATION_COMMAND:
			hci_read_local_version_information_command (parameter_len, parameters); break;

		case HCI_READ_LOCAL_SUPPORTED_COMMANDS_COMMAND:
			hci_read_local_supported_commands_command (parameter_len, parameters); break;

		case HCI_READ_LOCAL_SUPPORTED_FEATURES_COMMAND:
			hci_read_local_supported_features_command (parameter_len, parameters); break;

		case HCI_READ_LOCAL_EXTENDED_FEATURES_COMMAND:
			hci_read_local_extended_features_command (parameter_len, parameters); break;

		case HCI_READ_BUFFER_SIZE_COMMAND:
			hci_read_buffer_size_command (parameter_len, parameters); break;

		case HCI_READ_BD_ADDR_COMMAND:
			hci_read_bd_addr_command (parameter_len, parameters); break;

		case HCI_LE_SET_EVENT_MASK_COMMAND:
			hci_le_set_event_mask_command (parameter_len, parameters); break;

		case HCI_LE_READ_BUFFER_SIZE_COMMAND:
			hci_le_read_buffer_size_command (parameter_len, parameters); break;

		case HCI_LE_READ_LOCAL_SUPPORTED_FEATURES_COMMAND:
			hci_le_read_local_supported_features_command (parameter_len, parameters); break;

		case HCI_LE_SET_ADVERTISING_PARAMETERS_COMMAND:
			hci_le_set_advertising_parameters_command (parameter_len, parameters); break;

		case HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER_COMMAND:
			hci_le_read_advertising_channel_tx_power_command (parameter_len, parameters); break;

		case HCI_LE_SET_ADVERTISING_DATA_COMMAND:
			hci_le_set_advertising_data_command (parameter_len, parameters); break;

		case HCI_LE_SET_SCAN_RESPONSE_DATA_COMMAND:
			hci_le_set_scan_response_data_command (parameter_len, parameters); break;

		case HCI_LE_SET_ADVERTISE_ENABLE_COMMAND:
			hci_le_set_advertise_enable_command (parameter_len, parameters); break;

		case HCI_LE_SET_SCAN_PARAMETERS_COMMAND:
			hci_le_set_scan_parameters_command (parameter_len, parameters); break;

		case HCI_LE_SET_SCAN_ENABLE_COMMAND:
			hci_le_set_scan_enable_command (parameter_len, parameters); break;
			
		case HCI_LE_READ_WHITE_LIST_SIZE_COMMAND:
			hci_le_read_white_list_size_command (parameter_len, parameters); break;

		case HCI_LE_READ_SUPPORTED_STATES_COMMAND:
			hci_le_read_supported_states_command (parameter_len, parameters); break;

		default:
			hci_unsupported_command (opcode); break;
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::send_event (int opcode, int parameter_len, char *parameters)
{
	char header[3];


	if ((1LL << (opcode - 1)) & hci_event_mask)
	{
		log_start (LOG_LOWERHCI, "HCI Event %02X (%d) ", opcode, parameter_len);
		for (int index = 0; index < parameter_len; index ++)
		{
			log_continuation ("%02X", parameters[index] & 0xFF);
		}
		log_end ();

		header[0] = HCI_EVENT;
		header[1] = opcode;
		header[2] = parameter_len;

		write_data (header, 3);
		write_data (parameters, parameter_len);
	}
	else
	{
		log (LOG_LOWERHCI, "Filtered HCI Event %02X : %016llX", opcode, hci_event_mask);
	}
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::send_command_complete_event (int command_opcode, int parameter_len, char *parameters)
{
	char buffer[255];


	log (LOG_LOWERHCI, "LowerHCI::send_command_complete_event %04X", command_opcode);

	buffer[0] = (unsigned char) num_hci_command_packets;
	buffer[1] = (command_opcode) & 0xFF;
	buffer[2] = (command_opcode >> 8) & 0xFF;

	if (parameter_len > 0)
	{
		memcpy (&buffer[3], parameters, parameter_len);
	}
	
	send_event (COMMAND_COMPLETE_EVENT, 3 + parameter_len, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void LowerHCI::send_le_advertising_report_event (int len, uint8 *data)
{
	char buffer[255];


	log (LOG_LOWERHCI, "LowerHCI::send_le_advertising_report_event");

	buffer[0] = LE_ADVERTISING_REPORT_EVENT;
	buffer[1] = 1;
	buffer[2] = 0x00;
	buffer[3] = 0;
	buffer[4] = data[2];
	buffer[5] = data[3];
	buffer[6] = data[4];
	buffer[7] = data[5];
	buffer[8] = data[6];
	buffer[9] = data[7];
	buffer[10] = len - 8;
	memcpy (&buffer[11], &data[8], len - 8);
	buffer[11 + len - 8] = -60;

	send_event (LE_META_EVENT, 12 + len - 8, buffer);
}

////////////////////////////////////////////////////////////////////////////////

uint8 LowerHCI::hci_get_version (void)
{
	return 0x06;
}

////////////////////////////////////////////////////////////////////////////////

uint16 LowerHCI::hci_get_revision (void)
{
	long t;


	t = get_program_start_time ();

	return (t >> 4) & 0xFFFF;
}

////////////////////////////////////////////////////////////////////////////////

uint16 LowerHCI::hci_get_manufacturer (void)
{
	return 0xFFFF;
}

////////////////////////////////////////////////////////////////////////////////
