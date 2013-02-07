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

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#include "controller.h"
#include "log.h"

////////////////////////////////////////////////////////////////////////////////

int64 physical_clock = 0;	// nanoseconds

pthread_mutex_t physical_layer_mutex;

PhysicalLayer *PhysicalLayer::all_radios = 0;

PhysicalPacket *PhysicalLayer::ordered_transmitters = 0;
PhysicalPacket *PhysicalLayer::ordered_receivers = 0;

int PhysicalLayer::transmitting[40] = { 0 };
bool PhysicalLayer::bad_transmission[40] = { false };

////////////////////////////////////////////////////////////////////////////////

PhysicalPacket::PhysicalPacket (PhysicalLayer *phy)
{
	is_tx = false;
	channel = 0;
	modulation = GFSK_LE;
	start_time = 0;
	end_time = 0;
	pdu_length = 0;
	physical_layer = phy;
}

////////////////////////////////////////////////////////////////////////////////

PhysicalPacket::~PhysicalPacket ()
{
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalPacket::set_transmit (uint8 chan, PhyModulation mod, int64 when)
{
	is_tx = true;
	channel = chan;
	modulation = mod;
	start_time = when;
	end_time = start_time + (8 + 32) * 8; // preamble + access address
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalPacket::set_receive (uint8 chan, PhyModulation mod, int64 start, int64 end)
{
	is_tx = false;
	channel = chan;
	modulation = mod;
	start_time = start;
	end_time = end;
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalPacket::set_access_address (uint32 aa)
{
	access_address = aa;
	if (aa & 0x01)
	{
		preamble = 0x55;
	}
	else
	{
		preamble = 0xAA;
	}
}


////////////////////////////////////////////////////////////////////////////////

void PhysicalPacket::set_pdu (uint8 len, uint8 *pdu)
{
	if (len > maximum_pdu_length)
	{
		len = maximum_pdu_length;
	}

	pdu_length = len;
	memcpy (pdu_data, pdu, pdu_length);
	crc = 0x000000;
	end_time = start_time + (8 + 32 + 24 + 8 * pdu_length); // preamble, access address, crc
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalPacket::set_llsm (int index)
{
	llsm_index = index;
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalPacket::log (void)
{
	log_continuation ("PHY{");
	
	if (modulation == GFSK_LE)
	{
		log_continuation ("LE:");
	}
	else
	{
		log_continuation ("??:");
	}

	log_continuation ("[%d]", llsm_index);

	if (is_tx)
	{
		log_continuation ("tx@%016lld#%lld:%d:%08lx:", start_time, end_time - start_time, channel, access_address);
		for (int index = 0; index < pdu_length; index ++)
		{
			log_continuation ("%02X", pdu_data[index]);
		}
		log_continuation ("}");
	}
	else
	{
		log_continuation ("rx@%016lld#%lld:%d:%08lx", start_time, end_time - start_time, channel, access_address);
		log_continuation ("}");
	}
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalPacket::end_of_packet (int64 when, int rx_len, uint8 *rx_data)
{
	physical_layer->end_of_packet (this, when, rx_len, rx_data);
}

////////////////////////////////////////////////////////////////////////////////

void start_physical_layer_simulation (void)
{
	pthread_t t2;


	pthread_mutex_init (&physical_layer_mutex, NULL);

	pthread_create (&t2, NULL, &PhysicalLayer::physical_layer_simulation_thread, 0);
}

////////////////////////////////////////////////////////////////////////////////

PhysicalLayer::PhysicalLayer ()
{
	log (LOG_PHYSICALLAYER, "PhysicalLayer");

	enter_mutex (__FILE__, __LINE__);

	pred = 0;
	succ = all_radios;
	if (all_radios)
	{
		all_radios->pred = this;
	}
	all_radios = this;

	physical_layer_is_active = false;
	current_packet = 0;

	reset ();

	leave_mutex (__FILE__, __LINE__);
}

////////////////////////////////////////////////////////////////////////////////

PhysicalLayer::~PhysicalLayer ()
{
	log (LOG_PHYSICALLAYER, "~PhysicalLayer");

	enter_mutex (__FILE__, __LINE__);

	if (pred)
	{
		pred->succ = succ;
	}
	else
	{
		all_radios = succ;
	}

	if (succ)
	{
		succ->pred = pred;
	}

	leave_mutex (__FILE__, __LINE__);
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalLayer::reset (void)
{
	log (LOG_PHYSICALLAYER, "Physical::reset");
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalLayer::mk_active (void)
{
	physical_layer_is_active = true;
}

////////////////////////////////////////////////////////////////////////////////

void *PhysicalLayer::physical_layer_simulation_thread (void *arg)
{
	PhysicalLayer *phy;
	PhysicalLayer *next_phy;
	PhysicalPacket *packet;
	PhysicalPacket *next_packet;
	PhysicalPacket *receiver;
	PhysicalPacket *next_receiver;
	int64 time_until_next_event;
	int64 end_time;


	while (true)
	{
		enter_mutex (__FILE__, __LINE__);
		
		ordered_transmitters = 0;
		ordered_receivers = 0;

		phy = all_radios;
		while (phy)
		{
			next_phy = phy->succ;

			if (phy->is_active ())
			{
				if (phy->current_packet)
				{
					packet = phy->current_packet;
				}
				else
				{
					packet = phy->get_next_packet (physical_clock);

					if (packet)
					{
						phy->current_packet = packet;
					}
				}

				if (packet)
				{
					if (packet->is_transmit ())
					{
						insert_into (&ordered_transmitters, packet);
					}
					else if (packet->is_receive ())
					{
						insert_into (&ordered_receivers, packet);
					}
				}
			}

			phy = next_phy;
		}

		time_until_next_event = 12500;

		if (ordered_transmitters)
		{
			time_until_next_event = 1250;
			packet = ordered_transmitters;
			while (packet)
			{
				next_packet = packet->succ;
				if (packet->end_time == physical_clock)
				{
					if (!PhysicalLayer::bad_transmission[packet->get_channel ()])
					{
						log_start (LOG_PHYSICALLAYER, "  COMPLETED ");
						log_continuation ("%d,%d ", PhysicalLayer::transmitting[packet->get_channel ()], PhysicalLayer::bad_transmission[packet->get_channel ()]);
						packet->log ();
						log_end ();

						receiver = ordered_receivers;
						while (receiver)
						{
							next_receiver = receiver->succ;

							if
							(
								(receiver->start_time <= packet->start_time) &&
								(receiver->end_time >= packet->start_time + (8 + 32)) &&
								(receiver->get_channel () == packet->get_channel ())
							)
							{
								receiver->end_of_packet (physical_clock, packet->pdu_length, packet->pdu_data);
							}

							receiver = next_receiver;
						}
					}
					else
					{
						log_start (LOG_PHYSICALLAYER, "  ** BAD ** ");
						log_continuation ("%d,%d ", PhysicalLayer::transmitting[packet->get_channel ()], PhysicalLayer::bad_transmission[packet->get_channel ()]);
						packet->log ();
						log_end ();
					}

					packet->end_of_packet (physical_clock, 0, 0);

					PhysicalLayer::transmitting[packet->get_channel ()] --;

					if (PhysicalLayer::transmitting[packet->get_channel ()] == 0)
					{
						PhysicalLayer::bad_transmission[packet->get_channel ()] = false;
					}

					if (1 < time_until_next_event)
					{
						time_until_next_event = 1;
					}
				}
				else if ((packet->start_time < physical_clock) && (packet->end_time > physical_clock))
				{
					if (packet->end_time - physical_clock < time_until_next_event)
					{
						time_until_next_event = packet->end_time - physical_clock;
					}
				}
				else if (packet->start_time == physical_clock)
				{
					PhysicalLayer::transmitting[packet->get_channel ()] ++;

					if (PhysicalLayer::transmitting[packet->get_channel ()] >= 2)
					{
						PhysicalLayer::bad_transmission[packet->get_channel ()] = true;
					}

					if (packet->end_time - physical_clock < time_until_next_event)
					{
						time_until_next_event = packet->end_time - physical_clock;
					}
				}
				else if (packet->start_time > physical_clock)
				{
					if (packet->start_time - physical_clock < time_until_next_event)
					{
						time_until_next_event = packet->start_time - physical_clock;
					}
				}
				packet = next_packet;
			}
		}

		if (ordered_receivers)
		{
			packet = ordered_receivers;
			while (packet)
			{
				next_packet = packet->succ;

				if (packet->end_time == physical_clock)
				{
					log (LOG_PHYSICALLAYER, "  Rx End %lld %p", physical_clock, packet);

					packet->end_of_packet (physical_clock, 0, 0);

					if (1 < time_until_next_event)
					{
						time_until_next_event = 1;
					}
				}
				else if ((packet->start_time < physical_clock) && (packet->end_time > physical_clock))
				{
					if (packet->end_time - physical_clock < time_until_next_event)
					{
						time_until_next_event = packet->end_time - physical_clock;
					}
				}
				else if (packet->start_time == physical_clock)
				{
					log (LOG_PHYSICALLAYER, "  Rx Start %lld %p", physical_clock, packet);

					if (packet->end_time - physical_clock < time_until_next_event)
					{
						time_until_next_event = packet->end_time - physical_clock;
					}
				}
				else if (packet->start_time > physical_clock)
				{
					if (packet->start_time - physical_clock < time_until_next_event)
					{
						time_until_next_event = packet->start_time - physical_clock;
					}
				}

				packet = next_packet;
			}
		}

		physical_clock += time_until_next_event;

		leave_mutex (__FILE__, __LINE__);

		usleep (time_until_next_event + 1000 + 10);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalLayer::end_of_packet (PhysicalPacket *packet, int64 when, int rx_len, uint8 *rx_data)
{
	current_packet = 0;
	((LinkLayer *) this)->end_of_packet (packet, when, rx_len, rx_data);
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalLayer::enter_mutex (const char *file, int line)
{
	pthread_mutex_lock (&physical_layer_mutex);	
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalLayer::leave_mutex (const char *file, int line)
{
	pthread_mutex_unlock (&physical_layer_mutex);
}

////////////////////////////////////////////////////////////////////////////////

void PhysicalLayer::insert_into (PhysicalPacket **list, PhysicalPacket *packet)
{
	PhysicalPacket *p;
	PhysicalPacket *last_p;


	if (*list == 0)
	{
		*list = packet;
		packet->succ = 0;
	}
	else if ((*list)->start_time >= packet->start_time)
	{
		packet->succ = *list;
		*list = packet;
	}
	else if ((*list)->succ == 0)
	{
		(*list)->succ = packet;
		packet->succ = 0;
	}
	else
	{
		last_p = *list;
		p = (*list)->succ;
		while (p)
		{
			if (packet->start_time < p->start_time)
			{
				break;
			}
			last_p = p;
			p = p->succ;
		}

		packet->succ = last_p->succ;
		last_p->succ = packet;		
	}
}

////////////////////////////////////////////////////////////////////////////////
