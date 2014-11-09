FROM ubuntu:14.04
MAINTAINER rcarmo
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Baseline config for a CLI development environment
RUN apt-get update; apt-get upgrade -y; apt-get install -y man openssh-server htop tmux vim bash-completion rsync build-essential git; apt-get autoremove; apt-get clean
# A shell user with sudo 
ADD home/user /home/user
# The following injects the Vagrant public key, CHANGE IT if you really need to SSH into the container, otherwise just use docker-enter if you have Docker 1.3+
RUN useradd -G sudo -s /bin/bash user && mkdir -p /home/user/.ssh && echo 'ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA6NF8iallvQVp22WDkTkyrtvp9eWW6A8YVr+kz4TjGYe7gHzIw+niNltGEFHzD8+v1I2YJ6oXevct1YeS0o9HZyN1Q9qgCgzUFtdOKLv6IedplqoPkcmF0aYet2PkEDo3MlTBckFXPITAMzF8dJSIFo9D8HfdOV0IAdx4O7PtixWKn5y2hMNG0zQPyUecp4pzC6kivAIhyfHilFR61RGL+GPXQ2MWZWFYbAGjyiYJnAmCP3NOTd0jMZEnDkbUvxhMmBYSdETk1rRgm+R4LOzFUGaHqHDLKLX+FIPKcF96hrucXzcWyLbIbEgE98OHlnVYCzRdK8jlqm8tehUc9c9WhQ== vagrant insecure public key' > /home/user/.ssh/authorized_keys && chown -R user:user /home/user && sudo -u user ssh-keygen -q -t rsa -f /home/user/.ssh/id_rsa -N ""
RUN echo 'user:changeme' | chpasswd
# Make sure we can use SSH if we don't have docker-enter
RUN mkdir -p /var/run/sshd && chmod 755 /var/run/sshd && start ssh
# Grab the Arduino SDK
RUN cd /home/user; wget "http://arduino.cc/download.php?f=/arduino-1.0.6-linux64.tgz"; tar -zxvf download*.tgz; rm -f *.tgz
# Get the bq Marlin firmware tree
RUN cd /home/user; git clone https://github.com/bq/Marlin.git; cd Marlin; git checkout v1.3.1_hephestos
# Build a firmware image
RUN cd /home/user/Marlin/Marlin; ARDUINO_INSTALL_DIR=/home/user/arduino-1.0.6 make
VOLUME /target
ENV __FLUSH_LOG 1
# This is only really necessary if you can't use docker-enter (or if you want to do post-provisioning using fabric, etc.)
EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
