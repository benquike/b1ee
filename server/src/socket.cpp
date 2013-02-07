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

Socket *Socket::all_sockets = 0;

int ListenSocket::pipefd[2] = { 0, 0 };

////////////////////////////////////////////////////////////////////////////////

Socket::Socket ()
{
	Socket *sl;


	sockfd = 0;
	port = 0;
	addr = 0;

	pred = 0;
	succ = all_sockets;
	if (all_sockets)
	{
		all_sockets->pred = this;
	}
	all_sockets = this;

	log (LOG_SOCKET, "Socket::Socket");

	delete_pending = false;
	delete_ready = false;
}

////////////////////////////////////////////////////////////////////////////////

Socket::~Socket ()
{
	log (LOG_SOCKET, "Socket::~Socket");

	close (sockfd);

	if (pred)
	{
		pred->succ = succ;
	}
	else
	{
		all_sockets = succ;
	}

	if (succ)
	{
		succ->pred = pred;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool Socket::poll (void)
{
	Socket *sl;
	Socket *next_sl;

	fd_set read_set;
	fd_set write_set;
	fd_set error_set;

	int len;
	int err;
	int max_fd;

	struct timeval tv;

	char buffer[1];


	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&error_set);

	max_fd = 0;

	sl = all_sockets;
	while (sl)
	{
		next_sl = sl->succ;

		if (sl->is_active ())
		{
			log (LOG_SOCKET, " >> %p : %d : %s", sl, sl->sockfd, sl->get_name ());

			if (sl->is_readable ())
			{
				FD_SET (sl->sockfd, &read_set);
				FD_SET (sl->sockfd, &error_set);

				if (sl->sockfd > max_fd)
				{
					max_fd = sl->sockfd;
				}
			}

			if (sl->is_writable ())
			{
				FD_SET (sl->sockfd, &write_set);
				FD_SET (sl->sockfd, &error_set);

				if (sl->sockfd > max_fd)
				{
					max_fd = sl->sockfd;
				}
			}
		}

		sl = next_sl;
	}

	FD_SET (ListenSocket::get_read_pipefd (), &read_set);

	tv.tv_sec = 60;
	tv.tv_usec = 0;

	err = select (max_fd + 1, &read_set, &write_set, &error_set, &tv);

	log (LOG_SOCKET, "select = %d", err);

	if (err < 0)
	{
		log (LOG_ERROR, "select (%d : %s)", err, strerror (err));
		return false;
	}

	if (err == 0)
	{
		return true;
	}

	sl = all_sockets;
	while (sl)
	{
		next_sl = sl->succ;

		if (FD_ISSET (sl->sockfd, &read_set))
		{
			sl->on_readable ();
		}

		if (FD_ISSET (sl->sockfd, &write_set))
		{
			sl->on_writable ();
		}

		if (FD_ISSET (sl->sockfd, &error_set))
		{
			sl->set_delete_pending ();
		}

		sl = next_sl;
	}

	if (FD_ISSET (ListenSocket::get_read_pipefd (), &read_set))
	{
		err = read (ListenSocket::get_read_pipefd (), buffer, 1);
	}

	sl = all_sockets;
	while (sl)
	{
		next_sl = sl->succ;
		if (sl->is_delete_ready ())
		{
			delete sl;
		}
		sl = next_sl;
	}

	return true;
}

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

ListenSocket::ListenSocket (int listen_port)
{
	int opt;
	int err;
	struct sockaddr_in sock_addr;	

	port = listen_port;
	addr = INADDR_ANY;

	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	
	if (sockfd < 0) 
	{
		log (LOG_ERROR, "opening socket (%d : %s)", errno, strerror (errno));
		return;
	}

	opt = 1;
	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero ((char *) &sock_addr, sizeof (sock_addr));

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	sock_addr.sin_port = htons (port);
	
	if (bind (sockfd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) < 0)
	{
		log (LOG_ERROR, "on binding (%d : %s)", errno, strerror (errno));
		exit (1);
	}

	callback_func = 0;
	
	listen (sockfd, 50);

	if (pipe (pipefd) < 0)
	{
		log (LOG_ERROR, "pipe (%d : %s)", errno, strerror (errno));
		return;		
	}

	log (LOG_LISTENSOCKET, "Pipe = %d,%d", pipefd[0], pipefd[1]);

	log (LOG_LISTENSOCKET, "ListenSocket %p", this);
}

////////////////////////////////////////////////////////////////////////////////

ListenSocket::~ListenSocket ()
{
	log (LOG_LISTENSOCKET, "~ListenSocket %p", this);
}

////////////////////////////////////////////////////////////////////////////////

bool ListenSocket::is_readable (void)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ListenSocket::is_writable (void)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void ListenSocket::on_readable (void)
{
	int new_sockfd;
	socklen_t new_addrlen;
	struct sockaddr_in new_addr;
	Socket *new_socket;
	char buffer[100];


	log (LOG_LISTENSOCKET, "ListenSocket::on_readable");

	new_addrlen = sizeof (new_addr);
	new_sockfd = accept (sockfd, (struct sockaddr *) &new_addr, &new_addrlen);

	if (new_sockfd == -1)
	{
		log (LOG_ERROR, "accept (%d : %s)", errno, strerror (errno));
		return;
	}

	callback_func (new_sockfd, ntohl (new_addr.sin_addr.s_addr), ntohs (new_addr.sin_port));
}

////////////////////////////////////////////////////////////////////////////////

void ListenSocket::set_callback (void (*func)(int, unsigned long, unsigned int))
{
	callback_func = func;
}

////////////////////////////////////////////////////////////////////////////////

char *ListenSocket::get_name (void)
{
	static char buffer[100];


	sprintf (buffer, "ListenSocket{%04X}", port);
	return buffer;
}

////////////////////////////////////////////////////////////////////////////////

int ListenSocket::get_read_pipefd (void)
{
	return pipefd[0];
}

////////////////////////////////////////////////////////////////////////////////

int ListenSocket::get_write_pipefd (void)
{
	return pipefd[1];
}

////////////////////////////////////////////////////////////////////////////////
