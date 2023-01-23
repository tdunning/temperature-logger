# Temperature Logger
A simple wireless data logger that can read four temperature sensors

# Libraries and Board

This sketch uses the OneWire (2.3.7), DallasTemperature (3.9.0), and U8g2 (2.33.15) libraries available in the Arduino interface.

The processor board is an ESP8266 from Amica. The Arduino interface is happy talking to this board if you select
* Board: "NodeMCU 1.0 (ESP-12E Modeule)"
* Upload Speed: "115200"
* Flash Size: "4MB"

I am using version 3.0.2 of `esp8266` to support this board. This library is considerable down-rev, but the board involved is ancient 
so it is probably fine.

I use port `/dev/cu.SLAB_USBtoUART` on my Mac. Your mileage will vary, of course.

![image](https://user-images.githubusercontent.com/250490/214145581-dc9f662e-250f-4bd1-9050-0e51f0ca7057.png)


# Parts

See [these sensors](https://www.amazon.com/gp/product/B00KUNKR3M/) for compatible sensors.

For crimping the connectors you may find these useful:

* [a crimping tool](https://www.amazon.com/gp/product/B0B2L7FTY1), and

* [connectors](https://www.amazon.com/gp/product/B077X8XV2J)
