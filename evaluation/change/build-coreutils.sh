#!/bin/bash

names=(
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.1.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.2.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.3.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.4.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.5.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.6.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.7.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.8.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.9.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.10.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.11.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.12.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.13.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.14.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.15.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.16.tar.gz
  http://ftp.gnu.org/gnu/coreutils/coreutils-8.17.tar.xz
)

dir="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

build_path=${dir}/build

[ ! -d ${build_path}32 ] || exit 0
[ ! -d ${build_path}64 ] || exit 0

for (( i = 0 ; i < ${#names[@]} ; i++ ))
do
  tmpdir=$(mktemp -d)
  (
    cd $tmpdir

    wget ${names[$i]}
    name=${names[$i]##*/}

    if echo "${name}" | grep -q '.tar.gz$'; then
      tar zxvf ${name}
    elif echo "${name}" | grep -q '.tar.xz$'; then
      tar xJvf ${name}
    fi

    cd ${name%.tar*}; ./configure --host=i686-pc-linux-gnu --prefix=${build_path}32/${name%.tar*} CC="gcc -m32 -march=i586" CXX="g++ -m32 -march=i586" LDFLAGS="-m32"; make; make install; make clean
    cd ${name%.tar*}; ./configure --prefix=${build_path}64/${name%.tar*}; make; make install
    echo 
    rm -rf $tmpdir
  )
done
