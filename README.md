## ESPHome components for MIoT devices

These [ESPHome](https://esphome.io/) components are designed for [MIoT devices](https://home.miot-spec.com/) which adhere to the [Xiaomi MIoT Serial Communication](https://github.com/blakadder/miot) protocol.

Such devices contain two microcontrollers, one actually controls the hardware, and the other acts as a LAN/cloud gateway.

These components allow you to replace the firmware on the latter, hence liberating your devices from the vendor cloud.

Since this uses [ESPHome](https://esphome.io/), adding your liberated devices to [Home Assistant](https://www.home-assistant.io/) becomes a breeze with the [official integration](https://www.home-assistant.io/integrations/esphome/):

![controls](https://github.com/dhewg/esphome-miot/assets/605548/279d997c-54d0-48df-9a50-9b2971350077)
![sensors](https://github.com/dhewg/esphome-miot/assets/605548/59bd38e6-13a7-41d9-a794-1ab3af165d0b)
![config](https://github.com/dhewg/esphome-miot/assets/605548/a834ad8a-0a83-4559-8d53-7538449e51d5)
![diag](https://github.com/dhewg/esphome-miot/assets/605548/6e73d82a-2c9d-4775-a065-49198f611811)

## Supported devices

There are probably many more devices that could be supported, currently there are ESPHome configs for the following:

[Xiaomi Smart Air Purifier 4 Lite](config/zhimi.airp.rmb1.yaml)
[Xiaomi Smart Air Purifier 4](config/zhimi.airp.mb5.yaml)

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
