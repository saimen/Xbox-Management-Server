# Put this file into Q:\scripts folder to be executed when XBMC starts.
import os
import xbmc

# Change this path according your XBMC setup (you may not need 'My Scripts')
script_path = xbmc.translatePath(os.path.join(os.getcwd(),'startup.py'))
xbmc.executescript(script_path)

