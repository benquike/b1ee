% Example Client

# Using Python

	# This just connects to b1ee.com on port 0xb1ee and then sends a Read_BD_ADDR command
	# and will hopefully receive an event response with the BD_ADDR assigned to this client
	
	import socket
	
	HOST = 'b1ee.com'    # The remote host
	PORT = 0xb1ee        # The same port as used by the server
	
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((HOST, PORT))
	s.sendall(b'\x01\x09\x10\x00')
	data = s.recv(1024)
	s.close()
	
	print ('Received', data)