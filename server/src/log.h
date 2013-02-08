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

enum DebugLog
{
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_SOCKET,
	LOG_WEBSOCKET,
	LOG_CLIENTSOCKET,
	LOG_LISTENSOCKET,
	LOG_CONTROLLER,
	LOG_LOWERHCI,
	LOG_LINKLAYER,
	LOG_LLSM,
	LOG_PHYSICALLAYER,
};

////////////////////////////////////////////////////////////////////////////////

extern void enable_logging_of (DebugLog val);
extern void disable_logging_of (DebugLog val);

////////////////////////////////////////////////////////////////////////////////

void log (DebugLog val, const char *format, ...);
void log_start (DebugLog val, const char *format, ...);
void log_continuation (const char *format, ...);
void log_end ();

////////////////////////////////////////////////////////////////////////////////
