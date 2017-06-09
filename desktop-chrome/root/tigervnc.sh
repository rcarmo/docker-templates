#!/bin/bash
TARBALL=https://bintray.com/tigervnc/stable/download_file?file_path=tigervnc-1.7.0.x86_64.tar.gz
CHECKSUM=c55506f222633d763f001d47aa8329ec7744e5da858ae8f03582db2c5b85390a
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
