FROM ubuntu:14.04
MAINTAINER rcarmo
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Ugrade to a clean slate
RUN apt-get update; apt-get upgrade -y; apt-get clean
# Baseline config for a CLI environment
RUN apt-get update; apt-get upgrade -y; apt-get install -y man htop tmux vim bash-completion rsync wget; apt-get autoremove; apt-get clean
# X11 and web fonts
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | sudo debconf-set-selections; apt-get install -y fonts-droid fonts-roboto fonts-liberation ttf-mscorefonts-installer; apt-get clean
# Infinality
RUN apt-get install -y software-properties-common; add-apt-repository ppa:no1wantdthisname/ppa; apt-get update; apt-get install -y fontconfig-infinality; cd /etc/fonts/infinality; bash ./infctl.sh setstyle osx; apt-get clean
# PhantomJS
RUN sudo apt-get install -y build-essential g++ flex bison gperf ruby perl \
  libsqlite3-dev libfontconfig1-dev libicu-dev libfreetype6 libssl-dev \
  libpng-dev libjpeg-dev python git
RUN cd /home; git clone git://github.com/ariya/phantomjs.git; cd phantomjs; git checkout 2.0;
RUN cd /home/phantomjs; ./build.sh --confirm
