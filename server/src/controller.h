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

#include "types.h"
#include "socket.h"

////////////////////////////////////////////////////////////////////////////////

const int maximum_pdu_length = 31 + 6 + 2; // 31 data length + 6 advertising address + header + length
const int maximum_radio_channels = 40;
const int maximum_advertising_data_length = 31;
const int maximum_scan_response_data_length = 31;
const int maximum_features_page_number = 4;
const int maximum_number_of_white_list_entries = 1;
const int maximum_number_of_link_layer_state_machines = 2;
const uint32 advertising_access_address = 0x8E89BED6;

////////////////////////////////////////////////////////////////////////////////

void start_physical_layer_simulation (void);

////////////////////////////////////////////////////////////////////////////////

enum PhysicalPacketState
{
	PPS_Advertise,
	PPS_Scan,
};

////////////////////////////////////////////////////////////////////////////////

enum PhyModulation
{
	GFSK_LE
};

class PhysicalLayer;

////////////////////////////////////////////////////////////////////////////////

class PhysicalPacket
{
	friend class PhysicalLayer;
public:
	PhysicalPacket (PhysicalLayer *phy);
	~PhysicalPacket ();

	void set_transmit (uint8 chan, PhyModulation mod, int64 when);
	void set_receive (uint8 chan, PhyModulation mod, int64 start, int64 end);
	void set_access_address (uint32 aa);
	void set_pdu (uint8 len, uint8 *pdu);
	void set_llsm (int index);

	bool is_transmit (void) { return is_tx; };
	bool is_receive (void) { return !is_tx; };
	uint8 get_channel (void) { return channel; };
	int get_llsm (void) { return llsm_index; };

	void log (void);
	void end_of_packet (int64 when, int rx_len, uint8 *rx_data);

private:
	bool is_tx; // true = Transmit, false = Receive
	uint8 channel;
	PhyModulation modulation;
	uint64 start_time, end_time;
	uint8 preamble;
	uint32 access_address;
	uint8 pdu_length;
	uint8 pdu_data[maximum_pdu_length];
	uint32 crc;
	int llsm_index;

	PhysicalLayer *physical_layer;
	PhysicalPacket *succ;

};

////////////////////////////////////////////////////////////////////////////////

class PhysicalLayer
{
public:

	PhysicalLayer ();
	virtual ~PhysicalLayer ();

	void reset (void);

	void mk_active (void);

	static void *physical_layer_simulation_thread (void *arg);

	virtual PhysicalPacket *get_next_packet (int64 after) = 0;
	void end_of_packet (PhysicalPacket *packet, int64 when, int rx_len, uint8 *rx_data);

	static void enter_mutex (const char *file, int line);
	static void leave_mutex (const char *file, int line);

private:

	bool physical_layer_is_active;
	bool is_active (void) { return physical_layer_is_active; };

	PhysicalPacket *current_packet;

	static PhysicalLayer *all_radios;

	PhysicalLayer *pred;
	PhysicalLayer *succ;

	static void insert_into (PhysicalPacket **list, PhysicalPacket *packet);

	static PhysicalPacket *ordered_transmitters;
	static PhysicalPacket *ordered_receivers;

	static int transmitting[maximum_radio_channels];
	static bool bad_transmission[maximum_radio_channels];

};

////////////////////////////////////////////////////////////////////////////////

enum LinkLayerState
{
	LLS_Idle,
	LLS_Advertising,
	LLS_Scanning,
	LLS_Initiator,
	LLS_Slave,
	LLS_Master
};

////////////////////////////////////////////////////////////////////////////////

enum Advertising_SubStates
{
	ASS_Advertise,
	ASS_Advertise_Request,
	ASS_Advertise_Response,
};

////////////////////////////////////////////////////////////////////////////////

enum Scanning_SubStates
{
	SSS_Scan,
	SSS_Scan_Request,
	SSS_Scan_Response,
};

////////////////////////////////////////////////////////////////////////////////

class LinkLayerStateMachine
{
	friend class LinkLayer;
public:

	LinkLayerStateMachine ();
	~LinkLayerStateMachine ();

	void reset (void);

	void mk_idle (void);
	void mk_advertiser (int64 after);
	void mk_scanner (int64 after);

	int64 determine_next_packet_time (void);
	PhysicalPacket *create_next_packet (void);

private:

	LinkLayerState state;

	union
	{
		struct
		{
			Advertising_SubStates substate;
			int64 ll_next_advertising_instant; // when is the next 0
			int64 ll_next_advertising_tx; // when is the next transmission
			int ll_advertising_channel; // 0, 1, 2 ... interval ... 0, 1, 2 ... 
		} adv;

		struct
		{
			Scanning_SubStates substate;
			int64 ll_next_scanning_instant;
			int ll_scanning_channel;
		} scan;
	};

};

////////////////////////////////////////////////////////////////////////////////

class LinkLayer : public PhysicalLayer
{
public:

	LinkLayer ();
	virtual ~LinkLayer ();

	void reset (void);

	uint64 ll_get_bd_addr (void);
	void ll_set_bd_addr (uint64 bd_addr);

