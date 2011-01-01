NEEDED:
	- Wake on LAN 
	- Ports 445, 35002 open


The src folder contains all the needed files.
It is ordered to the client part and the server part.
The client folder contains the startup and the shutdown scripts for xbmc
The server folder contains the server programm and an init-script.

The archive contains a Makefile to build the server on the targeted machine.

To do so:
-extract the archive to a folder of your choice on your server
-type 'make all'
-copy the executable 'xbox_management_server' to a location of your choice
-modify the SERVER_PATH variable in the init-script to use the right path for your server
