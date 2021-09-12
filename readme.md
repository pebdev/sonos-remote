## Description ##

Control volume of your Sonos system with this remote.



## Hardware ##

List of hardware parts :

* ESP32 WROOM
* rotary KY-040 DS
* level shifter 
* push button



![sonos-remote-4](https://github.com/pebdev/sonos-remote/blob/master/img/sonos-remote-4.jpg)



Wiring :

- [KY-040 CLK] --- [LEVEL SHIFTER HV1] --- [LEVEL SHIFTER LV1] ---  [ESP32 PIN 4]
- [KY-040 DT] --- [LEVEL SHIFTER HV2] --- [LEVEL SHIFTER LV2] ---  [ESP32 PIN 2]
- [KY-040 SW] --- [LEVEL SHIFTER HV3]--- [LEVEL SHIFTER LV3] ---  [ESP32 PIN 15]
- [KY-040 +] --- [ESP32 VIN]
- [KY-040 GND] --- [ESP32 GND]
- [LEVEL SHIFTER LV] ---  [ESP32 3.3V]
- [LEVEL SHIFTER HV] ---  [ESP32 VIN]
- [LEVEL SHIFTER GND] ---  [ESP32 GND]
- [ESP32 USB] --- [USB charger]



## Mechanical

You can find an example of a box to print into the mechanical folder (Fusion 360 format).

![sonos-remote-1](https://github.com/pebdev/sonos-remote/blob/master/img/sonos-remote-1.PNG)

![sonos-remote-2](https://github.com/pebdev/sonos-remote/blob/master/img/sonos-remote-2.PNG)

![sonos-remote-3](https://github.com/pebdev/sonos-remote/blob/master/img/sonos-remote-3.jpg)



![sonos-remote-5](https://github.com/pebdev/sonos-remote/blob/master/img/sonos-remote-5.jpg)



Note : screw format = M2.5



## Software

### Dependencies ###

You must install two libraries (zip ready to use are provided into the dependencies folder or you can download these libraries directly from official repositories) :

* sonos (https://github.com/joeybab3/sonos)
* microxpath (https://github.com/tmittet/microxpath)

Note : I patched the sonos library, if you get it from official repo, think to do a diff on this function :

```
uint8_t SonosUPnP::getVolume(IPAddress speakerIP)
```

If you don't know how to do that, follow this [page](https://www.arduino.cc/en/guide/libraries) to install libraries.

You need support of the ESP32 board too, you can follow this [page](https://circuitdigest.com/microcontroller-projects/programming-esp32-with-arduino-ide).

Note : to avoid compilation issue, use ESP32 software 1.0.4



### Configuration ###

Just create *wifi_key.h* file (see ino file for more information).



### Flash ###

In the arduino IDE, when flashing, you have '_' after ''........', push the **BOOT** button of the VROOM board.

