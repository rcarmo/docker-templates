#!/bin/bash
TARBALL=https://bintray.com/tigervnc/stable/download_file?file_path=tigervnc-1.7.1.x86_64.tar.gz
CHECKSUM=cdb2df7d96c6acca62e01deceee3ea1c8468606064cc51a162da413e0dd2cff5
cd /tmp
SHA256SUM=`wget -O - $TARBALL | tee dist.tgz | sha256sum | cut -d\  -f 1`
if [ $SHA256SUM = $CHECKSUM ]; then
  echo "Checksum OK, overwriting existing VNC installation"
  tar -zxvf dist.tgz --strip 1 -C /
  rm dist.tgz
  exit 0
fi
echo "Invalid checksum"
exit 1
