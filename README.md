[![Build Status](https://travis-ci.org/u-fire/ec-salinity-probe.svg?branch=master)](https://travis-ci.org/u-fire/ec-salinity-probe)
[![Codacy grade](https://img.shields.io/codacy/grade/9230f66847aa45148465aa5381e6e57d.svg)]()

### EC_Salinity Probe Interface

>Monitor hydroponic nutrient levels, salinity in aquariums, the ocean,
or pools, measure soil salinity, monitor water quality
* Electrical Conductivity in Siemens
* Total Dissolved Solids in PPM
* Salinity in PSU and PPT

[Buy one](https://www.tindie.com/products/ufire/ec-salinity-probe-interface/)    
or make one with the [source code](https://www.github.com/u-fire/ec-salinity-probe) and [schematics](https://upverter.com/justind000/19cb71ec38391a95/EC-Salinity-Probe/).  

#### What it is
An ATTiny85 programmed as an I2C slave, a DS18B20 waterproof temperature probe, and a two-electrode EC probe. It measures conductance and converts it into a temperature-compensated Siemen. From that value, it derives PPM and salinity. It is accurate to ~0.5mS in seawater, and to within ~0.05mS in the hydroponic or freshwater range.

#### Using it
An Arduino-compatible [library](https://github.com/u-fire/ECSalinity) is provided to make using the probe easy and there is extensive [documentation](http://ufire.co/ECSalinity/) on the use and setup of the device.

~~~
#include <ECSalinity.h>
EC_Salinity ec;

mS = ec.measureEC(ec.tempCoefEC);
~~~

#### Compiling
This is a [PlatformIO](http://platformio.org/) project. Download and install it, import this repo, and it should download all the required tools for you. It expects a USBTiny device to upload the firmware.
