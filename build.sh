#!/bin/bash
cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug -S . -B ./build && cd build && make -j4 -f Makefile
