#!/bin/bash
# set password to "changeme"
PASSWD="/home/user/.vnc/passwd"
echo "MPTcXfgXGiY=" | base64 -d > $PASSWD
chown user:user $PASSWD
chmod 600 $PASSWD
# start VNC server without authentication
su - user -c "vncserver -SecurityTypes None"
echo "*WARNING* VNC Server launched without any authentication mechanism. Please consider using SSH tunneling instead."
echo "You can now connect to localhost:5901 using a VNC client."
echo "*NOTE* Terminating this shell will also kill your desktop."
sleep infinity
