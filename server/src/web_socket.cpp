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
#include <sys/stat.h>
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

void WebSocket::write_data (const char *buffer, int len)
{
//	log_start (LOG_WEBSOCKET, "WebSocket::write_data (%d : %p) ", len, buffer);
//	for (int index = 0; index < len; index ++)
//	{
//		log_continuation ("%02X", buffer[index]);
//	}
//	log_end ();

	if (write_buffer_size - write_buffer_len < len)
	{
		write_buffer_size = write_buffer_len + len;
		write_buffer = (char *) realloc (write_buffer, write_buffer_size);
	}

	memcpy (&write_buffer[write_buffer_len], buffer, len);
	write_buffer_len += len;

	write (ListenSocket::get_write_pipefd (), " ", 1);
}

void WebSocket::write_string (const char *buffer)
{
	write_data (buffer, strlen (buffer));
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

WebPage *WebPage::all_webpages = 0;

////////////////////////////////////////////////////////////////////////////////

WebPage::WebPage (const char *page_path, bool (*page_callback_func)(WebRequest *))
{
	path = page_path;
	callback_func = page_callback_func;

	html_len = 0;
	html_file = 0;
	filename = 0;

	succ = all_webpages;
	all_webpages = this;
}

////////////////////////////////////////////////////////////////////////////////

WebPage::WebPage (const char *page_path, char *page_filename, FILE *fp)
{
	off_t len;
	int pathlen;
	char *contents;
	WebPage *file;
	struct stat file_stat;


	path = strdup (page_path);
	callback_func = 0;

	filename = strdup (page_filename);

	stat (filename, &file_stat);

	last_modified = file_stat.st_mtimespec;

	fseeko (fp, 0, SEEK_END);
	html_len = ftello (fp);
	fseeko (fp, 0, SEEK_SET);
	html_file = (char *) malloc (html_len + 1);
	fread (html_file, html_len, 1, fp);
	html_file[html_len] = 0;

	succ = all_webpages;
	all_webpages = this;
}

////////////////////////////////////////////////////////////////////////////////

WebPage *WebPage::find (const char *path)
{
	WebPage *page;


	page = all_webpages;
	while (page)
	{
		if (strcasecmp (path, page->path) == 0)
		{
			return page;
		}
		page = page->succ;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

WebPage *WebPage::open (const char *path)
{
	off_t len;
	int pathlen;
	char pathname[1024];
	char *contents;
	FILE *fp;
	WebPage *file;


	getcwd (pathname, 1024);
	pathlen = strlen (pathname);
	if (strlen (path) + 5 + 10 + pathlen < 1023)
	{
		if (path[strlen(path)-1] == '/')
		{
			sprintf (&pathname[pathlen], "/html%sindex.html", path);
		}
		else
		{
			sprintf (&pathname[pathlen], "/html%s", path);
		}

		fp = fopen (pathname, "r");

		if (fp)
		{
			file = new WebPage (path, pathname, fp);
			fclose (fp);
			return file;
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool WebPage::process (WebRequest *req)
{
	int err;
	FILE *fp;
	struct stat file_stat;


	if (callback_func)
	{
		return callback_func (req);
	}
	else
	{
		err = stat (filename, &file_stat);

		if (err < 0)
		{
			log (LOG_ERROR, "stat %s (%d : %s)", filename, errno, strerror (errno));
		}

		if ((file_stat.st_mtimespec.tv_sec != last_modified.tv_sec) || (file_stat.st_mtimespec.tv_nsec != last_modified.tv_nsec))
		{
			last_modified = file_stat.st_mtimespec;

			free (html_file);
			html_file = 0;

			fp = fopen (filename, "r");

			if (fp)
			{
				fseeko (fp, 0, SEEK_END);
				html_len = ftello (fp);
				fseeko (fp, 0, SEEK_SET);
				html_file = (char *) malloc (html_len + 1);
				fread (html_file, html_len, 1, fp);
				html_file[html_len] = 0;

				fclose (fp);
				
				req->set_response_code (200);
				req->add_template_response (html_file, html_len);

				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			req->set_response_code (200);
			req->add_template_response (html_file, html_len);

			return true;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

WebPart *WebPart::all_webparts = 0;

////////////////////////////////////////////////////////////////////////////////

WebPart::WebPart (const char *part_name, const char *part_content)
{
	name = strdup (part_name);
	content = strdup (part_content);
	callback_func = 0;
	filename = 0;

	succ = all_webparts;
	all_webparts = this;
}

////////////////////////////////////////////////////////////////////////////////

WebPart::WebPart (const char *part_name, const char *(*func)(WebRequest *))
{
	name = strdup (part_name);
	content = 0;
	callback_func = func;
	filename = 0;

	succ = all_webparts;
	all_webparts = this;
}

////////////////////////////////////////////////////////////////////////////////

WebPart::WebPart (const char *part_name, const char *part_filename, FILE *fp)
{
	off_t part_len;
	struct stat file_stat;


	name = strdup (part_name);
	content = 0;
	callback_func = 0;
	filename = strdup (part_filename);

	stat (filename, &file_stat);
	last_modified = file_stat.st_mtimespec;

	fseeko (fp, 0, SEEK_END);
	part_len = ftello (fp);
	fseeko (fp, 0, SEEK_SET);
	content = (char *) malloc (part_len + 1);
	fread ((void *) content, part_len, 1, fp);
	content[part_len] = 0;

	succ = all_webparts;
	all_webparts = this;
}

////////////////////////////////////////////////////////////////////////////////

const char *WebPart::read_file (WebRequest *req)
{
	int err;
	off_t part_len;
	struct stat file_stat;
	FILE *fp;


	err = stat (filename, &file_stat);

	if (err < 0)
	{
		log (LOG_ERROR, "stat %s (%d : %s)", filename, errno, strerror (errno));
	}

	if ((file_stat.st_mtimespec.tv_sec != last_modified.tv_sec) || (file_stat.st_mtimespec.tv_nsec != last_modified.tv_nsec))
	{
		last_modified = file_stat.st_mtimespec;

		free (content);
		content = 0;

		fp = fopen (filename, "r");

		if (fp)
		{
			fseeko (fp, 0, SEEK_END);
			part_len = ftello (fp);
			fseeko (fp, 0, SEEK_SET);
			content = (char *) malloc (part_len + 1);
			fread ((void *) content, part_len, 1, fp);
			content[part_len] = 0;

			fclose (fp);

			return content;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return content;
	}
}

////////////////////////////////////////////////////////////////////////////////

const char *WebPart::get (const char *part_name, WebRequest *req)
{
	off_t len;
	int pathlen;
	char pathname[1024];
	FILE *fp;
	WebPart *part;
	struct stat file_stat;


	part = all_webparts;
	while (part)
	{
		if (strcasecmp (part->name, part_name) == 0)
		{
			if (part->callback_func)
			{
				return part->callback_func (req);
			}
			else if (part->filename)
			{
				return part->read_file (req);
			}
			else
			{
				return part->content;
			}
		}

		part = part->succ;
	}

	getcwd (pathname, 1024);
	pathlen = strlen (pathname);

	if (strlen (part_name) + 6 + 10 + pathlen < 1023)
	{
		sprintf (&pathname[pathlen], "/parts/%s", part_name);

		fp = fopen (pathname, "r");

		if (fp)
		{
			part = new WebPart (part_name, pathname, fp);

			fclose (fp);

			return part->content;
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

WebRequest::WebRequest (WebSocket *respond_to, char *request_string)
{
	int index;


	socket = respond_to;
	request = request_string;

	headers = 0;
	headers_len = 0;
	response = 0;
	response_len = 0;

	keep_alive = false;

	method = 0;
	path = 0;
	protocol = 0;

	number_of_arguments = 0;

	for (index = 0; index < maximum_number_of_request_arguments; index ++)
	{
		argument_key[index] = 0;
		argument_value[index] = 0;
	}

	number_of_headers = 0;

	for (index = 0; index < maximum_number_of_request_headers; index ++)
	{
		header_key[index] = 0;
		header_value[index] = 0;
	}

	for (index = 0; index < maximum_number_of_request_parts; index ++)
	{
		part_key[index] = 0;
		part_value[index] = 0;
	}

	request_to_headers ();
}

////////////////////////////////////////////////////////////////////////////////

WebRequest::~WebRequest ()
{
	int index;


	for (index = 0; index < maximum_number_of_request_parts; index ++)
	{
		if (part_key[index])
		{
			free (part_key[index]);
		}

		if (part_value[index])
		{
			free (part_value[index]);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::request_to_headers (void)
{
	int index;
	char *ptr;


	ptr = request;

	method = ptr;

	// process the method text e.g. GET

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
			
			path = ptr;
			break;
		}
		else
		{
			ptr ++;
		}
	}

	// process the url e.g. /path?arg=4&other=5

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
		else if (*ptr == '?')
		{
			*ptr = 0;
			ptr ++;

			index = 0;
			argument_key[index] = ptr;

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
				else if (*ptr == '=')
				{
					*ptr = 0;
					ptr ++;

					if (index < maximum_number_of_request_arguments)
					{
						argument_value[index] = ptr;
					}
					index ++;
				}
				else if (*ptr == '&')
				{
					*ptr = 0;
					ptr ++;

					if (index < maximum_number_of_request_arguments)
					{
						argument_key[index] = ptr;
					}
				}
				else
				{
					ptr ++;
				}
			}

//			query = ptr;
		}
		else
		{
			ptr ++;
		}
	}

	number_of_arguments = index;

	// process the protocol e.g. HTTP/1.1

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

	index = 0;
	header_key[index] = ptr;

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
				header_value[index] = ptr;
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
				header_key[index] = ptr;
			}
		}
	}

	number_of_headers = index;
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::register_page (const char *path, bool (*callback_func)(WebRequest *))
{
	new WebPage (path, callback_func);
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::register_part (const char *name, const char *content)
{
	new WebPart (name, content);
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::register_part (const char *name, const char *(*callback_func)(WebRequest *))
{
	new WebPart (name, callback_func);
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::add_header (const char *buffer)
{
	int len;


	len = strlen (buffer);
	headers = (char *) realloc (headers, headers_len + len + 2);
	memcpy (&headers[headers_len], buffer, len);
	headers_len += len;
	headers[headers_len] = 0x0D;
	headers_len ++;
	headers[headers_len] = 0x0A;
	headers_len ++;
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::set_response_code (int code)
{
	response_code = code;
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::add_response_code (void)
{
	char buffer[100];
	const char *code_string = 0;


	switch (response_code)
	{
		case 100: code_string = "100 Continue"; break;
		case 101: code_string = "101 Switching Protocols"; break;
		case 102: code_string = "102 Processing"; break;
		case 200: code_string = "200 OK"; break;
		case 201: code_string = "201 Created"; break;
		case 202: code_string = "202 Accepted"; break;
		case 203: code_string = "203 Non-Authoritative Information"; break;
		case 204: code_string = "204 No Content"; break;
		case 205: code_string = "205 Reset Content"; break;
		case 206: code_string = "206 Partial Content"; break;
		case 207: code_string = "207 Multi-Status"; break;
		case 208: code_string = "208 Already Reported"; break;
		case 226: code_string = "226 IM Used"; break;
		case 300: code_string = "300 Multiple Choices"; break;
		case 301: code_string = "301 Moved Permanently"; break;
		case 302: code_string = "302 Found"; break;
		case 303: code_string = "303 See Other"; break;
		case 304: code_string = "304 Not Modified"; break;
		case 305: code_string = "305 Use Proxy"; break;
		case 306: code_string = "306 Switch Proxy"; break;
		case 307: code_string = "307 Temporary Redirect"; break;
		case 308: code_string = "308 Permanent Redirect"; break;
		case 400: code_string = "400 Bad Request"; break;
		case 401: code_string = "401 Unauthorized"; break;
		case 402: code_string = "402 Payment Required"; break;
		case 403: code_string = "403 Forbidden"; break;
		case 404: code_string = "404 Not Found"; break;
		case 405: code_string = "405 Method Not Allowed"; break;
		case 406: code_string = "406 Not Acceptable"; break;
		case 407: code_string = "407 Proxy Authentication Required"; break;
		case 408: code_string = "408 Request Timeout"; break;
		case 409: code_string = "409 Conflict"; break;
		case 410: code_string = "410 Gone"; break;
		case 411: code_string = "411 Length Required"; break;
		case 412: code_string = "412 Precondition Failed"; break;
		case 413: code_string = "413 Requested Entity Too Large"; break;
		case 414: code_string = "414 Request-URI Too Long"; break;
		case 415: code_string = "415 Unsupported Media Type"; break;
		case 416: code_string = "416 Requested Range Not Satisfiable"; break;
		case 417: code_string = "417 Expectation Failed"; break;
		case 418: code_string = "418 I'm a teapot"; break;
		case 420: code_string = "420 Enhance Your Calm"; break;
		case 422: code_string = "422 Unprocessable Entity"; break;
		case 423: code_string = "423 Locked"; break;
		case 424: code_string = "424 Failed Dependency"; break;
		case 425: code_string = "425 Unordered Collection"; break;
		case 426: code_string = "426 Upgrade Required"; break;
		case 428: code_string = "428 Precondition Required"; break;
		case 429: code_string = "429 Too Many Requests"; break;
		case 431: code_string = "431 Request Header Fields Too Large"; break;
		case 444: code_string = "444 No Response"; break;
		case 449: code_string = "449 Retry With"; break;
		case 450: code_string = "450 Blocked By Windows Parental Controls"; break;
		case 451: code_string = "451 Unavailable For Legal Reasons"; break;
		case 494: code_string = "494 Request Header Too Large"; break;
		case 495: code_string = "495 Cert Error"; break;
		case 496: code_string = "496 No Cert"; break;
		case 497: code_string = "497 HTTP to HTTPS"; break;
		case 499: code_string = "499 Client Closed Request"; break;
		case 500: code_string = "500 Internal Server Error"; break;
		case 501: code_string = "501 Not Implemented"; break;
		case 502: code_string = "502 Bad Gateway"; break;
		case 503: code_string = "503 Service Unavailable"; break;
		case 504: code_string = "504 Gateway Timeout"; break;
		case 505: code_string = "505 HTTP Version Not Supported"; break;
		case 506: code_string = "506 Variant Also Negotiates"; break;
		case 507: code_string = "507 Insufficient Storage"; break;
		case 508: code_string = "508 Loop Detected"; break;
		case 509: code_string = "509 Bandwidth Limit Exceeded"; break;
		case 510: code_string = "510 Not Extended"; break;
		case 511: code_string = "511 Network Authentication Required"; break;
		case 598: code_string = "598 Network Read Timeout Error"; break;
		case 599: code_string = "599 Network Connect Timeout Error"; break;

		default: code_string = "500 Internal Server Error"; break;
	}

	sprintf (buffer, "HTTP/1.1 %s\r\n", code_string);
	socket->write_data (buffer, strlen (buffer));
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::add_response (const char *buffer, int len)
{
	response = (char *) realloc (response, response_len + len);
	memcpy (&response[response_len], buffer, len);
	response_len += len;
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::add_template_response (const char *buffer, int len)
{
	bool part_found;
	int index;
	int part_num;
	int count;
	char last_ch;
	char part_name[128];
	const char *part;
	const char *ptr;


	ptr = buffer;
	count = 1;
	for (index = 1; index < len; index ++)
	{
		if ((((index > 1) && (buffer[index-2] != '\\')) || (index < 2)) && (buffer[index-1] == '$') && (buffer[index] == '{'))
		{
			add_response (ptr, &buffer[index] - ptr - 1);

			index ++;
			ptr = &buffer[index];
			count = 0;

			while ((index < len) && (buffer[index] != '}'))
			{
				index += 1;
				count += 1;
			}

			if ((index < len) && (buffer[index] == '}'))
			{
				index += 1;
			}

			memcpy (part_name, ptr, count);
			
			part_name[count] = 0;
			
			part = WebPart::get (part_name, this);

			if (part)
			{
				add_template_response (part, strlen (part));
			}
			else
			{
				part_found = false;
				for (part_num = 0; part_num < maximum_number_of_request_parts; part_num ++)
				{
					if ((part_key[part_num]) && (strcasecmp (part_key[part_num], part_name) == 0))
					{
						add_template_response (part_value[part_num], strlen (part_value[part_num]));
						part_found = true;
						break;
					}
				}

				if (!part_found)
				{
					add_response ("${", 2);
					add_response (part_name, strlen (part_name));
					add_response ("}", 1);
				}
			}

			ptr = &buffer[index];
			count = 0;
		}
		else
		{
			count ++;
		}
	}

	add_response (ptr, count);
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::add_response_part (const char *part, const char *content)
{
	int index;


	for (index = 0; index < maximum_number_of_request_parts; index ++)
	{
		if (part_key[index] == 0)
		{
			part_key[index] = strdup (part);
			part_value[index] = strdup (content);
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::end_response (void)
{
	char buffer[100];


	add_response_code ();
	socket->write_data (headers, headers_len);

	if (keep_alive)
	{
		socket->write_string ("Connection: keep-alive\r\n");
	}
	else
	{
		socket->write_string ("Connection: close\r\n");
	}

	sprintf (buffer, "Content-Length: %d\r\n\r\n", response_len);
	socket->write_data (buffer, strlen (buffer));
	socket->write_data (response, response_len);
}

////////////////////////////////////////////////////////////////////////////////

void WebRequest::process (void)
{
	int index;
	static int count = 0;
	char buffer[100];
	WebPage *page;


	log (LOG_WEBSOCKET, "Process: %s, %s, %s", method, path, protocol);

	for (index = 0; index < number_of_arguments; index ++)
	{
		log (LOG_WEBSOCKET, "   Arg : %-15s = %s", argument_key[index], argument_value[index]);
	}

	for (index = 0; index < number_of_headers; index ++)
	{
		log (LOG_WEBSOCKET, "   Head: %-15s = %s", header_key[index], header_value[index]);
	}

	page = WebPage::find (path);

	if (page)
	{
		log (LOG_WEBSOCKET, "    Page = %p", page);

		if (page->process (this))
		{
			end_response ();
		}
	}
	else
	{
		page = WebPage::open (path);

		if ((page) && (page->process (this)))
		{
			end_response ();
		}
		else
		{
			add_response_part ("request_path", path);
			add_response_part ("page_left", "");
			add_response_part ("page_right", "Page Not Found : ${request_path}");
			add_template_response ("${page_layout}", strlen ("${page_layout}"));
			set_response_code (404);
			end_response ();
		}
	}

	socket->close_when_written ();
}

////////////////////////////////////////////////////////////////////////////////
