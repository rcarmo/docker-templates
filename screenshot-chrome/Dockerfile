FROM ubuntu:16.04
MAINTAINER rcarmo

# Base packages
RUN apt-get update -y \
 && apt-get dist-upgrade -y \
 && apt-get install -y \
    apt-transport-https \
	ca-certificates \
	fonts-liberation \
	fonts-noto \
	fonts-roboto \
	fonts-symbola \
	gconf-service \
	hicolor-icon-theme \
	language-pack-en-base \
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
	software-properties-common \
	strace \
	wget \
	xdg-utils \
 && locale-gen en_US.UTF-8 \
 && apt-get autoremove -y \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Install Infinality and web fonts
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections \
 && add-apt-repository ppa:no1wantdthisname/ppa \
 && apt-get update \
 && apt-get install -y \
    fontconfig-infinality \
	ttf-mscorefonts-installer \
 && cd /etc/fonts/infinality \
 && bash ./infctl.sh setstyle osx \
 && apt-get autoremove -y \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* 

ADD / rootfs

# Install Chrome
RUN wget https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb -O /tmp/chrome.deb \
 && dpkg -i /tmp/chrome.deb \
 && apt-get autoremove -y \
 && apt-get clean \
 && rm -rf /tmp/*.deb \
 && rm -rf /var/lib/apt/lists/*

# Install other tools (move these to initial install when image is stable)
RUN apt-get update \
 && apt-get install -y \
    pngquant \
	jpegoptim \
 && rm -rf /var/lib/apt/lists/* 

ENTRYPOINT [ "google-chrome" ]
CMD [ "--user-data-dir=/data" ]
