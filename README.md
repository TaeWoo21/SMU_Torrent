# SMUTorrent 

usage

SMU client
compile: gcc -o smuTorrent.c

SMU server
before you start build this code, you need mysql client 

1.you can install mysql by this code

	apt-get install mysql-client
	
2.modify server code 
	change your mysql user id
	change your mysql user's passwd

	#define DB_USER "root"//mysql user_id
	#define DB_PASS "***********"//user passwd

3.build code 

	you can build server this code
		
		gcc -o smuTorrentServer smuTorrentServer.c -lmysqlclient -lpthread
	
	you can build client this code

		gcc -o smuTorrent smuTorrent.c
	
	
