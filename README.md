[![Stories in Ready](https://badge.waffle.io/rcarmo/docker-templates.png?label=ready&title=Ready)](https://waffle.io/rcarmo/docker-templates)
docker-templates
================

A set of Docker templates (Dockerfiles and associated scripts). These templates are optimized for [DigitalOcean][do], which is where I run most of my fire-and-forget environments.

* `dropbox-sandbox`: A sandboxed environment to run a Dropbox daemon, exposing the Dropbox directory as a Docker data volume.

* `browser-vnc`: A minimalist desktop environment with Firefox, Openbox, `fbpanel` and the Infinality font rendering tweaks, accessible via VNC over SSH (just log in and run `vncserver` to start it, then open an SSH tunnel to 5901). Can be trivially tweaked to not use SSH at all, but this is a more generic template that can be used for other purposes.

* `jekyll-playground`: A way to bootstrap a [Github Pages][ghh]/[Jekyll][jk] environment.

* `bq-prusa-i3-hephestos`: Bootstraps an Arduino development environment and builds a firmware image for the [bq Prusa i3 Hephestos][bq] 3D printer, which I'm [trying to get in working order][b1].

* `peervpn`: A test container to run [peervpn][peervpn] (contains insecure defaults, customize at will)

## Bootstrapping a new Ubuntu 14.04 LTS Docker host on [DigitalOcean][do] and [Linode](http://www.linode.com):

This is what I normally do when I need a fresh Ubuntu Docker host these days. 

```
# Change timezone to something sane
sudo dpkg-reconfigure tzdata
# Install the latest Docker version
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 36A1D7869245C8950F966E92D8576A8BA88D21E9
sudo sh -c "echo deb https://get.docker.com/ubuntu docker main\
> /etc/apt/sources.list.d/docker.list"
sudo apt-get update
sudo apt-get dist-upgrade -y
sudo apt-get autoremove
sudo apt-get clean
sudo apt-get install -y lxc-docker monit varnish vim tmux htop
# now make sure we can invoke docker without sudo
sudo groupadd docker
sudo gpasswd -a ${USER} docker
sudo service docker restart
newgrp docker
# I actually use a private repo with other templates, but you can do this:
git clone https://github.com/rcarmo/docker-templates
# profit!
```

If running a container host inside a local VM, I also do this to make it easier to SSH in from a Mac:

```
apt-get install -y avahi-daemon
cp /usr/share/doc/avahi-daemon/examples/ssh.service /etc/avahi/services/.
```


[jk]: http://jekyllrb.com/
[gh]: https://github.com/github/pages-gem
[ghh]: https://help.github.com/articles/using-jekyll-with-pages/
[do]: https://www.digitalocean.com/?refcode=5090627e4da5
[bq]: http://www.bqreaders.com/gb/products/prusa-hephestos.html
[b1]: http://the.taoofmac.com/space/blog/2014/11/01/1230#3d-printing-speed-bumps
[peervpn]: http://www.peervpn.net