	uint8 ll_get_version (void);
	uint16 ll_get_subversion (void);
	uint8 ll_get_maximum_features_page_number (void);
	uint64 ll_get_extended_features (uint8 page_number);
	uint64 ll_get_le_features (void);
	void ll_set_host_supports (int le_supported_host, int simultaneous_le_host);
	int ll_get_maximum_number_of_white_list_entries (void);
	uint64 ll_get_supported_states (void);
	void ll_set_advertising_parameters (int advertising_interval_min, int advertising_interval_max, int advertising_type, int own_address_type, int direct_address_type, uint64 direct_address, int advertising_channel_map, int advertising_filter_policy);
	void ll_set_advertising_data (int len, char *data);
	void ll_set_scan_response_data (int len, char *data);
	bool ll_set_advertising_enable (int enable);
	void ll_set_scan_parameters (int scan_type, int scan_interval, int scan_window, int own_address_type, int scanning_filter_policy);
	bool ll_set_scan_enable (int enable, int filter_duplicates);

	virtual PhysicalPacket *get_next_packet (int64 after);
	void end_of_packet (PhysicalPacket *packet, int64 when, int rx_len, uint8 *rx_data);

	virtual void send_le_advertising_report_event (int rx_len, uint8 *rx_data) = 0;

	virtual void set_delete_ready (void) = 0;
	virtual bool is_delete_pending (void) = 0;

private:

	int64 last_clock;

	uint64 ll_bd_addr;
	uint64 lmp_features[maximum_features_page_number];
	uint64 le_features;
	uint64 ll_supported_states;

	PhysicalPacket *ll_packet;

	int ll_advertising_interval_min;
	int ll_advertising_interval_max;
	int ll_advertising_type;
	int ll_advertising_own_address_type;
	int ll_direct_address_type;
	uint64 ll_direct_address;
	int ll_advertising_channel_map;
	int ll_advertising_filter_policy;

	int ll_advertising_data_length;
	char ll_advertising_data[maximum_advertising_data_length];

	int ll_scan_response_data_length;
	char ll_scan_response_data[maximum_scan_response_data_length];

	int ll_advertising_enabled;

	int ll_scan_type;
	int ll_scan_interval;
	int ll_scan_window;
	int ll_scan_own_address_type;
	int ll_scanning_filter_policy;
	int ll_scanning_enabled;
	int ll_scan_filter_duplicates;

	int last_machine;
	LinkLayerStateMachine machine[maximum_number_of_link_layer_state_machines];

};

////////////////////////////////////////////////////////////////////////////////

class LowerHCI : public LinkLayer
{
public:

	LowerHCI ();
	virtual ~LowerHCI ();

	void reset (void);

	void hci_set_event_mask_command (int parameter_len, char *parameters);
	void hci_reset_command (int parameter_len, char *parameters);
	void hci_write_le_host_supported_command (int parameter_len, char *parameters);
	void hci_read_local_version_information_command (int parameter_len, char *parameters);
	void hci_read_local_supported_commands_command (int parameter_len, char *parameters);
	void hci_read_local_supported_features_command (int parameter_len, char *parameters);
	void hci_read_local_extended_features_command (int parameter_len, char *parameters);
	void hci_read_buffer_size_command (int parameter_len, char *parameters);
	void hci_read_bd_addr_command (int parameter_len, char *parameters);
	void hci_le_set_event_mask_command (int parameter_len, char *parameters);
	void hci_le_read_buffer_size_command (int parameter_len, char *parameters);
	void hci_le_read_local_supported_features_command (int parameter_len, char *parameters);
	void hci_le_set_advertising_parameters_command (int parameter_len, char *parameters);
	void hci_le_read_advertising_channel_tx_power_command (int parameter_len, char *parameters);
	void hci_le_set_advertising_data_command (int parameter_len, char *parameters);
	void hci_le_set_scan_response_data_command (int parameter_len, char *parameters);
	void hci_le_set_advertise_enable_command (int parameter_len, char *parameters);
	void hci_le_set_scan_parameters_command (int parameter_len, char *parameters);
	void hci_le_set_scan_enable_command (int parameter_len, char *parameters);
	void hci_le_read_white_list_size_command (int parameter_len, char *parameters);
	void hci_le_read_supported_states_command (int parameter_len, char *parameters);
	void hci_unsupported_command (int opcode);
	
	void process_command (int opcode, int parameter_len, char *parameters);

	virtual void write_data (char *buffer, int len) = 0;
	void send_event (int opcode, int parameter_len, char *parameters);
	void send_command_complete_event (int command_opcode, int parameter_len, char *parameters);

	virtual void send_le_advertising_report_event (int rx_len, uint8 *rx_data);

	uint8 hci_get_version (void);
	uint16 hci_get_revision (void);
	uint16 hci_get_manufacturer (void);

private:
	int num_hci_command_packets;
	uint64 hci_event_mask;
	uint64 hci_le_event_mask;

	int hci_le_acl_data_packet_length;
	int hci_total_num_le_acl_data_packets;

	int hci_acl_data_packet_length;
	int hci_synchronous_data_packet_length;
	int hci_total_num_acl_data_packets;
	int hci_total_num_synchronous_data_packets;

};

////////////////////////////////////////////////////////////////////////////////

const uint8 HCI_COMMAND = 0x01;
const uint8 HCI_DATA = 0x02;
const uint8 HCI_EVENT = 0x04;

class Controller : public ClientSocket, public LowerHCI
{
public:

	Controller (int sockfd, unsigned long addr, unsigned int port);
	virtual ~Controller ();

	virtual void on_readable (void);
	virtual void write_data (char *buffer, int len);

	virtual void set_delete_ready (void);
	virtual bool is_delete_pending (void);

};

////////////////////////////////////////////////////////////////////////////////
