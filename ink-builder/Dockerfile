FROM ubuntu:14.04
MAINTAINER rcarmo
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Ugrade to a clean slate
RUN apt-get update; apt-get upgrade -y; apt-get clean
# Install gem and compiler
RUN apt-get update; apt-get upgrade -y; apt-get install -y python-software-properties python g++ make ruby ruby-dev; gem install compass; apt-get clean
# Install nodejs
RUN apt-get update; apt-get install -y nodejs npm
# Fix node for root
RUN ln -s /usr/bin/nodejs /usr/bin/node
# Install NPM modules
RUN npm install -g async eslint grunt grunt-cli grunt-bower-task grunt-bump grunt-contrib-clean grunt-contrib-compass grunt-contrib-compress grunt-contrib-concat grunt-contrib-connect grunt-contrib-copy grunt-contrib-cssmin grunt-contrib-jshint grunt-contrib-qunit grunt-contrib-uglify grunt-contrib-watch grunt-lib-phantomjs inkdoc jit-grunt phantomjs
RUN npm install -g bower bower-config bower-logger font-awesome animate.css
