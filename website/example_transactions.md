% Example Transactions

	Client> 01 03 0c 00 
		<Reset_Command>
		
	Server> 04 0e 04 01 03 0c 00 
		<Command Complete Event (Reset, status=00)>
		
		
	Client> 01 09 10 00 
		<Read_BD_ADDR_Command>
		
	Server> 04 0e 0a 01 09 10 00 c0 ef 01 00 00 7f 
		<Command Complete Event (Read_BD_ADDR, status=00, bd_addr=7f000001efc0)>
	
	
	Client> 01 03 10 00 
		<Read_Local_Supported_Features_Command>
		
	Server> 04 0e 0c 01 03 10 00 00 00 00 40 00 00 00 80 
		<Command Complete Event (Read_Local_Supported_Features, status=00, features=8000000040000000)>
