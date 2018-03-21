[![Build Status](https://travis-ci.org/u-fire/ec-salinity-probe.svg?branch=master)](https://travis-ci.org/u-fire/ec-salinity-probe)
[![Codacy grade](https://img.shields.io/codacy/grade/9230f66847aa45148465aa5381e6e57d.svg)]()

### EC-Salinity Probe Interface

>Monitor hydroponic nutrient levels or salinity in aquariums, pools, and soil.
* Electrical Conductivity in Siemens
* Total Dissolved Solids in PPM
* Salinity in PSU and PPT
* Temperature in Celsius/Fahrenheit

#### What it is
An I2C sensor device, an optional DS18B20 waterproof temperature probe, and a two-electrode EC probe. It measures the conductivity of a solution and converts it into Siemens (S). From that value, it derives total dissolved solids and salinity. The firmware on the device provides two [calibration options](http://ufire.co/ECSalinity/#calibration), single or dual point which can be used simultaneously. Temperature compensation is also provided in the firmware.

Multiple EC-Salinity probes can be connected to the same master device and in the same solution without causing interference. The [ISE Probe Interface](http://ufire.co/ise_interface.html) can be also used without any interference.

#### Using it
There is extensive [documentation](http://ufire.co/ECSalinity/) on the [specifications](http://ufire.co/ECSalinity/#characteristics), [setup](http://ufire.co/ECSalinity/#getting-started), and [use](http://ufire.co/ECSalinity/#use) of the device. The library to use it is in the Arduino and Particle.io IDE; a python implementation for Raspberry Pi and MicroPython is also available.

~~~
#include <ECSalinity.h>
EC_Salinity ec;
ec.measureEC();
~~~

#### Buy it
Visit [ufire.co](http://ufire.co) and buy a board and probe.

#### Compiling
This is a [PlatformIO](http://platformio.org/) project. Download and install it, import this repo, and it should download all the required tools for you. It expects a USBTiny device to upload the firmware.
