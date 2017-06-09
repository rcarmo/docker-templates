FROM rcarmo/desktop-chrome
COPY ./root/tigervnc.sh /root/tigervnc.sh
RUN apt-get remove -y vnc4server && chmod +x /root/tigervnc.sh && bash /root/tigervnc.sh 
