
# OpenThread RTOS Demo 101

In demo 101, we provide a showcase of how to connect a device to Thread network and periodically visit a website via HTTP.

## Build and run the demo

### Prerequisites

So as to connect your thread device to Internet, you need to set up a border router. You may follow [this guide](https://openthread.io/guides/border-router/docker) for detailed instructions on how to set up a border router and how to form a network with it.

To configure Thread network parameters, you also need to start an external commissioner. You can use the command line commissioner shipped with the [border router repository](https://github.com/openthread/borderrouter). Follow this [guide](https://openthread.io/guides/border-router/build) locally build the project. The instruction to start the commissioner is:

```sh
./src/commissioner/otbr-commissioner --network-name OpenThreadDemo --xpanid 1111111122222222 --network-password 123456 --joiner-pskd ABCDEF --agent-host 127.0.0.1 --agent-port 49191 --allow-all --debug-level 7
```

All the network arguments should be the same as those used to form the Thread network in the web gui.

### Build the demo 101 image

```sh
cd ${PROJECT_ROOT_DIRECTORY}
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -DPLATFORM_NAME=nrf52
make -j12
```

The image will be found at `build/ot_demo_101.hex`. Flash the image to NRF52840 dev kit with following command:

```sh
nrfjprog -f nrf52 --chiperase --program ./ot_demo_101.hex --reset --log
```

Connect to the serial port of NRF52840 (typically `/dev/ttyACM0`). Log will be directed here.

### Run the demo

Push button 1 on the NRF52840 dev kit and it will start the Thread join process. After joining the Thread network, the node will periodically curl `www.google.com` and output the http response to log. Push button 1 again will stop the demo.

