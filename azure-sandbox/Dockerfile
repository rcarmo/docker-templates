FROM rcarmo/desktop-chrome
MAINTAINER rcarmo
# Java
RUN echo "oracle-java8-installer shared/accepted-oracle-license-v1-1 select true" | debconf-set-selections && add-apt-repository -y ppa:webupd8team/java; apt-get update; apt-get install -y oracle-java8-installer
# NodeJS
RUN curl -sL https://deb.nodesource.com/setup_6.x | sudo -E bash -
# Runtimes
RUN apt-get update; apt-get upgrade -y; apt-get install -y python-pip libssl-dev nodejs npm oracle-java8-installer libnotify4 libffi-dev firefox
# Azure CLI && AZ
RUN npm install -g azure-cli && pip install --upgrade pip pycparser && pip install azure-cli 
# Visual Studio Code
RUN wget https://go.microsoft.com/fwlink/?LinkID=760868 -O /tmp/vscode.deb && dpkg -i /tmp/vscode.deb && mkdir -p /opt/patches/lib && cp /usr/lib/x86_64-linux-gnu/libxcb.so.1 /opt/patches/lib && sed -i 's/BIG-REQUESTS/_IG-REQUESTS/' /opt/patches/lib/libxcb.so.1 && sed -i 's/Exec=/Exec=env LD_LIBRARY_PATH\\=\/opt\/patches\/lib /' /usr/share/applications/code.desktop && rm /tmp/vscode.deb
# For Windows users who don't know how to tunnel in via SSH
EXPOSE 5901
