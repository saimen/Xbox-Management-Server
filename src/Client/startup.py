## @file startup.py
#  @brief This script is executed when a client is started, turns on the server if necessary, and registers it at the server 
#
#  @author Simon Barth <Simon.Barth@gmx.de>

import time, socket, xbmcgui

server_ip="192.168.1.111"
server_address=(server_ip,445)
server_xbox_mngmnt=(server_ip,35002)
xbox_ip=xbmc.getIPAddress()
xbox_address=(xbox_ip,33333)
xbox_address2=(xbox_ip,33301)
xbox_mngmnt_address=(xbox_ip, 33302)
message="startup"


## pings the server with IP Address server_ip on port 445(smb)
#
# return true if server was pingable false if server was not pingable
def pingServer():
	
	# ping the server first
	ping=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	ping.settimeout(5.0)
	ping.bind(xbox_address)
		
	try:
		ping.connect(server_address)
		ping.close()
		del ping
		return True
	#server laeuft
	except: 
	#server ist aus
		#close connection
		ping.close()
		del ping
		return False

## shows a dialog that informs the user that the server is allready running
#
def isStarted():
	dialog7=xbmcgui.Dialog()
	dialog7.ok("Startup.py", "Server laeuft bereits")
	del dialog7


## sends a magic packet to the server, the MAC-Address of the servers NIC is currently
# hardcoded inside of the function
#
def wakeUp():
	## send wake up packet to server
	wakeup=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	wakeup.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	wakeup.sendto('\xff'*6+'\x00\x0b\x6a\x60\x79\x6a'*16, ("255.255.255.255",80))
	#close and delete wakeup
	wakeup.close()
	del wakeup


	xbmc.executebuiltin('XBMC.WakeOnLan(00:0B:6A:60:79:6A)')

	# 00:0B:6A:60:79:6A = Server MAC

## shows a dialog that informs the user that the server is booting
# and displays how long it will take with a progress bar
#
def startingServer():
	# print waiting screen
	dialog = xbmcgui.DialogProgress()
	dialog.create("Server wird gestartet...")
	
	interval = 150.0 / 100
	for percent in range(100):
		time.sleep( interval )
		dialog.update(percent)
		if (dialog.iscanceled()):
	      	 	dialog.close()

	del dialog


## connects to the server on port 35002 and sends message (in this case startup)
#
def registerClient():
	# create file on server
	try:
		connection=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		connection.bind(xbox_mngmnt_address)
		connection.connect(server_xbox_mngmnt)
		connection.send(message)
		return True
	except:
		connection.close()
		connectionFailure('Couldn\'t connect to Server')
		del connection
		return False

## displays a dialog that informs the user about a connection failure
#
def connectionFailure(error):
	dialog=xbmcgui.Dialog()
	dialog.ok('Startup.py', error)
	del dialog

ret = pingServer()
if ret == True :
	registerClient()
	isStarted()
else:
	wakeUp()
	startingServer()
	registerClient()
