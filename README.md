docker-templates
================

A set of Docker templates (Dockerfiles and associated scripts):

* `browser-vnc`: A minimalist desktop environment with Firefox, Openbox, `fbpanel` and the Infinality font rendering tweaks, accessible via VNC over SSH (just log in and run `vncserver` to start it, then open an SSH tunnel to 5901). Can be trivially tweaked to not use SSH at all, but this is a more generic template that can be used for other purposes.
