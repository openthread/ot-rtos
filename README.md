
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

### Nordic nRF52840

```sh
git submodule update --init
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -DPLATFORM_NAME=nrf52
make -j12
```

This will build the demo application of OpenThread command line in `build/ot_cli_nrf52840`, you can use the command line application by connecting to the serial port of nRF52840.

### Test HTTP Client

```
panid 0x1234
extpanid 1111111122222222
masterkey 00112233445566778899AABBCCDDEEFF
channel 15
networkname OpenThreadDemo
ifconfig up
thread start
test http
```

You should see http response

### Test MQTT Client
Please read the [quick start guide](https://cloud.google.com/iot/docs/quickstart) for google cloud iot core first and save your private key, also select RS256 for authentication.

You need to put your client config in `include/apps/google_cloud_iot/client_cfg.h`
A sample config is given below:

```
#ifndef OT_RTOS_CLIENT_CFG_HPP_
#define OT_RTOS_CLIENT_CFG_HPP_

#define CLOUDIOT_SERVER_ADDRESS "mqtt.googleapis.com"
#define CLOUDIOT_DEVICE_ID "{your_device_id}"
#define CLOUDIOT_REGISTRY_ID "{your_registry_name}"
#define CLOUDIOT_PROJECT_ID "{your_project_id}"
#define CLOUDIOT_REGION "{your_region}"
#define CLOUDIOT_CLIENT_ID                                                                                       \
    "projects/" CLOUDIOT_PROJECT_ID "/locations/" CLOUDIOT_REGION "/registries/" CLOUDIOT_REGISTRY_ID "/devices" \
    "/" CLOUDIOT_DEVICE_ID

#define CLOUDIOT_PRIV_KEY \
"-----BEGIN PRIVATE KEY-----\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"\
"xxxxxxxxxxxxxxxxxxxxxxxx\n"\
"-----END PRIVATE KEY-----"\

#define CLOUDIOT_CERT \
"-----BEGIN CERTIFICATE-----\n"\
"MIIEMjCCAxqgAwIBAgIBATANBgkqhkiG9w0BAQUFADB7MQswCQYDVQQGEwJHQjEb\n"\
"MBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYDVQQHDAdTYWxmb3JkMRow\n"\
"GAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UEAwwYQUFBIENlcnRpZmlj\n"\
"YXRlIFNlcnZpY2VzMB4XDTA0MDEwMTAwMDAwMFoXDTI4MTIzMTIzNTk1OVowezEL\n"\
"MAkGA1UEBhMCR0IxGzAZBgNVBAgMEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UE\n"\
"BwwHU2FsZm9yZDEaMBgGA1UECgwRQ29tb2RvIENBIExpbWl0ZWQxITAfBgNVBAMM\n"\
"GEFBQSBDZXJ0aWZpY2F0ZSBTZXJ2aWNlczCCASIwDQYJKoZIhvcNAQEBBQADggEP\n"\
"ADCCAQoCggEBAL5AnfRu4ep2hxxNRUSOvkbIgwadwSr+GB+O5AL686tdUIoWMQua\n"\
"BtDFcCLNSS1UY8y2bmhGC1Pqy0wkwLxyTurxFa70VJoSCsN6sjNg4tqJVfMiWPPe\n"\
"3M/vg4aijJRPn2jymJBGhCfHdr/jzDUsi14HZGWCwEiwqJH5YZ92IFCokcdmtet4\n"\
"YgNW8IoaE+oxox6gmf049vYnMlhvB/VruPsUK6+3qszWY19zjNoFmag4qMsXeDZR\n"\
"rOme9Hg6jc8P2ULimAyrL58OAd7vn5lJ8S3frHRNG5i1R8XlKdH5kBjHYpy+g8cm\n"\
"ez6KJcfA3Z3mNWgQIJ2P2N7Sw4ScDV7oL8kCAwEAAaOBwDCBvTAdBgNVHQ4EFgQU\n"\
"oBEKIz6W8Qfs4q8p74Klf9AwpLQwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQF\n"\
"MAMBAf8wewYDVR0fBHQwcjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5jb20v\n"\
"QUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNqA0oDKGMGh0dHA6Ly9jcmwuY29t\n"\
"b2RvLm5ldC9BQUFDZXJ0aWZpY2F0ZVNlcnZpY2VzLmNybDANBgkqhkiG9w0BAQUF\n"\
"AAOCAQEACFb8AvCb6P+k+tZ7xkSAzk/ExfYAWMymtrwUSWgEdujm7l3sAg9g1o1Q\n"\
"GE8mTgHj5rCl7r+8dFRBv/38ErjHT1r0iWAFf2C3BUrz9vHCv8S5dIa2LX1rzNLz\n"\
"Rt0vxuBqw8M0Ayx9lt1awg6nCpnBBYurDC/zXDrPbDdVCYfeU0BsWO/8tqtlbgT2\n"\
"G9w84FoVxp7Z8VlIMCFlA2zs6SFz7JsDoeA3raAVGI/6ugLOpyypEBMs1OUIJqsi\n"\
"l2D4kF501KKaU73yqWjgom7C12yxow+ev+to51byrvLjKzg6CYG1a4XXvi3tPxq3\n"\
"smPi9WIsgtRqAEFQ8TmDn5XpNpaYbg==\n"\
"-----END CERTIFICATE-----"

#endif
```

`CLOUDIOT_CERT` is the first certificate from [https://pki.google.com/roots.pem](https://pki.google.com/roots.pem)

You can use command `test mqtt` to test connect and publish a message to Google Cloud IoT core.

### TCP echo server and client

Commands:
- `tcp_echo_server port` starts TCP echo server on given `port`.
- `tcp_connect ipaddr port` connects to given TCP server.
- `tcp_send size count` sends `count` packets with given `size` to connected TCP server. At the end it prints statistics.
- `tcp_disconnect` disconnects from TCP server.

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
