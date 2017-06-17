FROM ubuntu:16.04
MAINTAINER rcarmo

RUN apt-get update -y \
 && apt-get dist-upgrade -y \
 && apt-get install -y \
	ca-certificates \
	fonts-liberation \
	fonts-roboto \
	gconf-service \
	hicolor-icon-theme \
	libappindicator1 \
	libasound2 \
	libcanberra-gtk-module \
	libcurl3 \
	libexif-dev \
	libfontconfig1 \
	libfreetype6 \
	libgconf-2-4 \
	libgl1-mesa-dri \
	libgl1-mesa-glx \
	libnspr4 \
	libnss3 \
	libpango1.0-0 \
	libv4l-0 \
	libxss1 \
	libxtst6 \
	lsb-base \
	strace \
	wget \
	xdg-utils \
	fonts-symbola

 
RUN \
    wget https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb -O /tmp/google-chrome-stable_current_amd64.deb \
 && dpkg -i /tmp/google-chrome-stable_current_amd64.deb \
	&& rm -rf /var/lib/apt/lists/* \
	&& rm -rf /tmp/*.deb

ENTRYPOINT [ "google-chrome" ]
CMD [ "--user-data-dir=/data" ]
