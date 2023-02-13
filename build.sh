#!/bin/bash
cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -S . -B ./build \
	&& cd build \
	&& make -j4 -f Makefile \
	&& rm ./compile_commands.json \
	&& ln -s ./build/compile_commands.json ./
