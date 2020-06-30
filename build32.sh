#!/bin/bash

rm -rf retrorun32

export AR=arm-linux-gnueabihf-ar
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
export LINK=arm-linux-gnueabihf-g++

make
mv retrorun retrorun32
strip retrorun32

