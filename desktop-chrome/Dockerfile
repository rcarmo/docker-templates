FROM ubuntu:16.04
MAINTAINER rcarmo
ENV DEBIAN_FRONTEND noninteractive
ENV __FLUSH_LOG 1

# Add our sources, which include universe and multiverse
ADD etc/apt/sources.list /etc/apt/sources.list

# Ugrade to a clean slate
RUN apt-get update \
 && apt-get upgrade -y \
 && apt-get install -y \
    ca-certificates \
    apt-transport-https \
    bash-completion \
    curl \
    openssh-server \
    htop \
    git \
    man \
    rsync \
    silversearcher-ag \
    sudo \
    tmux \ 
    vim \ 
    wget \
    language-pack-en-base \
 && apt-get autoremove -y \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* \
 && locale-gen en_US.UTF-8

# X11 and web fonts
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections \
 && apt-get update \
 && apt-get install -y \
     elementary-icon-theme \
     fbpanel \ 
     fonts-noto \
     fonts-roboto \
     fonts-liberation \
     lib32stdc++6 \
     lxappearance \ 
     lxterminal \ 
     openbox \
     pcmanfm \
     software-properties-common \
     ssh-askpass \
     ttf-mscorefonts-installer \
     vnc4server \
     xcursor-themes \
 && apt-get autoremove -y \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Infinality
RUN add-apt-repository ppa:no1wantdthisname/ppa \
 && apt-get update \
 && apt-get install -y fontconfig-infinality \
 && cd /etc/fonts/infinality \
 && bash ./infctl.sh setstyle osx \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Chrome
RUN wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | apt-key add - \
 && sh -c 'echo "deb [arch=amd64] http://dl.google.com/linux/chrome/deb/ stable main" >> /etc/apt/sources.list.d/google-chrome.list' \
 && apt-get update \
 && apt-get install -y google-chrome-stable \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* \
 && sed -i 's/google-chrome-stable/google-chrome-stable --no-sandbox /g' /usr/share/applications/google-chrome.desktop 

# Add our homedir with prebuilt config files
ADD home/user /home/user

# A shell user with sudo and a throwaway SSH key
RUN useradd -G sudo -s /bin/bash user \
 && chown -R user:user /home/user \
 && sudo -u user ssh-keygen -q -t rsa -f /home/user/.ssh/id_rsa -N "" \
 && echo 'user:changeme' | chpasswd
# Use this instead if you'd rather login via SSH (this is the Vagrant public key, CHANGE IT!)
# RUN useradd -G sudo -s /bin/bash user && mkdir -p /home/user/.ssh && echo 'ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA6NF8iallvQVp22WDkTkyrtvp9eWW6A8YVr+kz4TjGYe7gHzIw+niNltGEFHzD8+v1I2YJ6oXevct1YeS0o9HZyN1Q9qgCgzUFtdOKLv6IedplqoPkcmF0aYet2PkEDo3MlTBckFXPITAMzF8dJSIFo9D8HfdOV0IAdx4O7PtixWKn5y2hMNG0zQPyUecp4pzC6kivAIhyfHilFR61RGL+GPXQ2MWZWFYbAGjyiYJnAmCP3NOTd0jMZEnDkbUvxhMmBYSdETk1rRgm+R4LOzFUGaHqHDLKLX+FIPKcF96hrucXzcWyLbIbEgE98OHlnVYCzRdK8jlqm8tehUc9c9WhQ== vagrant insecure public key' > /home/user/.ssh/authorized_keys && chown -R user:user /home/user && sudo -u user ssh-keygen -q -t rsa -f /home/user/.ssh/id_rsa -N ""

# our VNC quickstart script and sundry
ADD /root /

# Filesystem cleanups
RUN rm -rf /var/lib/apt/lists/* \
 && rm -rf /var/log/* \
 && chmod +x /quickstart.sh \
 && mkdir -p /var/run/sshd \
 && chmod 755 /var/run/sshd \
 && /usr/sbin/sshd

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
