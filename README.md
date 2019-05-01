[![Build Status][ot-rtos-travis-svg]][ot-rtos-travis]

[ot-rtos-travis]: https://travis-ci.org/openthread/ot-rtos
[ot-rtos-travis-svg]: https://travis-ci.org/openthread/ot-rtos.svg?branch=master

---

# OpenThread RTOS

The OpenThread RTOS project provides an integration of:

1. [OpenThread](https://github.com/openthread/openthread), an open-source implementation of the Thread networking protocol.
2. [LwIP](https://git.savannah.nongnu.org/git/lwip/lwip-contrib.git/), a small independent implementation of the TCP/IP protocol suite.
3. [FreeRTOS](https://www.freertos.org/), a real time operating system for microcontrollers.

OpenThread RTOS includes a number of application-layer demonstrations, including:

- [MQTT](http://mqtt.org/), a machine-to-machine (M2M)/"Internet of Things" connectivity protocol.
- [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol), the underlying protocol used by the World Wide Web.
- [TCP](https://en.wikipedia.org/wiki/Transmission_Control_Protocol), one of the main transport protocols in the Internet protocol suite.

## Getting started

### Linux simulation

```sh
git submodule update --init
mkdir build && cd build
cmake .. -DPLATFORM_NAME=linux
make -j12
```

This will build the CLI test application in `build/ot_cli_linux`.

### Nordic nRF52840

```sh
git submodule update --init
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -DPLATFORM_NAME=nrf52
make -j12
```
This will build the CLI test application in `build/ot_cli_nrf52840.hex`. You can flash the binary with `nrfjprog`([Download](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools)) and connecting to the nRF52840 DK serial port.
This will also build the demo application in `build/ot_demo_101`. See the [Demo 101 README](examples/apps/demo_101/README.md) for a description of the demo application.

# Contributing

We would love for you to contribute to OpenThread RTOS and help make it even better than it is today! See our [Contributing Guidelines](https://github.com/openthread/ot-rtos/blob/master/CONTRIBUTING.md) for more information.

Contributors are required to abide by our [Code of Conduct](https://github.com/openthread/ot-rtos/blob/master/CODE_OF_CONDUCT.md) and [Coding Conventions and Style Guide](https://github.com/openthread/ot-rtos/blob/master/STYLE_GUIDE.md).

We follow the philosophy of [Scripts to Rule Them All](https://github.com/github/scripts-to-rule-them-all).

# License

OpenThread RTOS is released under the [BSD 3-Clause license](https://github.com/openthread/ot-rtos/blob/master/LICENSE). See the [`LICENSE`](https://github.com/openthread/ot-rtos/blob/master/LICENSE) file for more information.

Please only use the OpenThread name and marks when accurately referencing this software distribution. Do not use the marks in a way that suggests you are endorsed by or otherwise affiliated with Nest, Google, or The Thread Group.

# Need help?

There are numerous avenues for OpenThread support:

* Bugs and feature requests — [submit to the Issue Tracker](https://github.com/openthread/ot-rtos/issues)
* Stack Overflow — [post questions using the `openthread` tag](http://stackoverflow.com/questions/tagged/openthread)
* Google Groups — [discussion and announcements at openthread-users](https://groups.google.com/forum/#!forum/openthread-users)

The openthread-users Google Group is the recommended place for users to discuss OpenThread and interact directly with the OpenThread team.

## OpenThread

To learn more about OpenThread, see the [OpenThread repository](https://github.com/openthread/openthread).
