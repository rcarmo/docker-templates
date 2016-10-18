# desktop-chrome

    docker run -p 2211:22 rcarmo/desktop-chrome

A minimalist desktop environment containing:

* An Ubuntu 16.04 (Xenial) userland
* A simple Openbox/(`fbpanel`) desktop you can access over VNC
* Google Chrome

Login via SSH to port 2211 as `user`, password `changeme`. Once inside, `remote.sh` will launch the VNC server with a set of predefined resolutions (use `xrandr -s <number>` to resize the desktop - some VNC clients may need to reconnect).

(Obviously, you can use everything inside the container without SSH, but I find it useful to deploy this as a temporary jumpbox occasionally.)
