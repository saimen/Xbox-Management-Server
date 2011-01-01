import time, socket, xbmcgui

server_ip="192.168.1.111"
server_address=(server_ip,445)
server_xbox_mngmnt=(server_ip,35002)
xbox_ip=xbmc.getIPAddress()
xbox_address=(xbox_ip,33333)
xbox_address2=(xbox_ip,33301)
xbox_mngmnt_address=(xbox_ip, 33302)
# ping the server first

ping=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ping.settimeout(5.0)
ping.bind(xbox_address)
try:
	ping.connect(server_address)
	dialog7=xbmcgui.Dialog()
	dialog7.ok("Startup.py", "Server läuft bereits")
	ping.close()
	del ping
	del dialog7
except: 
	
	#close connection
	ping.close()
	del ping

	## send wake up packet to server
	wakeup=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	wakeup.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	wakeup.sendto('\xff'*6+'\x00\x0b\x6a\x60\x79\x6a'*16, ("255.255.255.255",80))

	xbmc.executebuiltin('XBMC.WakeOnLan(00:0B:6A:60:79:6A)')

	# 00:0B:6A:60:79:6A = Server MAC

	#close and delete wakeup
	wakeup.close()
	del wakeup

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


'''	

	# ping server
	try:
		ping2=socket.socket(socket.AF_INET, sock.SOCK_STREAM)
		ping2.bind(xbox_address2)
		ping2.connect(server_address)
		dialog6=xbmcgui.Dialog()
		dialog6.ok('Startup.py', 'Server wurde erfolgreich gestartet')
		del dialog6
		ping2.close()
		del ping2
	except: 
		# couldn't start server
		dialog5=xbmcgui.Dialog()
		dialog5.ok('Startup.py', 'Server konnte nicht gestartet werden...')
		del dialog5
		try:
			ping2.close()
		except:
			del ping2
		del ping2
		'''

# create file on server
# seems to need server app 

connection=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
connection.bind(xbox_mngmnt_address)
try:
	connection.connect(server_xbox_mngmnt)
except:
	dialog3=xbmcgui.Dialog()
	dialog3.ok('Startup.py', 'Couldn\'t connect to Server, trying to wait and connect again...')
	del dialog3
	connection.close()
	del connection
	try:
		time.sleep(5)
		connection2.connect(server_xbox_mngmnt)
	except:
		dialog3=xbmcgui.Dialog()
		dialog3.ok('Startup.py', 'Couldn\'t connect to Server')
		del dialog3
		connection2.close()
		del connection2





message="startup"
try:
	connection.send(message)
	connection.close()
	del connection
except:
	dialog4=xbmcgui.Dialog()
	dialog4.ok('Startup.py', 'Couldn\'t send message')
	del dialog4
# could give problems, test number of bytes sent if problems occur




