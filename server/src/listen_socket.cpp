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

////////////////////////////////////////////////////////////////////////////////

int ListenSocket::pipefd[2] = { 0, 0 };

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
