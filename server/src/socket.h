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

#ifndef __CPP_H_SOCKET__
#define __CPP_H_SOCKET__

////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////

class Socket
{
public:

	Socket ();
	virtual ~Socket ();

	static bool poll (void);

	virtual bool is_readable (void) = 0;
	virtual bool is_writable (void) = 0;

	virtual void on_readable (void) = 0;
	virtual void on_writable (void) = 0;

	virtual char *get_name (void) = 0;

	void set_delete_pending (void) { delete_pending = true; };
	void set_delete_ready (void) { delete_ready = true; };

	bool is_active (void) { return !delete_pending; };
	bool is_delete_pending (void) { return delete_pending; };
	bool is_delete_ready (void) {return delete_pending & delete_ready; };

protected:
	int sockfd;

	unsigned int port;
	unsigned long addr;

private:
	static Socket *all_sockets;

	bool delete_pending;
	bool delete_ready;
	
	Socket *pred;
	Socket *succ;

};

////////////////////////////////////////////////////////////////////////////////

class ClientSocket : public Socket
{
public:

	ClientSocket (int client_sockfd, unsigned long addr, unsigned int port);
	virtual ~ClientSocket ();

	bool is_readable (void);
	bool is_writable (void);

	virtual void on_readable (void);
	virtual void on_writable (void);

	char *peek_read_buffer (int *len);
	void consume_read_buffer (int len);

	void write_data (char *buffer, int len);

	virtual char *get_name (void);

private:

	int read_buffer_len;	// length of the valid data in the read_buffer
	int read_buffer_size;	// how big is the read_buffer
	char *read_buffer;		// the read_buffer itself

	int write_buffer_len;
	int write_buffer_size;
	char *write_buffer;

};

////////////////////////////////////////////////////////////////////////////////

class WebSocket : public Socket
{
public:

	WebSocket (int client_sockfd, unsigned long addr, unsigned int port);
	virtual ~WebSocket ();

	bool is_readable (void);
	bool is_writable (void);

	virtual void on_readable (void);
	virtual void on_writable (void);

	char *peek_read_buffer (int *len);
	void consume_read_buffer (int len);

	void write_data (const char *buffer, int len);
	void write_string (const char *buffer);
	void close_when_written (void);

	virtual char *get_name (void);

private:

	void process_request (char *request);

	int read_buffer_len;	// length of the valid data in the read_buffer
	int read_buffer_size;	// how big is the read_buffer
	char *read_buffer;		// the read_buffer itself

	int write_buffer_len;
	int write_buffer_size;
	char *write_buffer;

	bool close_pending;

};

////////////////////////////////////////////////////////////////////////////////

const int maximum_number_of_request_headers = 64;
const int maximum_number_of_request_arguments = 64;
const int maximum_number_of_request_parts = 64;

////////////////////////////////////////////////////////////////////////////////

class WebRequest;

////////////////////////////////////////////////////////////////////////////////

class WebPage
{
public:

	WebPage (const char *path, char *filename, FILE *fp);
	WebPage (const char *path, bool (*callback_func)(WebRequest *));
	~WebPage ();

	static WebPage *find (const char *path);

	static WebPage *open (const char *path);

	bool process (WebRequest *);

private:

	const char *path;
	bool (*callback_func)(WebRequest *);
	char *filename;

	struct timespec last_modified;
	int html_len;
	char *html_file;

	static WebPage *all_webpages;

	WebPage *succ;

};

////////////////////////////////////////////////////////////////////////////////

class WebPart
{
public:

	WebPart (const char *name, const char *content);
	WebPart (const char *name, const char *filename, FILE *fp);
	WebPart (const char *name, const char *(*func)(WebRequest *));
	~WebPart ();

	static const char *get (const char *name, WebRequest *req);

private:

	const char *name;
	char *content;
	const char *(*callback_func)(WebRequest *);

	const char *read_file (WebRequest *);

	char *filename;
	struct timespec last_modified;

	static WebPart *all_webparts;

	WebPart *succ;

};

////////////////////////////////////////////////////////////////////////////////

class WebRequest
{
public:

	WebRequest (WebSocket *respond_to, char *request);
	~WebRequest ();

	void process (void);

	static void register_page (const char *url, bool (*callback_func)(WebRequest *));
	static void register_part (const char *name, const char *content);
	static void register_part (const char *name, const char *(*callback_func)(WebRequest *));

	void set_response_code (int);
	void add_header (const char *buffer);
	void add_response (const char *buffer, int len);
	void add_template_response (const char *buffer, int len);
	void add_response_part (const char *name, const char *content);
	void end_response (void);

private:

	void request_to_headers (void);
	void add_response_code (void);

	WebSocket *socket;

	char *request;

	int response_code;
	char *headers;
	int headers_len;
	char *response;
	int response_len;

	bool keep_alive;

	char *method;
	char *path;
	char *protocol;

	int number_of_arguments;
	char *argument_key[maximum_number_of_request_arguments];
	char *argument_value[maximum_number_of_request_arguments];

	int number_of_headers;
	char *header_key[maximum_number_of_request_headers];
	char *header_value[maximum_number_of_request_headers];

	int number_of_parts;
	char *part_key[maximum_number_of_request_parts];
	char *part_value[maximum_number_of_request_parts];

};

////////////////////////////////////////////////////////////////////////////////

class ListenSocket : public Socket
{
public:

	ListenSocket (int port);
	virtual ~ListenSocket ();

	bool is_readable (void);
	bool is_writable (void);

	virtual void on_readable (void);
	virtual void on_writable (void) {};

	void set_callback (void (*func)(int, unsigned long, unsigned int));

	virtual char *get_name (void);

	static int get_read_pipefd (void);
	static int get_write_pipefd (void);

private:

	static int pipefd[2];

	void (*callback_func)(int, unsigned long, unsigned int);

};

////////////////////////////////////////////////////////////////////////////////

#endif
