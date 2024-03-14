
# LAB 0

this is the lab to set up the environment

## Part 1

coding environment

### Step 1

install docker-compose and docker.io in ubuntu/wsl for a better expirence in coding
Get the dockerfile from advanced UNIX class
the user name in the `Dockerfile` is in line `groupadd` and `useradd`

### Step 2
create the data volume to be linked to the docker container
also check the port of the machine so that it is free of collision in `docker-compose.yml` file

run command to start the docker container
```
    docker-compose up -d
```

connect to the container with the command
```
    ssh -p <the port choosen> <user name>@localhost
```

### Step 3

install the crosscompiler, qemu and tty with the script `env.sh`
or just modify the dockerfile to include the things

### Step 4

use the Makefile to compile the program given `a.S` to an image file
test the file with command `make run`


## Part 2

testing the given hardware

### Step 1

using the given image `nycuos.img` create the bootable SD card for rpi
check the location of the SD card with commnd `lsblk`
use command `dd if=nycuos.img of=/dev/<SD card location>` to write it to the SD card

### Step 2

put the SD card into the rpi
connect the USB and uart pins correctly to the rpi

add the user to the permission group of `/dev/ttyUSB0` (should be in `dialout`)
check group with `ls -l /dev/ttyUSB0`
use command `sudo adduser $USER <the group of the /dev/ttyUSB0>`
logout and login to apply the change of the group

### Step 3

use command `screen /dev/ttyUSB0 115200` to connect to the screen of the uart output
