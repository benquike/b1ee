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
#include <string.h>
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

static time_t program_start_time;

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

void on_hci_connection (int sockfd, unsigned long addr, unsigned int port)
{
	Controller *controller;

	controller = new Controller (sockfd, addr, port);
	controller->mk_active ();
}

////////////////////////////////////////////////////////////////////////////////

void on_web_connection (int sockfd, unsigned long addr, unsigned int port)
{
	WebSocket *web;

	web = new WebSocket (sockfd, addr, port);
}

////////////////////////////////////////////////////////////////////////////////

bool server_uptime (WebRequest *req)
{
	char buffer[100];
	time_t now;
	long uptime;
	

	time (&now);
	uptime = now - program_start_time;

	req->add_response_part ("page_right", "Uptime = ${uptime}");
	req->add_response_part ("page_left", "");

	sprintf (buffer, "${page_layout}");

	req->set_response_code (200);
	req->add_template_response (buffer, strlen (buffer));

	return true;
}

////////////////////////////////////////////////////////////////////////////////

const char *part_uptime (WebRequest *req)
{
	static char buffer[100];
	time_t now;
	long uptime;
	long seconds;
	long minutes;
	long hours;
	long days;
	

	time (&now);
	uptime = now - program_start_time;

	seconds = uptime % 60;
	uptime = (uptime - seconds) / 60;
	minutes = uptime % 60;
	uptime = (uptime - minutes) / 60;
	hours = uptime % 24;
	uptime = (uptime - hours) / 24;
	days = uptime;

	sprintf (buffer, "%ld days %02ld:%02ld:%02ld", days, hours, minutes, seconds);

	return buffer;
}

////////////////////////////////////////////////////////////////////////////////

const char *part_hit_count (WebRequest *req)
{
	static int count = 0;
	static char buffer[20];

	sprintf (buffer, "%d", count);
	count += 1;

	return buffer;
}

////////////////////////////////////////////////////////////////////////////////

long get_program_start_time (void)
{
	return program_start_time;
}

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv)
{
	ListenSocket *hci_listen;
	ListenSocket *web_listen;
	struct tm *timeinfo;
	char *timestr;

	enable_logging_of (LOG_INFO);
	enable_logging_of (LOG_WARNING);
	enable_logging_of (LOG_ERROR);
//	enable_logging_of (LOG_SOCKET);
	enable_logging_of (LOG_WEBSOCKET);
//	enable_logging_of (LOG_LISTENSOCKET);
//	enable_logging_of (LOG_CLIENTSOCKET);		
//	enable_logging_of (LOG_CONTROLLER);
//	enable_logging_of (LOG_LOWERHCI);		
//	enable_logging_of (LOG_LINKLAYER);
//	enable_logging_of (LOG_LLSM);
//	enable_logging_of (LOG_PHYSICALLAYER);

	time (&program_start_time);
	srand (program_start_time);
	timeinfo = localtime (&program_start_time);
	timestr = asctime (timeinfo);
	timestr[24] = 0;

	log (LOG_INFO, "-----------------------------------------------------------------------------");
	log (LOG_INFO, "%s%s", "Bluetooth Low Energy Virtual Controller Server @ ", timestr);
	log (LOG_INFO, "-----------------------------------------------------------------------------");

	WebRequest::register_page ("/server/uptime", server_uptime);
	WebRequest::register_part ("hit_count", part_hit_count);
	WebRequest::register_part ("uptime", part_uptime);

	start_background_monitor ((void *) argv[0]);
	start_physical_layer_simulation ();

	hci_listen = new ListenSocket (0xb1ee);
	hci_listen->set_callback (on_hci_connection);

	web_listen = new ListenSocket (0xb1ed);
	web_listen->set_callback (on_web_connection);
	
	while (Socket::poll ())
	{
	}
}

////////////////////////////////////////////////////////////////////////////////
