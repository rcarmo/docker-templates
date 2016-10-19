#!/bin/bash
# set password to "changeme"
PASSWD="/home/user/.vnc/passwd"
echo "MPTcXfgXGiY=" | base64 -d > $PASSWD
chown user:user $PASSWD
chmod 600 $PASSWD
# start VNC server without authentication
su - user -c "vncserver -SecurityTypes None -depth 16 -geometry 1024x768 -geometry 1280x800 -geometry 1366x768 -geometry 1600x1200 -geometry 1280x720 -geometry 1920x1080 -geometry 2048x1536"
echo "*WARNING* VNC Server launched without any authentication mechanism. Please consider using SSH tunneling instead."
echo "You can now connect to localhost:5901 using a VNC client."
echo "*NOTE* Terminating this shell will also kill your desktop."
sleep infinity
