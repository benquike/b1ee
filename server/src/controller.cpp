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

#include "controller.h"
#include "log.h"

////////////////////////////////////////////////////////////////////////////////

Controller::Controller (int sockfd, unsigned long addr, unsigned int port) :
	ClientSocket (sockfd, addr, port)
{
	log (LOG_CONTROLLER, "Controller");

	ll_set_bd_addr ((addr << 16) | port);
}

////////////////////////////////////////////////////////////////////////////////

Controller::~Controller ()
{
	log (LOG_CONTROLLER, "~Controller");
}

////////////////////////////////////////////////////////////////////////////////

void Controller::on_readable (void)
{
	int len;
	int packet_type;
	char *buffer;


	log (LOG_CONTROLLER, "Controller::on_readable %s", get_name ());

	// do the actual socket read into a buffer
	ClientSocket::on_readable ();

	while (true)
	{
		buffer = peek_read_buffer (&len);

		if (len > 1)
		{
			packet_type = buffer[0];

			if (packet_type == HCI_COMMAND)
			{
				int opcode;
				int command_len;

				if (len > 3)
				{
					opcode = (buffer[1]) + (buffer[2] << 8);
					command_len = buffer[3];
				
					if (command_len + 4 >= len)
					{
						if (command_len == 0)
						{
							process_command (opcode, command_len, 0);
						}
						else
						{
							process_command (opcode, command_len, &buffer[4]);
						}

						consume_read_buffer (command_len + 4);
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else if (packet_type == HCI_DATA)
			{
				log (LOG_CONTROLLER, "HCI Data");
			}
			else
			{
				log (LOG_ERROR, "Invalid Packet Type %02x", packet_type);

				// TODO : should this be delete this, or should this set the delete pending flag?
				delete this;
			}
		}
		else
		{
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void Controller::write_data (char *buffer, int len)
{
	ClientSocket::write_data (buffer, len);
}

////////////////////////////////////////////////////////////////////////////////

void Controller::set_delete_ready (void)
{
	ClientSocket::set_delete_ready ();
}

////////////////////////////////////////////////////////////////////////////////

bool Controller::is_delete_pending (void)
{
	return ClientSocket::is_delete_pending ();
}

////////////////////////////////////////////////////////////////////////////////
