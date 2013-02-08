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
