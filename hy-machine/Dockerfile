FROM busybox
MAINTAINER rcarmo
RUN wget 
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Ugrade to a clean slate and install the basics we need to install and run Dropbox
RUN apt-get update; apt-get upgrade -y; apt-get install -y wget python; apt-get autoremove; apt-get clean
RUN mkdir /home/dropbox && useradd -s /bin/bash dropbox && cd /home/dropbox && wget -O db.tar.gz "https://www.dropbox.com/download/?plat=lnx.x86_64" && tar -zxvf db.tar.gz && rm db.tar.gz && chown -R dropbox:dropbox /home/dropbox && echo 'dropbox:changeme' | chpasswd 
# Expose the Dropbox directory as a Docker volume
VOLUME ["/home/dropbox/Dropbox"]
RUN wget -O /usr/local/bin/dropbox "https://www.dropbox.com/download?dl=packages/dropbox.py" && chmod +x /usr/local/bin/dropbox
USER dropbox
# Disable LAN sync (saves you the time to docker exec inside and do it yourself)
RUN dropbox start && dropbox lansync n
CMD ["/home/dropbox/.dropbox-dist/dropboxd"]
