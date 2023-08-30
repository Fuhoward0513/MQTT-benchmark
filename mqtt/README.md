# MQTT-benchmark
Benchmark for MQTT's performance


# Build Docker
## Pull docker image
    docker pull zihhaofu0513/mqtt-image:version2

## create container
    docker run -it -d --rm --name mqtt-container -v $PWD/mqtt/:/root/mqtt zihhaofu0513/mqtt-image:version2
