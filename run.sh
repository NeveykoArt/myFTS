#!/bin/bash

if [ "$#" == 1 ]; then
	LD_LIBRARY_PATH=build/debug/src/app/ java -jar build/debug/src/app/javasearcher.jar $1
else
	echo wrong input
fi
