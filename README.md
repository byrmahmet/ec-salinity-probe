[![Build Status](https://travis-ci.org/u-fire/ec-salinity-probe.svg?branch=master)](https://travis-ci.org/u-fire/ec-salinity-probe)
[![Codacy grade](https://img.shields.io/codacy/grade/9230f66847aa45148465aa5381e6e57d.svg)]()

### EC_Salinity Probe Interface

>Measure Electrical Conductivity in Siemens, Total Dissolved Solids in PPM, and Salinity in PSU and PPT. Monitor hydroponic nutrient levels, salinity in aquariums, pools, or soil.
* Electrical Conductivity in Siemens
* Total Dissolved Solids in PPM
* Salinity in PSU and PPT

[Buy one](https://www.tindie.com/products/ufire/ec-salinity-probe-interface/)    
or make one with the [source code](https://www.github.com/u-fire/ec-salinity-probe) and [schematics](https://upverter.com/ufire/6cab745dd03f6f6d/EC-Salinity-Probe-2018a/).  

#### What it is
An ATTiny85 programmed as an I2C slave, a DS18B20 waterproof temperature probe, and a two-electrode EC probe. It measures conductance and converts it into a temperature-compensated Siemen. From that value, it derives PPM and salinity. Any two-electrode probe can be used.

#### Using it
There is extensive [documentation](http://ufire.co/ECSalinity/) on the use and setup of the device. This library is available in the Particle.io IDE, and a [python implementation](https://github.com/u-fire/ECSalinity-python) for Raspberry Pi and MicroPython is also available.

#### Compiling
This is a [PlatformIO](http://platformio.org/) project. Download and install it, import this repo, and it should download all the required tools for you. It expects a USBTiny device to upload the firmware.
