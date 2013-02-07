"""
Copyright (c) 2012, Robin Heydon
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import socket
import select


class AsynchronousSockets:
	all_sockets = {}

	def select (self):
		readable_sockets = []
		writable_sockets = []
		error_sockets = []

		for fileno, object in AsynchronousSockets.all_sockets.items ():
			is_readable = object.readable ()
			is_writable = object.writable ()

			if is_readable:
				readable_sockets.append (fileno)

			if is_writable:
				writable_sockets.append (fileno)

			if is_readable or is_writable:
				error_sockets.append (fileno)

		if [] == readable_sockets == writable_sockets == error_sockets:
			return None

		readable_sockets, writable_sockets, error_sockets = select.select (readable_sockets, writable_sockets, error_sockets, 0.1)

		if [] == readable_sockets == writable_sockets == error_sockets:
			return None

		for fileno in readable_sockets:
			object = AsynchronousSockets.all_sockets[fileno]
			object.handle_read ()

		for fileno in writable_sockets:
			object = AsynchronousSockets.all_sockets[fileno]
			object.handle_write ()

		for fileno in error_sockets:
			object = AsynchronousSockets.all_sockets[fileno]
			object.handle_error ()
	
	@staticmethod
	def add_asynchronous_socket (socket):
		AsynchronousSockets.all_sockets[socket._fileno] = socket

	@staticmethod
	def del_asynchronous_socket (socket):
		del (AsynchronousSockets.all_sockets[socket._fileno])


class AsyncSocket:
	def __init__ (self, socket):
		self.connected = True
		self.socket = socket
		self.read_buffer = b''
		self.write_buffer = b''
		self.when_done = False
		self.terminator = None
		self._fileno = self.fileno ()
		AsynchronousSockets.add_asynchronous_socket (self)	
	
	def set_terminator (self, terminator):
		self.terminator = terminator
		if isinstance (terminator, int):
			self.numeric_terminator = True
		else:
			self.numeric_terminator = False
	
	def fileno (self):
		return self.socket.fileno ()

	def readable (self):
		return self.connected;

	def writable (self):
		return len (self.write_buffer)

	def handle_read (self):
		data = self.socket.recv (select.PIPE_BUF)
		if len (data) == 0:
			self.close ()
		else:
			self.read_buffer = self.read_buffer + data

			if self.terminator:
				if self.numeric_terminator:
					index = self.terminator
					if len (self.read_buffer) >= index:
						input = self.read_buffer[:index]
						self.read_buffer = self.read_buffer[index:]
						self.handle_input (input)
				else:
					index = self.read_buffer.find (self.terminator)
					terminator_len = len (self.terminator)
					if index != -1:
						input = self.read_buffer[:index]
						self.read_buffer = self.read_buffer[index + terminator_len:]
						if index > 0:
							self.handle_input (input)
			else:
				input = self.read_buffer
				self.read_buffer = b''
				self.handle_input (input)
	
	def write (self, data):
		self.write_buffer = self.write_buffer + data
	
	def close_when_done (self):
		self.when_done = True
	
	def close (self):
		self.connected = False
		self.socket.close ()
		self.handle_close ()
		AsynchronousSockets.del_asynchronous_socket (self)	

	def handle_write (self):
		sent = self.socket.send (self.write_buffer[:select.PIPE_BUF])
		self.write_buffer = self.write_buffer[sent:]

		if self.when_done and len (self.write_buffer) == 0:
			self.close ()

	def handle_error (self):
		self.close ()
	
	def handle_close (self):
		pass


class AsyncListenSocket (AsyncSocket):
	def __init__ (self, host_port):
		s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
		s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		s.bind (host_port)
		s.listen (128)
		AsyncSocket.__init__ (self, s)

	def __repr__ (self):
		return 'AsyncListenSocket(%d)' % self._fileno

