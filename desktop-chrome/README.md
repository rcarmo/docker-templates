# desktop-chrome

    docker run -p 2211:22 rcarmo/desktop-chrome

A minimalist desktop environment containing:

* An Ubuntu 16.04 (Xenial) userland
* A simple Openbox/(`fbpanel`) desktop you can access over VNC
* The Infinality font rendering engine
* Google Chrome
* Optionally, with the (`:tiger` tag): TigerVNC Server, for a dynamically resizable desktop

## Logging In

Login via SSH to port 2211 as `user`, password `changeme`. Once inside, `remote.sh` will launch the VNC server with a set of predefined resolutions (use `xrandr -s <number>` to resize the desktop - some VNC clients may need to reconnect, but TigerVNC seems to work fine in Windows, and so does the macOS VNC client)

## Quickstart (NO AUTHENTICATION!)

    docker run -d -p 5901:5901 rcarmo/desktop-chrome /quickstart.sh noauth

No need to use SSH, no VNC authentication. Great for trying it out -- but a bad idea if you expose Docker ports outside your machine.

If you're using a Mac, then don't use `noauth` and type `changeme` as a VNC password (the built-in VNC client rightfully dislikes null authentication options).

## Typical Session (via SSH)

    > docker run -d -p 2211:22 rcarmo/desktop-chrome
    93b7f70b971468095ba737b1d942131362c9bd7281905f5e8924fd8ddf36300d 
    > ssh -p 2211 -L 5901:localhost:5901 user@localhost
    The authenticity of host '[localhost]:2211 ([127.0.0.1]:2211)' can't be established.
    ECDSA key fingerprint is SHA256:Kizn3NWB7LNUjG6BejExMA5f9ZC+Ch4BkooADRgXKZ8.
    Are you sure you want to continue connecting (yes/no)? yes
    Warning: Permanently added '[localhost]:2211' (ECDSA) to the list of known hosts.
    user@localhost's password:
    $ sh renote.sh
    You will require a password to access your desktops.
    
    Password:
    Verify:
    xauth:  file /home/user/.Xauthority does not exist
    
    New '93b7f70b9714:1 (user)' desktop is 93b7f70b9714:1
    
    Starting applications specified in /home/user/.vnc/xstartup
    Log file is /home/user/.vnc/93b7f70b9714:1.log
    $
    # you can now VNC to localhost:5901

## Preserving Your Work

Just mount a data volume and work inside it. For instance, on your Mac:

    docker run -d -p 5901:5901 -v /Users/Johnny\ Appleseed/Development:/home/user/Development rcarmo/desktop-chrome /quickstart.sh

Preserving browser and editor settings is also doable. Just mount another volume as `/home/user/.config` and you should be set (it's probably best to copy the existing settings across first).

(Obviously, you can use everything inside the container without SSH, but I find it useful to deploy this as a temporary jumpbox occasionally.)

## Security Considerations

I ordinarily use VNC exclusively over SSH, so that's the only port I decided to `EXPOSE`.

As such, use of the `quickstart.sh` script should not be considered a best practice -- if you're going to leave this running someplace, best do it properly.

If you don't pass `noauth` to `quickstart.sh` the VNC server will prompt for a password (which is `changeme` too), but that's hardly secure in practice.

## Derived Works

My [Azure Toolbox](https://github.com/rcarmo/azure-toolbox) is one of the public containers that uses this.
