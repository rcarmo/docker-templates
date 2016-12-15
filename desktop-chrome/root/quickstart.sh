#!/bin/bash
PASSWD="/home/user/.vnc/passwd"
SETTINGS="-depth 16 -geometry 1024x768 -geometry 1280x800 -geometry 1366x768 -geometry 1600x1200 -geometry 1280x720 -geometry 1920x1080 -geometry 2048x1536"
AUTHMODE=" "
# cleanup /tmp
rm -rf /tmp/.X*
rm -rf /tmp/ssh-*
rm -f /home/user/.vnc/*.log
# set password to "changeme" if not set already
if [ ! -f $PASSWD ]; then
echo "MPTcXfgXGiY=" | base64 -d > $PASSWD
chown user:user $PASSWD
chmod 600 $PASSWD
fi
for i in "$@"
do
case $i in
  noauth)
  AUTHMODE="-SecurityTypes None"
  echo "*WARNING* VNC Server will be launched without any authentication mechanism. Please consider using SSH tunneling instead."
  ;;
esac
done
# start VNC server 
su - user -c "vncserver $AUTHMODE $SETTINGS"
echo "You can now connect to localhost:5901 using a VNC client."
echo "*NOTE* Terminating this shell will also kill your desktop."
sleep infinity
