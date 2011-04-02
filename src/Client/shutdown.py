## @file shutdown.py
#  @brief This script is executed when a client is shut down to unregister it from the server.
#
#  @author Simon Barth <Simon.Barth@gmx.de>

import socket
import xbmcgui

## The IP-Address of the Server
server_ip="192.168.1.111"
## A socket on the server which is pingable
server_address=(server_ip,445)
## The socket on the server for the xbox_management-protocoll
server_xbox_mngmnt=(server_ip,35002)
## IP-Address of the client
xbox_ip=xbmc.getIPAddress()
## socket on the client for first ping test
xbox_address=(xbox_ip,33334)
## socket on the client to check if the server is up after WOL
xbox_mngmnt_address=(xbox_ip, 33303)
## the message to send to the server
message="shutdown"

## pings the server to test if it is running
# @return true if the server is running, false if it is powered-off
def pingServer():
	
	# ping the server first
	ping=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
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

## asks the user if the server should be shut down
# 
def askShutdown():

	dialog2=xbmcgui.Dialog()
	ret = dialog2.yesno("Ausschalten", "Vom Server abmelden ?")
	if ret == True:
		try:
			connection=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			connection.bind(xbox_mngmnt_address)
			connection.connect(server_xbox_mngmnt)
			if sendShutdown(connection) == True :
				if recieveAnswer(connection) == True :
					connection.close()
					del connection

		except:
			dialog3=xbmcgui.Dialog()
			dialog3.ok('Shutdown.py', 'askShutdown()','Couldn\'t connect to Server')
			del dialog3
			connection.close()
			del connection

## sends a shutdown message to connection
# @param connection socket connected to the management server
#
def sendShutdown(connection):
	try:
		connection.send(message)
		return True
	except :
		dialog4=xbmcgui.Dialog()
		dialog4.ok('Shutdown.py', 'sendShutdown()','Couldn\'t send message')
		del dialog4
		return False

## recieves messages from the server
# @param connection socket connected to the management server
# 
def recieveAnswer(connection):
	try:
		answer = connection.recv(4)
		if answer == "off\0" :
			# server faehrt sich runter
			dialog = xbmcgui.Dialog()
			dialog.ok('Shutdown.py', "Server wird abgeschalten")
			return True
		elif answer == "on\0\0" :
			# server bleibt an
			dialog = xbmcgui.Dialog()
			dialog.ok('Shutdown.py', "Server faehrt sich nicht herunter:", "Es sind noch weitere clients angemeldet")
			return True
		else:
			dialog = xbmcgui.Dialog()
			dialog.ok('Shutdown.py', "Unbekannte Nachricht vom Server geschickt.", answer)

	except:
		dialog = xbmcgui.Dialog()
		dialog.ok('Shutdown.py', 'recieveAnswer','Couldn\'t recieve message')
		del dialog
		return False

##
#
def connectionFailure(error):
	dialog=xbmcgui.Dialog()
	dialog.ok('Startup.py', error)
	del dialog

# shut xbox down
ret = pingServer()
if ret == True:
	askShutdown()
	#print "server an"
else:
	#print "server aus"
	dialog=xbmcgui.Dialog()
	dialog.ok("Shutdown.py", "Server ist bereits aus.")

xbmc.executebuiltin('XBMC.ShutDown()')
