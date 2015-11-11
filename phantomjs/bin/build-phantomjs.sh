#!/bin/bash
# Install build requirements
sudo apt-get install -y build-essential g++ flex bison gperf ruby perl \
  libsqlite3-dev libfontconfig1-dev libicu-dev libfreetype6 libssl-dev \
  libpng-dev libjpeg-dev python git
# Clone repo
git config --global user.name "Docker User"; git clone https://github.com/ariya/phantomjs.git /home/phantomjs
# Check it out and perform the build
cd /home/phantomjs; git checkout 2.0; ./build.sh --confirm; make clean
# Remove all the packages we don't need inside the container
apt-get remove -y build-essential g++ flex bison gperf ruby perl python git; apt-get autoremove -y; apt-get clean
# Put the binary someplace obvious and remove the source tree
mv /home/phantomjs/bin/phantomjs /bin/phantomjs; rm -rf /home/phantomjs
