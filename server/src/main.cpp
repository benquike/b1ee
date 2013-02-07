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

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////

#include "controller.h"
#include "socket.h"
#include "log.h"

////////////////////////////////////////////////////////////////////////////////

void *background_monitor_thread (void *arg)
{
	struct stat st;
	int last_modified_time;
	int err;
	bool monitoring;
	char *program_name;


	program_name = (char *) arg;
	err = stat ((char *) program_name, &st);
	if (err >= 0)
	{
		last_modified_time = st.st_mtime;
		monitoring = true;
	}
	else
	{
		monitoring = false;
	}

	while (monitoring)
	{
		err = stat (program_name, &st);

		if (err >= 0)
		{
			if (st.st_mtime != last_modified_time)
			{
				monitoring = false;
			}
			usleep (100000);			
		}
		else
		{
			monitoring = false;
		}
	}

	log (LOG_WARNING, "executable updated - exiting");
	exit (0);
}

////////////////////////////////////////////////////////////////////////////////

void start_background_monitor (void *arg)
{
	pthread_t t1;


	pthread_create (&t1, NULL, &background_monitor_thread, arg);
}

////////////////////////////////////////////////////////////////////////////////

void on_connection (int sockfd, unsigned long addr, unsigned int port)
{
	Controller *controller;

	controller = new Controller (sockfd, addr, port);
	controller->mk_active ();
}

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv)
{
	ListenSocket *listen;
	time_t rawtime;
	struct tm *timeinfo;
	char *timestr;


	enable_logging_of (LOG_INFO);
	enable_logging_of (LOG_WARNING);
	enable_logging_of (LOG_ERROR);
//	enable_logging_of (LOG_SOCKET);
//	enable_logging_of (LOG_LISTENSOCKET);
//	enable_logging_of (LOG_CLIENTSOCKET);		
//	enable_logging_of (LOG_CONTROLLER);
//	enable_logging_of (LOG_LOWERHCI);		
//	enable_logging_of (LOG_LINKLAYER);
//	enable_logging_of (LOG_LLSM);
//	enable_logging_of (LOG_PHYSICALLAYER);

	time (&rawtime);
	srand (rawtime);
	timeinfo = localtime (&rawtime);
	timestr = asctime (timeinfo);
	timestr[24] = 0;

	log (LOG_INFO, "-----------------------------------------------------------------------------");
	log (LOG_INFO, "%s%s", "Bluetooth Low Energy Virtual Controller Server @ ", timestr);
	log (LOG_INFO, "-----------------------------------------------------------------------------");
	
	start_background_monitor ((void *) argv[0]);
	start_physical_layer_simulation ();

	listen = new ListenSocket (0xb1ee);
	listen->set_callback (on_connection);
	
	while (Socket::poll ())
	{
	}
}

////////////////////////////////////////////////////////////////////////////////
