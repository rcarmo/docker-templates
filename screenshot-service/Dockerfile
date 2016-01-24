FROM rcarmo/phantomjs:2.1
MAINTAINER rcarmo
ADD app /app
USER www-data
WORKDIR /app
EXPOSE 8000
CMD ["phantomjs","--web-security=false","--ignore-ssl-errors=true","--ssl-protocol=any","server.js"]
