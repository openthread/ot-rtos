# OpenThread RTOS AoGH

This example provides a UDP server which receives and handles UDP packets from associated AoGH-based device.

See [Actions on Google - Local execution](https://developers.google.com/actions/smarthome/concepts/local) for more details.

## Build and run the demo

```sh
cd ${PROJECT_ROOT_DIRECTORY}
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -DPLATFORM_NAME=nrf52
make -j12
```

The image will be found at `build/ot_aogh.hex`. Flash the image to NRF52840 dev kit with following command:

```sh
nrfjprog -f nrf52 --chiperase --program ot_aogh.hex --reset --log
```

Connect to the serial port of NRF52840 (typically `/dev/ttyACM0`). Log will be directed here.

## Interact with it

1. Link the Google Home device (or other similar Google product) to your Google account in your Home App.

2. Follow the [doc](https://developers.google.com/actions/smarthome/develop/local#2_implement_the) to implement the local execution app for required handling actions.

* IDENTIFY: identify the NRF52840 as `action.devices.types.LIGHT` with 2 traits (`action.devices.traits.OnOff` and `action.devices.traits.Brightness`)
* EXECUTE: translate the command to bytes. Send one byte `00` for Off and one byte `01` for On. Send two bytes `02` || `brightness_value_in_hex_radix` for brightness trait (eg. send `024E` for 78% brightness).

3. Run the following command on your Google Home to link it with the NRF52840 via Thread.

```sh
# Firmware update
stop wpantund
ncp-ctl update

# Create network
start wpantund
wpanctl leave
wpanctl status
wpanctl form -c 14 -p 0xbeef -x DEADFFBEEFFFCAFE -k 00112233445566778899aabbccddeeff OpenThread-AoGH

# Wait for 5s

wpanctl scan
# Expected scan result:
#    | Joinable | NetworkName        | PAN ID | Ch | XPanID           | HWAddr           | RSSI
# ---+----------+--------------------+--------+----+------------------+------------------+------
#  1 |       NO | "Openthread-AoGH"  | 0xBEEF | 14 | DEADFFBEEFFFCAFE | 3A119719AB28BA58 |  -23

wpanctl status
# Expected status:
# wpan0 => [
# 	"NCP:State" => "associated"
# 	"Daemon:Enabled" => true
# 	"Config:NCP:DriverName" => "spinel"
# 	"NCP:HardwareAddress" => [9A12B004DD907A74]
# 	"NCP:Channel" => 14
# 	"Network:NodeType" => "leader"
# 	"Network:Name" => "Openthread-AoGH"
# 	"Network:XPANID" => 0xDEADFFBEEFFFCAFE
# 	"Network:PANID" => 0xBEEF
# 	"IPv6:LinkLocalAddress" => "fe80::807f:1f17:340:dfd1"
# 	"IPv6:MeshLocalAddress" => "fdde:adff:beef:0:295a:acff:f82:bcf8"
# 	"IPv6:MeshLocalPrefix" => "fdde:adff:beef::/64"
# 	"com.nestlabs.internal:Network:AllowingJoin" => false
# ]
```

4. Enable the UDP scanner to send IPv6 multicast to `[ff03::1]:30000` from the Thread network interface (run `ifconfig` to see its name, typically `wpan0`). The NRF52840 will receive the multicast packet and respond its mac address. Then the Google Home device should identify the scanned device as a light and cache it.

5. In Home App, press the plus icon to add some new devices. Choose the option about setting some smart lights. Name your NRF52840, choose a room and it will be added into your house.

6. Back to the homepage, choose the smart light and use the panel to control the light. You can turn on or off all the LEDs or set the brightness (every time increase 25% brightness, it will light one LED). Another way is to talk with the Google Home like "OK Google, turn on my light" or "OK Google, set light brightness to 78".
