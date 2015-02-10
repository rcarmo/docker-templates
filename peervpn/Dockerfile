FROM ubuntu:14.04
MAINTAINER rcarmo
# Copy our DigitalOcean sources.list across
ADD etc/apt/sources.list /etc/apt/sources.list
# Ugrade to a clean slate
RUN apt-get update; apt-get upgrade -y; apt-get clean
# Baseline config for a CLI environment
RUN apt-get update; apt-get upgrade -y; apt-get install -y build-essential libssl-dev
ADD src /opt/src
RUN cd /opt/src/peervpn-0-042; make; mv peervpn /usr/local/sbin/peervpn
ADD etc/peervpn /etc/peervpn
RUN apt-get remove -y build-essential; apt-get autoremove -y
ENV __FLUSH_LOG 1
EXPOSE 7000/udp
CMD ["/usr/local/sbin/peervpn", "/etc/peervpn/peervpn.conf"]
