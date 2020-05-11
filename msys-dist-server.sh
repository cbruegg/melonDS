#!/bin/bash

if [[ ! -x melonDS-server.exe ]]; then
	echo "Run this script from the directory you built melonDS-server."
	exit 1
fi

mkdir -p dist-server

for lib in $(ldd melonDS-server.exe | grep mingw | sed "s/.*=> //" | sed "s/(.*)//"); do
	cp "${lib}" dist-server
done

cp melonDS-server.exe romlist.bin dist-server
