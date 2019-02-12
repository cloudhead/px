#!/bin/sh

output=${2-$1}

if [ -f $output ]; then
	cp $output $output\~                       # Backup original
fi

convert $1 -depth 32 -type TrueColor $output # Convert to RGBA/32-bit
convert $output -transparent black $output   # Set transparency color to black
