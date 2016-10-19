FROM ubuntu:16.04
MAINTAINER rcarmo
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Ugrade to a clean slate
RUN apt-get update; apt-get upgrade -y; apt-get clean
# Baseline config for a CLI environment
RUN apt-get update; apt-get upgrade -y; apt-get install -y man openssh-server htop tmux vim bash-completion rsync git sudo wget silversearcher-ag; apt-get autoremove; apt-get clean
# X11 and web fonts
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | sudo debconf-set-selections; apt-get install -y vnc4server lxterminal fbpanel lxappearance elementary-icon-theme fonts-noto fonts-roboto fonts-liberation ttf-mscorefonts-installer ssh-askpass pcmanfm leafpad openbox xcursor-themes lib32stdc++6; apt-get clean
# Infinality
RUN apt-get install -y software-properties-common; add-apt-repository ppa:no1wantdthisname/ppa; apt-get update; apt-get install -y fontconfig-infinality; cd /etc/fonts/infinality; bash ./infctl.sh setstyle osx; apt-get clean
# Chrome
RUN wget https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb -P /tmp/ && dpkg -i /tmp/google-chrome-stable_current_amd64.deb || true && apt-get install -fy && rm -f /tmp/google-chrome-stable_current_amd64.deb
RUN sed -i 's/google-chrome-stable/google-chrome-stable --no-sandbox /' /usr/share/applications/google-chrome.desktop 
# Add our homedir with prebuilt config files
ADD home/user /home/user
# A shell user with sudo and a throwaway SSH key
RUN useradd -G sudo -s /bin/bash user && chown -R user:user /home/user && sudo -u user ssh-keygen -q -t rsa -f /home/user/.ssh/id_rsa -N ""
RUN echo 'user:changeme' | chpasswd
# Use this instead if you'd rather login via SSH (this is the Vagrant public key, CHANGE IT!)
# RUN useradd -G sudo -s /bin/bash user && mkdir -p /home/user/.ssh && echo 'ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA6NF8iallvQVp22WDkTkyrtvp9eWW6A8YVr+kz4TjGYe7gHzIw+niNltGEFHzD8+v1I2YJ6oXevct1YeS0o9HZyN1Q9qgCgzUFtdOKLv6IedplqoPkcmF0aYet2PkEDo3MlTBckFXPITAMzF8dJSIFo9D8HfdOV0IAdx4O7PtixWKn5y2hMNG0zQPyUecp4pzC6kivAIhyfHilFR61RGL+GPXQ2MWZWFYbAGjyiYJnAmCP3NOTd0jMZEnDkbUvxhMmBYSdETk1rRgm+R4LOzFUGaHqHDLKLX+FIPKcF96hrucXzcWyLbIbEgE98OHlnVYCzRdK8jlqm8tehUc9c9WhQ== vagrant insecure public key' > /home/user/.ssh/authorized_keys && chown -R user:user /home/user && sudo -u user ssh-keygen -q -t rsa -f /home/user/.ssh/id_rsa -N ""
COPY ./root/quickstart.sh /quickstart.sh
RUN chmod +x /quickstart.sh && mkdir -p /var/run/sshd && chmod 755 /var/run/sshd && /usr/sbin/sshd
ENV __FLUSH_LOG 1
EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
