# Ostinato

[![Build Status](https://travis-ci.org/pstavirs/ostinato.svg?branch=master)](https://travis-ci.org/pstavirs/ostinato)

Ostinato is an open-source, cross-platform network packet crafter/traffic generator and analyzer with a friendly GUI. Craft and send packets of several streams with different protocols at different rates. 

Ostinato aims to be "Wireshark in Reverse" and become complementary to Wireshark.

License: GPLv3+ (see [COPYING](https://raw.githubusercontent.com/pstavirs/ostinato/master/COPYING))

For more information visit http://ostinato.org.

# Building Example

## Ubuntu 14.04 LTS

1. sudo apt-get install qt4-qmake protobuf-compiler qt4-dev-tools libprotoc-dev libpcap-dev
2. qmake
3. make
4. make install
