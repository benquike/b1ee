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
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

#include "controller.h"
#include "log.h"

////////////////////////////////////////////////////////////////////////////////

LinkLayer::LinkLayer ()
{
	log (LOG_LINKLAYER, "LinkLayer::LinkLayer");

	ll_packet = new PhysicalPacket (this);

	reset ();
}

////////////////////////////////////////////////////////////////////////////////

LinkLayer::~LinkLayer ()
{
	log (LOG_LINKLAYER, "LinkLayer::~LinkLayer");
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::reset (void)
{
	log (LOG_LINKLAYER, "LinkLayer::reset");

	for (int index = 0; index < maximum_features_page_number; index ++)
	{
		lmp_features[index] = 0x00000000000000000000000000000000;
	}

	lmp_features[0] = 0x00000000000000008000006000000000;

	le_features = 0x00000000000000000000000000000000;
	ll_supported_states = 0x00000000000000000000000000000037;

	ll_advertising_interval_min = 0x0800;
	ll_advertising_interval_max = 0x0800;
	ll_advertising_type = 0x00;
	ll_advertising_own_address_type = 0x00;
	ll_direct_address_type = 0x00;
	ll_direct_address = 0x000000000000;
	ll_advertising_channel_map = 0x07;
	ll_advertising_filter_policy = 0x00;

	ll_advertising_data_length = 0x00;
	memset (ll_advertising_data, 0, 31);

	ll_scan_response_data_length = 0x00;
	memset (ll_scan_response_data, 0, 31);

	ll_scan_type = 0;
	ll_scan_interval = 0;
	ll_scan_window = 0;
	ll_scan_own_address_type = 0;
	ll_scanning_filter_policy = 0;
	ll_scanning_enabled = 0;
	ll_scan_filter_duplicates = 0;
	
	for (int index = 0; index < maximum_number_of_link_layer_state_machines; index ++)
	{
		machine[index].reset ();
	}

	last_machine = 0;
}

////////////////////////////////////////////////////////////////////////////////

uint64 LinkLayer::ll_get_bd_addr (void)
{
	return ll_bd_addr;
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::ll_set_bd_addr (uint64 bd_addr)
{
	ll_bd_addr = bd_addr;
}

////////////////////////////////////////////////////////////////////////////////

uint8 LinkLayer::ll_get_version (void)
{
	return 0x06;
}

////////////////////////////////////////////////////////////////////////////////

uint16 LinkLayer::ll_get_subversion (void)
{
	long t;


	t = get_program_start_time ();

	return t & 0xFFFF;
}

////////////////////////////////////////////////////////////////////////////////

uint8 LinkLayer::ll_get_maximum_features_page_number (void)
{
	return maximum_features_page_number;
}

////////////////////////////////////////////////////////////////////////////////

uint64 LinkLayer::ll_get_extended_features (uint8 page_number)
{
	uint64 features;


	if (page_number < maximum_features_page_number)
	{
		features = lmp_features[page_number];
	}
	else
	{
		features = 0x00000000000000000000000000000000;
	}

	return features;
}

////////////////////////////////////////////////////////////////////////////////

uint64 LinkLayer::ll_get_le_features (void)
{
	return le_features;
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::ll_set_host_supports (int le_supported_host, int simultaneous_le_host)
{
	lmp_features[1] &= ~(1 << 1);
	lmp_features[1] &= ~(1 << 2);

	if (le_supported_host == 0x01)
	{
		lmp_features[1] |= (1 << 1);
	}

	if (simultaneous_le_host == 0x01)
	{
		lmp_features[1] |= (1 << 2);
	}
}

////////////////////////////////////////////////////////////////////////////////

int LinkLayer::ll_get_maximum_number_of_white_list_entries (void)
{
	return maximum_number_of_white_list_entries;
}

////////////////////////////////////////////////////////////////////////////////

uint64 LinkLayer::ll_get_supported_states (void)
{
	return ll_supported_states;
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::ll_set_advertising_parameters (int advertising_interval_min, int advertising_interval_max, int advertising_type, int own_address_type, int direct_address_type, uint64 direct_address, int advertising_channel_map, int advertising_filter_policy)
{
	ll_advertising_interval_min = advertising_interval_min;
	ll_advertising_interval_max = advertising_interval_max;
	ll_advertising_type = advertising_type;
	ll_advertising_own_address_type = own_address_type;
	ll_direct_address_type = direct_address_type;
	ll_direct_address = direct_address;
	ll_advertising_channel_map = advertising_channel_map;
	ll_advertising_filter_policy = advertising_filter_policy;
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::ll_set_advertising_data (int len, char *data)
{
	if (len > 31)
	{
		len = 31;
	}

	ll_advertising_data_length = len;

	memset (ll_advertising_data, 0, 31);
	memcpy (ll_advertising_data, data, len);
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::ll_set_scan_response_data (int len, char *data)
{
	if (len > 31)
	{
		len = 31;
	}

	ll_scan_response_data_length = len;

	memset (ll_scan_response_data, 0, 31);
	memcpy (ll_scan_response_data, data, len);
}

////////////////////////////////////////////////////////////////////////////////

bool LinkLayer::ll_set_advertising_enable (int enable)
{
	int index;


	if ((enable) && (ll_advertising_enabled == false))
	{
		for (index = 0; index < maximum_number_of_link_layer_state_machines; index ++)
		{
			if (machine[index].state == LLS_Idle)
			{
				ll_advertising_enabled = enable;
				machine[index].mk_advertiser (last_clock);
				return true;
			}
		}
	}
	else if ((!enable) && (ll_advertising_enabled == true))
	{
		for (index = 0; index < maximum_number_of_link_layer_state_machines; index ++)
		{
			if (machine[index].state == LLS_Advertising)
			{
				ll_advertising_enabled = enable;

				machine[index].mk_idle ();

				return true;
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::ll_set_scan_parameters (int scan_type, int scan_interval, int scan_window, int own_address_type, int scanning_filter_policy)
{
	ll_scan_type = scan_type;
	ll_scan_interval = scan_interval;
	ll_scan_window = scan_window;
	ll_scan_own_address_type = own_address_type;
	ll_scanning_filter_policy = scanning_filter_policy;
}

////////////////////////////////////////////////////////////////////////////////

bool LinkLayer::ll_set_scan_enable (int enable, int filter_duplicates)
{
	int index;


	if ((enable) && (ll_scanning_enabled == false))
	{
		for (index = 0; index < maximum_number_of_link_layer_state_machines; index ++)
		{
			if (machine[index].state == LLS_Idle)
			{
				ll_scanning_enabled = enable;
				ll_scan_filter_duplicates = filter_duplicates;

				machine[index].mk_scanner (last_clock);

				return true;
			}
		}
	}
	else if ((!enable) && (ll_scanning_enabled == true))
	{
		for (index = 0; index < maximum_number_of_link_layer_state_machines; index ++)
		{
			if (machine[index].state == LLS_Scanning)
			{
				ll_scanning_enabled = enable;

				machine[index].mk_idle ();

				return true;
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

PhysicalPacket *LinkLayer::get_next_packet (int64 after)
{
	uint8 buffer[maximum_pdu_length];
	uint8 length;
	int index;
	int count;


	if (is_delete_pending ())
	{
		set_delete_ready ();
	}
	else
	{
		last_clock = after;

		index = (last_machine + 1) % maximum_number_of_link_layer_state_machines;
		count = 0;

		while (count < maximum_number_of_link_layer_state_machines)
		{
			count = count + 1;

			if (machine[index].state == LLS_Advertising)
			{
				if (machine[index].adv.ll_next_advertising_tx > after)
				{
					ll_packet->set_transmit (37 + machine[index].adv.ll_advertising_channel, GFSK_LE, machine[index].adv.ll_next_advertising_tx);
					ll_packet->set_access_address (advertising_access_address);
					buffer[0] = 0x00;
					buffer[1] = 8 + ll_advertising_data_length;
					buffer[2] = (ll_bd_addr >> 0) & 0xFF;
					buffer[3] = (ll_bd_addr >> 8) & 0xFF;
					buffer[4] = (ll_bd_addr >> 16) & 0xFF;
					buffer[5] = (ll_bd_addr >> 24) & 0xFF;
					buffer[6] = (ll_bd_addr >> 32) & 0xFF;
					buffer[7] = (ll_bd_addr >> 40) & 0xFF;
					length = 8;
					if (ll_advertising_data_length > 0)
					{
						memcpy (&buffer[length], ll_advertising_data, ll_advertising_data_length);
						length = 8 + ll_advertising_data_length;
					}
					ll_packet->set_pdu (length, buffer);
					ll_packet->set_llsm (index);

					machine[index].adv.ll_advertising_channel = (machine[index].adv.ll_advertising_channel + 1) % 3;

					if (machine[index].adv.ll_advertising_channel == 0)
					{
						machine[index].adv.ll_next_advertising_instant += ll_advertising_interval_min * 625;
						machine[index].adv.ll_next_advertising_tx = machine[index].adv.ll_next_advertising_instant + (rand () % 16) * 625;
					}
					else
					{
						machine[index].adv.ll_next_advertising_tx += 8 + 32 + length * 8 + 24 + 150;
					}

					last_machine = index;

					return ll_packet;
				}
				else if (machine[index].adv.ll_next_advertising_tx < after)
				{
					machine[index].adv.ll_next_advertising_instant += ll_advertising_interval_min * 625;
					machine[index].adv.ll_next_advertising_tx = machine[index].adv.ll_next_advertising_instant + (rand () % 16) * 625;
				}
			}
			else if (machine[index].state == LLS_Scanning)
			{
				if (machine[index].scan.ll_next_scanning_instant > after)
				{
					ll_packet->set_receive (37 + machine[index].scan.ll_scanning_channel, GFSK_LE, machine[index].scan.ll_next_scanning_instant, machine[index].scan.ll_next_scanning_instant + ll_scan_window * 625 - 150);
					ll_packet->set_access_address (advertising_access_address);
					ll_packet->set_llsm (index);

					machine[index].scan.ll_scanning_channel = (machine[index].scan.ll_scanning_channel + 1) % 3;

					machine[index].scan.ll_next_scanning_instant += ll_scan_interval * 625;

					return ll_packet;
				}
				else if (machine[index].scan.ll_next_scanning_instant < after)
				{
					machine[index].scan.ll_next_scanning_instant += ll_scan_interval * 625;
				}
			}

			index = (index + 1) % maximum_number_of_link_layer_state_machines;
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void LinkLayer::end_of_packet (PhysicalPacket *packet, int64 when, int rx_len, uint8 *rx_data)
{
	int index;


	log_start (LOG_LINKLAYER, "    LinkLayer::end_of_packet %016lld %d:%p ", when, rx_len, rx_data);
	packet->log ();
	log_end ();

	if (rx_len)
	{
		//for (int index = 0; index < rx_len; index ++)
		//{
		//	printf ("%02X", rx_data[index]);
		//}
		//printf ("\n");

		index = packet->get_llsm ();

		if (machine[index].state == LLS_Scanning)
		{
			if (machine[index].scan.substate == SSS_Scan)
			{
				log (LOG_LINKLAYER, "LE Advertising Report Event");
				send_le_advertising_report_event (rx_len, rx_data);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
