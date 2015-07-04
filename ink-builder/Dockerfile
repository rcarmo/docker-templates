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
