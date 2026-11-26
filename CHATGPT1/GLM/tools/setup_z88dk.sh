#!/usr/bin/env bash
# Builds z88dk (ZX81 target) on Ubuntu 24. Needed: apt packages below + z88dk-src tarball from GitHub releases.
# apt-get install -y build-essential bison flex re2c m4 perl libxml2-dev libgmp-dev libboost-all-dev dos2unix zlib1g-dev \
#   libmodern-perl-perl libcapture-tiny-perl libpath-tiny-perl libtext-table-perl libdata-hexdump-perl libclone-perl \
#   libfile-slurp-perl libyaml-perl libregexp-common-perl libobject-tiny-perl libtext-diff-perl libio-all-perl
# tar xzf z88dk-src-*.tgz && cd z88dk && export BUILD_SDCC=0 BUILD_SDCC_HTTP=0 && ./build.sh -t zx81 -l   # sdcc download step may fail: not needed
# make -C libsrc TARGET=zx81 && make -C libsrc install TARGET=zx81
# export ZCCCFG=$PWD/lib/config PATH=$PWD/bin:$PATH
# Test: zcc +zx81 -vn -startup=1 -clib=default h.c -o h -create-app   -> h.P (hello world = 3.5 KB incl. runtime)
