#!/bin/sh

# if [ -z "$2" ]
# then
#     echo "wrong format"
# fi

file_name="$1"

file_size=`stat --print="%s" $file_name`
file_size_in_hex=`printf "0x%08x" $file_size`

echo "kernel8.img file size " $file_size
echo "kernel8.img file size in hex " $file_size_in_hex

echo $file_size_in_hex | xxd -r > size_

chmod +x size_