[![Stories in Ready](https://badge.waffle.io/rcarmo/docker-templates.png?label=ready&title=Ready)](https://waffle.io/rcarmo/docker-templates)
docker-templates
================

A set of Docker templates (Dockerfiles and associated scripts):

* `browser-vnc`: A minimalist desktop environment with Firefox, Openbox, `fbpanel` and the Infinality font rendering tweaks, accessible via VNC over SSH (just log in and run `vncserver` to start it, then open an SSH tunnel to 5901). Can be trivially tweaked to not use SSH at all, but this is a more generic template that can be used for other purposes.


## Bootstrapping a new Ubuntu 14.04 LTS instance on DigitalOcean:

```
# Change timezone to something sane
dpkg-reconfigure tzdata
# Install the latest Docker version
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 36A1D7869245C8950F966E92D8576A8BA88D21E9
sh -c "echo deb https://get.docker.com/ubuntu docker main\
> /etc/apt/sources.list.d/docker.list"
apt-get update
apt-get dist-upgrade
apt-get autoremove
apt-get install lxc-docker monit varnish vim tmux htop
# install nsenter into /usr/local/bin
docker run --rm -v /usr/local/bin:/target jpetazzo/nsenter
git clone https://github.com/rcarmo/docker-templates
# profit!
```
