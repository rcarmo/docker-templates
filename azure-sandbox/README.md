# azure-sandbox

    docker run -d -p 2211:22 rcarmo/azure-sandbox

A standalone development environment to work on [Azure][a] solutions, containing:

* An Ubuntu 16.04 (Xenial) userland
* A simple X11 desktop you can access over VNC
* Google Chrome
* Java 8, NodeJS and Python runtimes
* The (old) [Azure Cross-Platform CLI][xcli] 
* The (new) [Azure CLI][az] 
* [Visual Studio Code][vc]

Login via SSH to port 2211 as `user`, password `changeme`. Once inside, `remote.sh` will launch the VNC server with a set of predefined resolutions (use `xrandr -s <number>` to resize the desktop - some VNC clients may need to reconnect)

[a]: http://azure.microsoft.com
[xcli]: https://github.com/azure/azure-xplat-cli
[az]: https://github.com/azure/azure-xplat-cli
[vc]: http://code.visualstudio.com

