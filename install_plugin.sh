
#!/bin/bash
sudo apt-get update && sudo apt-get install -y capnproto libcapnp-dev autoconf automake libtool
mkdir -p c-capnproto-src && cd c-capnproto-src
curl -L -o plugin.zip "https://github.com"
unzip plugin.zip
cd c-capnproto-master
autoreconf -fiv && ./configure && make && sudo make install
cd ../.. && rm -rf c-capnproto-src install_plugin.sh
echo "--- INSTALLATION COMPLETE ---"
