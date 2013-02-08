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
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

////////////////////////////////////////////////////////////////////////////////

#include "log.h"
#include "socket.h"

#define BUFFER_SIZE (64*1024)

////////////////////////////////////////////////////////////////////////////////

WebSocket::WebSocket (int new_sockfd, unsigned long new_addr, unsigned int new_port)
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

	close_pending = false;

	log (LOG_WEBSOCKET, "WebSocket %s", get_name ());
}

////////////////////////////////////////////////////////////////////////////////

WebSocket::~WebSocket ()
{
	if (read_buffer)
	{
		free (read_buffer);
	}

	if (write_buffer)
	{
		free (write_buffer);
	}

	log (LOG_WEBSOCKET, "~WebSocket %s", get_name ());
}

////////////////////////////////////////////////////////////////////////////////

bool WebSocket::is_readable (void)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool WebSocket::is_writable (void)
{
	if ((write_buffer) && (write_buffer_len > 0))
	{
		log (LOG_WEBSOCKET, "WebSocket is_writable %p %d", write_buffer, write_buffer_len);
		return true;
	}
	else
	{
		log (LOG_WEBSOCKET, "WebSocket !is_writable %p %d", write_buffer, write_buffer_len);
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////

void WebSocket::on_readable (void)
{
	int err;
	int index;
	WebRequest *request;


	if (read_buffer_size - read_buffer_len < BUFFER_SIZE)
	{
		read_buffer_size += BUFFER_SIZE;
		read_buffer = (char *) realloc (read_buffer, read_buffer_size);
	}

	err = recv (sockfd, &read_buffer[read_buffer_len], read_buffer_size - read_buffer_len, 0);

	log (LOG_WEBSOCKET, "WebSocket::on_readable (%d)", err);

	if (err < 0)
	{
		log (LOG_ERROR, "recv (%d : %s)", errno, strerror (errno));
		return;
	}

	if (err == 0)
	{
		this->set_delete_pending ();
		this->set_delete_ready ();
		return;
	}

	read_buffer_len += err; // err is actually the length of data read

	for (index = 3; index < read_buffer_len; index ++)
	{
		if
		(
			(read_buffer[index - 3] == 0x0D) &&
			(read_buffer[index - 2] == 0x0A) &&
			(read_buffer[index - 1] == 0x0D) &&
			(read_buffer[index - 0] == 0x0A)
		)
		{
			read_buffer[index - 3] = 0;

			request = new WebRequest (this, read_buffer);
			request->process ();
			delete request;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void WebSocket::on_writable (void)
{
	int err;
	int len;


	log (LOG_WEBSOCKET, "WebSocket::on_writable (%d)", write_buffer_len);

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

	if ((close_pending) && (write_buffer_len == 0))
	{
		log (LOG_WEBSOCKET, "Deleting WebSocket");
		set_delete_ready ();
		set_delete_pending ();
	}
}

////////////////////////////////////////////////////////////////////////////////

char *WebSocket::peek_read_buffer (int *len)
{
	*len = read_buffer_len;
	return read_buffer;
}

////////////////////////////////////////////////////////////////////////////////

void WebSocket::consume_read_buffer (int len)
{
	log (LOG_WEBSOCKET, "WebSocket::consume_read_buffer (%d)", len);

	if ((len > 0) && (len <= read_buffer_len))
	{
		memmove (read_buffer, &read_buffer[len], read_buffer_size - len);
		read_buffer_len -= len;
	}
}

////////////////////////////////////////////////////////////////////////////////

void WebSocket::write_data (char *buffer, int len)
{
	log_start (LOG_WEBSOCKET, "WebSocket::write_data (%d : %p) ", len, buffer);
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

void WebSocket::close_when_written (void)
{
	close_pending = true;
}

////////////////////////////////////////////////////////////////////////////////

char *WebSocket::get_name (void)
{
	static char buffer[100];


	sprintf (buffer, "WebSocket{%08lX:%04X}", addr, port); 
	return buffer;
}

////////////////////////////////////////////////////////////////////////////////

WebRequest::WebRequest (WebSocket *respond_to, char *request_string)
{
	int index;


	socket = respond_to;
	request = request_string;

	for (index = 0; index < maximum_number_of_request_headers; index ++)
	{
		header_keys[index] = 0;
		header_values[index] = 0;
	}

	method = 0;
	url = 0;
	protocol = 0;

	request_to_headers ();
}

////////////////////////////////////////////////////////////////////////////////

WebRequest::~WebRequest ()
{
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::request_to_headers (void)
{
	int index;
	char *ptr;


	ptr = request;

	index = 0;
	method = ptr;

	while ((*ptr) && (*ptr != 0x0A) && (*ptr != 0x0D))
	{
		if (*ptr == ' ')
		{
			*ptr = 0;
			ptr ++;
			while ((*ptr) && (*ptr == ' '))
			{
				ptr ++;
			}
			
			url = ptr;
			break;
		}
		else
		{
			ptr ++;
		}
	}

	while ((*ptr) && (*ptr != 0x0A) && (*ptr != 0x0D))
	{
		if (*ptr == ' ')
		{
			*ptr = 0;
			ptr ++;
			while ((*ptr) && (*ptr == ' '))
			{
				ptr ++;
			}

			protocol = ptr;
			break;
		}
		else
		{
			ptr ++;
		}
	}

	while ((*ptr) && (*ptr != 0x0A) && (*ptr != 0x0D))
	{
		ptr ++;
	}

	*ptr = 0;
	ptr ++;

	if ((*ptr == 0x0D) || (*ptr == 0x0A))
	{
		*ptr = 0;
	}
	ptr ++;

	header_keys[index] = ptr;

	while (*ptr)
	{
		while ((*ptr) && (*ptr != ':') && (*ptr != 0x0D) && (*ptr != 0x0A))
		{
			*ptr = tolower (*ptr);
			ptr ++;
		}

		if (*ptr == ':')
		{
			*ptr = 0;
			ptr ++;

			while ((*ptr) && (*ptr == ' '))
			{
				ptr ++;
			}

			if (index < maximum_number_of_request_headers)
			{
				header_values[index] = ptr;
			}

			index ++;
		}

		while ((*ptr) && (*ptr != 0x0A) && (*ptr != 0x0D))
		{
			ptr ++;
		}

		if (*ptr)
		{
			*ptr = 0;
			ptr ++;

			if ((*ptr == 0x0D) || (*ptr == 0x0A))
			{
				*ptr = 0;
			}
			ptr ++;

			if (index < maximum_number_of_request_headers)
			{
				header_keys[index] = ptr;
			}
		}
	}

	number_of_headers = index;
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::process (void)
{
	int index;
	static int count = 0;
	char buffer[100];


	log (LOG_WEBSOCKET, "Process: %s, %s, %s", method, url, protocol);

	for (index = 0; index < number_of_headers; index ++)
	{
		log (LOG_WEBSOCKET, "   %-12s = %s", header_keys[index], header_values[index]);
	}

	sprintf (buffer, "HTTP/1.1 200 OK\r\n\r\nCount = %d\n", count);
	count += 1;

	socket->write_data (buffer, strlen (buffer));
	socket->close_when_written ();
}

////////////////////////////////////////////////////////////////////////////////
