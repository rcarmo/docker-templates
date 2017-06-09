#!/bin/bash
TARBALL=https://bintray.com/tigervnc/stable/download_file?file_path=tigervnc-1.8.0.x86_64.tar.gz
CHECKSUM=be6b51016c27cbc854c37ec5379bd89d74d15387bf103ff7cd1c8c2924f164a7
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
