# SX1276 LORA Library
Lora SX1276 / RFM95w library for pi and arduino

A c/c++ library to control a SX1276 (aka RFM95w) modem via SPI. Tested on raspberry pi and ESP32 (arduino ide). There are much better alternatives, but I created this as a learning experience. It is intended to provide maximum flexibility and accesses to all LORA features defined in the datasheet. While a privative system to prevent transmitting outside ISM bands is implemented, it is currently only valid for the EU868 ISM band. It is assumed you know what you are doing and have a SDR to hand to verify your transmissions!
