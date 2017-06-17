# docker-templates

A set of Docker templates (Dockerfiles and associated scripts). 

## New Arrivals

* `screenshot-chrome`: A revamp of `screenshot-service` to take advantage of the new Chrome headless mode
* `azure-sandbox`: A sandboxed environment to develop Azure solutions, containing a complete set of CLI tools and Visual Studio Code - [moved to a separate repository](https://github.com/rcarmo/azure-toolbox)
* `desktop-chrome`: The base for the above, containing Google Chrome, a resizable VNC server and a complete desktop environment.

## Older Stuff

Some of these templates are optimized for [DigitalOcean][do].

* `ink-builder`: A sandboxed environment to run the build tools for [Ink](https://github.com/sapo/Ink), because I can't abide putting up with it on the Mac.

* `dropbox-sandbox`: A sandboxed environment to run a Dropbox daemon, exposing the Dropbox directory as a Docker data volume.

* `browser-vnc`: A minimalist desktop environment with Firefox, Openbox, `fbpanel` and the Infinality font rendering tweaks, accessible via VNC over SSH (just log in and run `vncserver` to start it, then open an SSH tunnel to 5901). Can be trivially tweaked to not use SSH at all, but this is a more generic template that can be used for other purposes.

* `jekyll-playground`: A way to bootstrap a [Github Pages][ghh]/[Jekyll][jk] environment.

* `bq-prusa-i3-hephestos`: Bootstraps an Arduino development environment and builds a firmware image for the [bq Prusa i3 Hephestos][bq] 3D printer, which I'm [trying to get in working order][b1].

* `peervpn`: A test container to run [peervpn][peervpn] (contains insecure defaults, customize at will)

[jk]: http://jekyllrb.com/
[gh]: https://github.com/github/pages-gem
[ghh]: https://help.github.com/articles/using-jekyll-with-pages/
[do]: https://www.digitalocean.com/?refcode=5090627e4da5
[bq]: http://www.bqreaders.com/gb/products/prusa-hephestos.html
[b1]: http://the.taoofmac.com/space/blog/2014/11/01/1230#3d-printing-speed-bumps
[peervpn]: http://www.peervpn.net
