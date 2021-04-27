#!/bin/bash

set -e

if ! [ -f $1 ] 
then
	exit 1
fi

echo filtering
./build/scenecutremover $1 output.mp4
echo adding audio
ffmpeg -i output.mp4 -i $1 -c copy -map 0:v:0 -map 1:a:0 ${1%%.*}_filter.mp4
