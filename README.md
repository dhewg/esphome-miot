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

Device | Model Version | Wiki | ESPHome Config | MIoT Specification
---|---|---|---|---
Mi Air Purifier 3H | zhimi.airpurifier.mb3 | [link](../../wiki/Xiaomi-Mi-Air-Purifier-3H) | [zhimi.airpurifier.mb3](config/zhimi.airpurifier.mb3.yaml) | [link](https://home.miot-spec.com/spec/zhimi.airpurifier.mb3)
Mi Air Purifier 3C | zhimi.airp.mb4a <br> zhimi.airpurifier.mb4 |[link](../../wiki/Xiaomi-Mi-Air-Purifier-3C) | [zhimi.airp.mb4a](config/zhimi.airp.mb4a.yaml) | [link](https://home.miot-spec.com/spec/zhimi.airp.mb4a) <br> [link](https://home.miot-spec.com/spec/zhimi.airpurifier.mb4)
Xiaomi Smart Air Purifier 4 | zhimi.airp.mb5 | [link](../../wiki/Xiaomi-Smart-Air-Purifier-4) | [zhimi.airp.mb5](config/zhimi.airp.mb5.yaml) | [link](https://home.miot-spec.com/spec/zhimi.airp.mb5)
Xiaomi Smart Air Purifier 4 Lite | zhimi.airp.rmb1 | [link](../../wiki/Xiaomi-Smart-Air-Purifier-4-Lite-(zhimi.airp.rmb1)) | [zhimi.airp.rmb1](config/zhimi.airp.rmb1.yaml) | [link](https://home.miot-spec.com/spec/zhimi.airp.rmb1)
Xiaomi Smart Air Purifier 4 Pro | zhimi.airp.vb4 |  | [zhimi.airp.vb4](config/zhimi.airp.vb4.yaml) | [link](https://home.miot-spec.com/spec?type=urn:miot-spec-v2:device:air-purifier:0000A007:zhimi-vb4:1)
Xiaomi Mi Smart Standing Fan 2 | dmaker.fan.p18 |  | [dmaker.fan.p18](config/dmaker.fan.p18.yaml) | [link](https://home.miot-spec.com/spec/dmaker.fan.p18)

Some of the devices have more than one model (like Mi Air Purifier 3C). If their MIoT specifications are compatible, the ESPHome config will be usable with all of them.

## Not supported devices

Some devices have Xiaomi proprietary chip instead of ESP32 chip. Those devices are not supported.

Device | Model Version | Wiki | MIoT Specification
---|---|---|---
Mi Air Purifier 2S | zhimi.airpurifier.mc1 | [link](../../wiki/Xiaomi-Mi-Air-Purifier-2S)|[link](https://home.miot-spec.com/spec/zhimi.airpurifier.mc1)

## Building a firmware

Either download an [ESPHome config](config/) or create your own (see below) and feed it to ESPHome to build the firmware.

There's no need to clone this repo, unless you plan to contribute - which would be very welcome!

## Adding devices

First, look up the desired device on the [Xiaomi MIoT Spec](https://home.miot-spec.com/) site.

Each device defines its service (`SIID`) and property (`PIID`) IDs. You just have to add all the desired properties with their according IDs to your ESPHome yaml config.

For examples, see the [supported devices](#supported-devices) table above and compare a config against its specification.

Once your newly added device is working, please open a PR to add its config here!

## Feedback

Please feel free to open [issues](../../issues) and/or [pull requests](../../pulls) here.

Alternatively, there's a [thread](https://community.home-assistant.io/t/esphome-components-for-miot-devices/686646) on the official ESPHome forums.

## Inspired by
https://github.com/jaromeyer/mipurifier-esphome
