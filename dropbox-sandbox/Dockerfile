FROM ubuntu:16.04
MAINTAINER rcarmo

# Ugrade to a clean slate and install the basics we need to install and run Dropbox
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update \
 && apt-get dist-upgrade -y \
 && apt-get autoremove -y \
 && apt-get install -y \
        wget \
        python \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

RUN adduser --disabled-password --gecos "" -u 1001 user \
 && wget -O /usr/local/bin/dropbox "https://www.dropbox.com/download?dl=packages/dropbox.py" \
 && chmod +x /usr/local/bin/dropbox

USER user
RUN cd \
 && wget -O db.tar.gz "https://www.dropbox.com/download/?plat=lnx.x86_64" \
 && tar -zxvf db.tar.gz \
 && rm db.tar.gz
 
# Expose the Dropbox directory as a Docker volume
VOLUME ["/home/user/Dropbox"]

# Disable LAN sync (saves you the time to docker exec inside and do it yourself)
RUN dropbox start && dropbox lansync n
CMD ["/home/user/.dropbox-dist/dropboxd"]

ARG VCS_REF
ARG VCS_URL
ARG BUILD_DATE
LABEL org.label-schema.vcs-ref=$VCS_REF \
      org.label-schema.vcs-url=$VCS_URL \
      org.label-schema.build-date=$BUILD_DATE
