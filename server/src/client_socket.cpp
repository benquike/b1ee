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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

////////////////////////////////////////////////////////////////////////////////

#include "log.h"
#include "socket.h"

#define BUFFER_SIZE (64*1024)

////////////////////////////////////////////////////////////////////////////////

ClientSocket::ClientSocket (int new_sockfd, unsigned long new_addr, unsigned int new_port)
{
	sockfd = new_sockfd;
	addr = new_addr;
	port = new_port;

	read_buffer_len = 0;
	read_buffer_size = 0;
	read_buffer = 0;

	write_buffer_len = 0;
	write_buffer_size = 0;
	write_buffer = 0;

	log (LOG_CLIENTSOCKET, "ClientSocket %s", get_name ());
}

////////////////////////////////////////////////////////////////////////////////

ClientSocket::~ClientSocket ()
{
	if (read_buffer)
	{
		free (read_buffer);
	}

	if (write_buffer)
	{
		free (write_buffer);
	}

	log (LOG_CLIENTSOCKET, "~ClientSocket %s", get_name ());
}

////////////////////////////////////////////////////////////////////////////////

bool ClientSocket::is_readable (void)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ClientSocket::is_writable (void)
{
	if ((write_buffer) && (write_buffer_len > 0))
	{
		log (LOG_CLIENTSOCKET, "ClientSocket is_writable %p %d", write_buffer, write_buffer_len);
		return true;
	}
	else
	{
		log (LOG_CLIENTSOCKET, "ClientSocket !is_writable %p %d", write_buffer, write_buffer_len);
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////

void ClientSocket::on_readable (void)
{
	int err;


	if (read_buffer_size - read_buffer_len < BUFFER_SIZE)
	{
		read_buffer_size += BUFFER_SIZE;
		read_buffer = (char *) realloc (read_buffer, read_buffer_size);
	}

	err = recv (sockfd, &read_buffer[read_buffer_len], read_buffer_size - read_buffer_len, 0);

	log (LOG_CLIENTSOCKET, "ClientSocket::on_readable (%d)", err);

	if (err < 0)
	{
		log (LOG_ERROR, "recv (%d : %s)", errno, strerror (errno));
		return;
	}

	if (err == 0)
	{
		this->set_delete_pending ();
		return;
	}

	read_buffer_len += err; // err is actually the length of data read
}

////////////////////////////////////////////////////////////////////////////////

void ClientSocket::on_writable (void)
{
	int err;
	int len;


	log (LOG_CLIENTSOCKET, "ClientSocket::on_writable (%d)", write_buffer_len);

	len = write_buffer_len;

	if (len > BUFFER_SIZE)
	{
		len = BUFFER_SIZE;
	}

	err = send (sockfd, write_buffer, len, 0);
	
	if (err < 0)
	{
		log (LOG_ERROR, "ERROR send (%d : %s)", errno, strerror (errno));
		return;
	}

	memcpy (write_buffer, &write_buffer[err], write_buffer_size - err);
	write_buffer_len -= err;
}

////////////////////////////////////////////////////////////////////////////////

char *ClientSocket::peek_read_buffer (int *len)
{
	*len = read_buffer_len;
	return read_buffer;
}

////////////////////////////////////////////////////////////////////////////////

void ClientSocket::consume_read_buffer (int len)
{
	log (LOG_CLIENTSOCKET, "ClientSocket::consume_read_buffer (%d)", len);

	if ((len > 0) && (len <= read_buffer_len))
	{
		memmove (read_buffer, &read_buffer[len], read_buffer_size - len);
		read_buffer_len -= len;
	}
}

////////////////////////////////////////////////////////////////////////////////

void ClientSocket::write_data (char *buffer, int len)
{
	log_start (LOG_CLIENTSOCKET, "ClientSocket::write_data (%d : %p) ", len, buffer);
	for (int index = 0; index < len; index ++)
	{
		log_continuation ("%02X", buffer[index]);
	}
	log_end ();

	if (write_buffer_size - write_buffer_len < len)
	{
		write_buffer_size = write_buffer_len + len;
		write_buffer = (char *) realloc (write_buffer, write_buffer_size);
	}

	memcpy (&write_buffer[write_buffer_len], buffer, len);
	write_buffer_len += len;

	write (ListenSocket::get_write_pipefd (), " ", 1);
}

////////////////////////////////////////////////////////////////////////////////

char *ClientSocket::get_name (void)
{
	static char buffer[100];


	sprintf (buffer, "ClientSocket{%08lX:%04X}", addr, port); 
	return buffer;
}

////////////////////////////////////////////////////////////////////////////////

