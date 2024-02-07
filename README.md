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

Device | Wiki | ESPHome Config | MIoT Specification
---|---|---|---
Mi Air Purifier 3C | | [zhimi.airp.mb4a](config/zhimi.airp.mb4a.yaml) | [link](https://home.miot-spec.com/spec/zhimi.airp.mb4a)
Xiaomi Smart Air Purifier 4 | | [zhimi.airp.mb5](config/zhimi.airp.mb5.yaml) | [link](https://home.miot-spec.com/spec/zhimi.airp.mb5)
Xiaomi Smart Air Purifier 4 Lite | [link](../../wiki/Xiaomi-Smart-Air-Purifier-4-Lite-(zhimi.airp.rmb1)) | [zhimi.airp.rmb1](config/zhimi.airp.rmb1.yaml) | [link](https://home.miot-spec.com/spec/zhimi.airp.rmb1)

## Building a firmware

Either download an [ESPHome config](config/) or create your own (see below) and feed it to ESPHome to build the firmware.

There's no need to clone this repo, unless you plan to contribute - which would be very welcome!

## Adding devices

First, look up the desired device on the [Xiaomi MIoT Spec](https://home.miot-spec.com/) site.

Each device defines its service (`SIID`) and property (`PIID`) IDs. You just have to add all the desired properties with their according IDs to your ESPHome yaml config.

For examples, see the [supported devices](#supported-devices) table above and compare a config against its specification.

Once your newly added device is working, please open a PR to add its config here!

## Inspired by
https://github.com/jaromeyer/mipurifier-esphome
