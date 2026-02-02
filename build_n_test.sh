#!/usr/bin/env bash
set -e

rm -rf build
mkdir build
cd build

cmake ..
make

echo -e "\nTests:"
./test_runner ./statdump_tool

echo -e "\nTest Table form:"
./statdump_tool t_ele_a.bin t_ele_b.bin t_ele_out.bin