FROM ubuntu:14.04
MAINTAINER rcarmo
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Ugrade to a clean slate
RUN apt-get update; apt-get upgrade -y; apt-get clean
# Baseline config for a CLI environment
RUN apt-get update; apt-get upgrade -y; apt-get install -y man openssh-server htop tmux vim bash-completion rsync; apt-get autoremove; apt-get clean
# Add our homedir with prebuilt config files
RUN apt-get update; apt-get upgrade -y; apt-get install -y ruby ruby-dev python-pygments; gem install  github-pages jekyll-sitemap; apt-get clean
ADD home/user /home/user
# A shell user with sudo and a throwaway SSH key
RUN useradd -G sudo -s /bin/bash user && chown -R user:user /home/user && sudo -u user ssh-keygen -q -t rsa -f /home/user/.ssh/id_rsa -N ""
RUN echo 'user:changeme' | chpasswd
# Use this instead if you'd rather login via SSH (this is the Vagrant public key, CHANGE IT!)
# RUN useradd -G sudo -s /bin/bash user && mkdir -p /home/user/.ssh && echo 'ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA6NF8iallvQVp22WDkTkyrtvp9eWW6A8YVr+kz4TjGYe7gHzIw+niNltGEFHzD8+v1I2YJ6oXevct1YeS0o9HZyN1Q9qgCgzUFtdOKLv6IedplqoPkcmF0aYet2PkEDo3MlTBckFXPITAMzF8dJSIFo9D8HfdOV0IAdx4O7PtixWKn5y2hMNG0zQPyUecp4pzC6kivAIhyfHilFR61RGL+GPXQ2MWZWFYbAGjyiYJnAmCP3NOTd0jMZEnDkbUvxhMmBYSdETk1rRgm+R4LOzFUGaHqHDLKLX+FIPKcF96hrucXzcWyLbIbEgE98OHlnVYCzRdK8jlqm8tehUc9c9WhQ== vagrant insecure public key' > /home/user/.ssh/authorized_keys && chown -R user:user /home/user && sudo -u user ssh-keygen -q -t rsa -f /home/user/.ssh/id_rsa -N ""
RUN mkdir -p /var/run/sshd && chmod 755 /var/run/sshd && start ssh
ENV __FLUSH_LOG 1
EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
