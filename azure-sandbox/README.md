# azure-sandbox

    docker run -d -p 2211:22 -p 5901:5901 rcarmo/azure-sandbox

A standalone development environment to work on [Azure][a] solutions, containing:

* An Ubuntu 16.04 (Xenial) userland
* A simple X11 desktop you can access over VNC
* Google Chrome
* Java 8, NodeJS and Python runtimes
* The (old) [Azure Cross-Platform CLI][xcli] 
* The (new) [Azure CLI][az] 
* [Visual Studio Code][vc]

## Logging In

Login via SSH to port 2211 as `user`, password `changeme`. Once inside, `remote.sh` will launch the VNC server with a set of predefined resolutions (use `xrandr -s <number>` to resize the desktop - some VNC clients may need to reconnect, but TigerVNC seems to work fine in Windows)

## Quickstart (NO AUTHENTICATION!)

    docker run -d -p 5901:5901 rcarmo/azure-sandbox /root/quickstart.sh

No need to use SSH, no VNC authentication. Great for trying it out -- but a bad idea if you expose Docker ports outside your machine.

## Typical Session (via SSH)

    > docker run -d -p 2211:22 rcarmo/azure-sandbox
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

Just mount a data volume and work inside it. For instance, in Windows:

    docker run -d -p 5901:5901 -v c:/Users/billg/Development:/home/user/Development rcarmo/azure-sandbox /root/quickstart.sh

Preserving browser and editor settings is also doable. Just mount another volume as `/home/user/.config` and you should be set (it's probably best to copy the existing settings across first).

[Visual Studio Code][vc] stores its settings in `~/.vscode`, so you can also mount that onto a host path (mind you, sharing configuration files with your host is not necessarily a good idea).

## Security Considerations

Please note that *this container exposes port 5901*. I ordinarily use VNC exclusively over SSH, but Windows users may not have SSH installed or have trouble setting up the tunnel, so I decided to `EXPOSE` the port. Docker for Windows seems to only bind it locally, but your mileage may vary.

Also, the `quickstart.sh` script should not be considered as a best practice -- if you're going to leave this running someplace, best do it properly.

If you don't use `quickstart.sh` the VNC server will prompt for a password, but that's hardly secure in practice - so if you run this container on a machine exposed to the Internet, _don't_ publish port 5901.

[a]: http://azure.microsoft.com
[xcli]: https://github.com/azure/azure-xplat-cli
[az]: https://github.com/azure/azure-xplat-cli
[vc]: http://code.visualstudio.com

