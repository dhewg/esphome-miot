## ESPHome components for MIoT devices

These [ESPHome](https://esphome.io/) components are designed for [MIoT devices](https://home.miot-spec.com/) which adhere to the [Xiaomi MIoT Serial Communication](https://github.com/blakadder/miot) protocol.

Such devices contain two microcontrollers, one actually controls the hardware, and the other acts as a LAN/cloud gateway.

These components allow you to replace the firmware on the latter, hence liberating your devices from the vendor cloud.

Since this uses [ESPHome](https://esphome.io/), adding your liberated devices to [Home Assistant](https://www.home-assistant.io/) becomes a breeze with the [official integration](https://www.home-assistant.io/integrations/esphome/):

![sensors](https://github.com/dhewg/esphome-miot/assets/605548/0354026c-4291-496a-a9ba-4b54424cf8bf)
![controls](https://github.com/dhewg/esphome-miot/assets/605548/86c4e970-0735-4541-bd6a-c2dbec90575f)
![config](https://github.com/dhewg/esphome-miot/assets/605548/afa2d4a8-04be-49f3-ac36-1098e6cdfa83)

## Supported devices

There are probably many more devices that could be supported, currently there are ESPHome configs for the following:

[Xiaomi Smart Air Purifier 4 Lite](config/zhimi.airp.rmb1.yaml)

## Building a firmware

Either download an [ESPHome config](config/) or create your own (see below) and feed it to ESPHome to build the firmware.

There's no need to clone this repo, unless you plan to contribute - which would be very welcome!

## Adding devices

First, look up the desired device on the [MIoT specs site](https://home.miot-spec.com/).

Each device defines its service (`SIID`) and property (`PIID`) IDs. You just have to add all the desired properties with their according IDs to your ESPHome yaml config.

For an example, see the [spec for the Xiaomi Smart Air Purifier 4 Lite](https://home.miot-spec.com/spec?type=urn%3Amiot-spec-v2%3Adevice%3Aair-purifier%3A0000A007%3Azhimi-rmb1%3A2) and its [ESPHome config file](config/zhimi.airp.rmb1.yaml).

Once your newly added device is working, please open a PR to add its config here!

## Inspired by
https://github.com/jaromeyer/mipurifier-esphome
