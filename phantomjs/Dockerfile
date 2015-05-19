FROM ubuntu:14.04
MAINTAINER rcarmo
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Ugrade to a clean slate
RUN apt-get update; apt-get upgrade -y; apt-get clean
# X11 and web fonts
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | sudo debconf-set-selections; apt-get install -y fonts-droid fonts-roboto fonts-liberation ttf-mscorefonts-installer; apt-get clean
# Infinality
RUN apt-get install -y software-properties-common; add-apt-repository ppa:no1wantdthisname/ppa; apt-get update; apt-get install -y fontconfig-infinality; cd /etc/fonts/infinality; bash ./infctl.sh setstyle osx; apt-get clean
# Add our build script and run it
ADD bin/build-phantomjs.sh /root/build-phantomjs.sh 
RUN /bin/bash /root/build-phantomjs.sh; rm /root/build-phantomjs.sh
