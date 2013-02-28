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
#include <stdarg.h>
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////

#include "log.h"

////////////////////////////////////////////////////////////////////////////////

class LoggerLock
{
	pthread_mutex_t mutex;

public:
	LoggerLock ();
	void lock (void);
	void unlock (void);
};

static LoggerLock logger_lock;

////////////////////////////////////////////////////////////////////////////////

LoggerLock::LoggerLock ()
{
	pthread_mutex_init (&mutex, NULL);
}

////////////////////////////////////////////////////////////////////////////////

void LoggerLock::lock ()
{
	pthread_mutex_lock (&mutex);
}

////////////////////////////////////////////////////////////////////////////////

void LoggerLock::unlock (void)
{
	pthread_mutex_unlock (&mutex);
}

////////////////////////////////////////////////////////////////////////////////

int log_enabled = 0;
int inside_val = 0;

const int max_log_buffer_size = 1024;

////////////////////////////////////////////////////////////////////////////////

void enable_logging_of (DebugLog val)
{
	logger_lock.lock ();
	log_enabled |= (1 << val);
	logger_lock.unlock ();
}

////////////////////////////////////////////////////////////////////////////////

void disable_logging_of (DebugLog val)
{
	logger_lock.lock ();
	log_enabled &= ~(1 << val);
	logger_lock.unlock ();
}

////////////////////////////////////////////////////////////////////////////////

const char *log_type_string (DebugLog val)
{
	const char *log_type;

	switch (val)
	{
		case LOG_INFO:           log_type = "Info    : "; break;
		case LOG_WARNING:        log_type = "Warning : "; break;
		case LOG_ERROR:          log_type = "ERROR * : "; break;
		case LOG_SOCKET:         log_type = "Socket  : "; break;
		case LOG_WEBSOCKET:      log_type = "WebSock : "; break;
		case LOG_CLIENTSOCKET:   log_type = "Client  : "; break;
		case LOG_LISTENSOCKET:   log_type = "Listen  : "; break;
		case LOG_CONTROLLER:     log_type = "cntr    : "; break;
		case LOG_LOWERHCI:       log_type = "lowHCI  : "; break;
		case LOG_LINKLAYER:      log_type = "LL      : "; break;
		case LOG_LLSM:           log_type = "LLSM    : "; break;
		case LOG_PHYSICALLAYER:  log_type = "PHY     : "; break;

		default:
			log_type = "Unknown";
			break;
	}

	return log_type;
}

////////////////////////////////////////////////////////////////////////////////

void log (DebugLog val, const char *format, ...)
{
    static char log_buffer[max_log_buffer_size];
    const char *log_type;

	if ((1 << val) & log_enabled)
	{
		logger_lock.lock ();

	    va_list ap;
	    va_start(ap, format);

		vsnprintf (log_buffer, max_log_buffer_size, format, ap);

		printf ("%s%s\n", log_type_string (val), log_buffer);

		logger_lock.unlock ();
	}
}

////////////////////////////////////////////////////////////////////////////////

void log_start (DebugLog val, const char *format, ...)
{
    static char log_buffer[max_log_buffer_size];
    const char *log_type;

	logger_lock.lock ();

	inside_val = val;	

	if ((1 << val) & log_enabled)
	{
		va_list ap;
		va_start(ap, format);

		vsnprintf (log_buffer, max_log_buffer_size, format, ap);

		printf ("%s%s", log_type_string (val), log_buffer);
	}
}

////////////////////////////////////////////////////////////////////////////////

void log_continuation (const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

	if ((1 << inside_val) & log_enabled)
	{
		vprintf (format, ap);
	}
}

////////////////////////////////////////////////////////////////////////////////

void log_end (void)
{
	if ((1 << inside_val) & log_enabled)
	{
		printf ("\n");
	}

	logger_lock.unlock ();
}

////////////////////////////////////////////////////////////////////////////////
