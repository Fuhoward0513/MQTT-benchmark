# MQTT-benchmark
This is a MQTT package using mosquitto broker to benchmarking and comparing the performance between different middlewares.


## Build Docker
The customized mqtt-image is based on Debian: bookworm (https://hub.docker.com/_/debian)

### Build from Debain: bookworm
#### Install sudo
    $ su root
    $ apt-get update
    $ apt-get install sudo
    $ exit

#### Install git
    $ apt-get update
    $ sudo apt install git

#### Install make
    $ sudo apt update && sudo apt upgrade -y
    $ sudo apt install -y make

#### Install cjson
the message is write in .json format

    $ wget https://github.com/DaveGamble/cJSON/archive/v1.7.14.tar.gz
    $ tar -xzvf v1.7.14.tar.gz
    $ cd cJSON-1.7.14
    $ make 
    $ sudo make install
    $ sudo ln -s /usr/local/lib/libcjson.so /usr/lib/libcjson.so

#### Install paho.mqtt.c
    $ git clone https://github.com/eclipse/paho.mqtt.c.git
    $ apt-get install build-essential gcc make cmake cmake-gui cmake-curses-gui
    $ apt-get install fakeroot devscripts dh-make lsb-release
    $ apt-get install libssl-dev
    $ apt-get install doxygen graphviz

    $ mkdir /tmp/build.paho ; cd /tmp/build.paho
    $ cmake -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=TRUE \
        -DPAHO_BUILD_SAMPLES=TRUE ~/paho.mqtt.c
    $ ccmake ~/paho.mqtt.c
    $ cmake --build .

    $ cmake --build . --target install

    $ cmake --build . --target package

#### Install broker
mosquitto for Debian(mosquitto-jessie): https://mosquitto.org/blog/2013/01/mosquitto-debian-repository/


### Pull docker image
    docker pull zihhaofu0513/mqtt-image:version2

### create container
    docker run -it -d --rm --name mqtt-container -v $PWD/mqtt/:/root/mqtt zihhaofu0513/mqtt-image:version2

## Run test
    ~/src# bash run.sh
