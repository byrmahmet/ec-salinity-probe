// Copyright (c) 2017 Justin Decker
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/*!
   \file main.cpp
   \brief Main source file for EC Salinity probe firmware
 */

#include "main.h"

float Conductivity;                   /**< Used to store the intermediate EC value */
float Temperature;                    /**< Temperature in C */
float K;                              /**< Cell constant K of probe */

static const int pinResistance = 23;
static const int Resistor = 500;

/*!
   \brief Called when data is requested over the I2C bus. Sends #Conductivity,
   #Temperature, and #K
 */
void requestEvent()
{
                TinyWireS.send( *((uint8_t *)&Conductivity ) );
                TinyWireS.send( *((uint8_t *)&Conductivity + 1) );
                TinyWireS.send( *((uint8_t *)&Conductivity + 2) );
                TinyWireS.send( *((uint8_t *)&Conductivity + 3) );

                TinyWireS.send( *((uint8_t *)&Temperature ) );
                TinyWireS.send( *((uint8_t *)&Temperature + 1) );
                TinyWireS.send( *((uint8_t *)&Temperature + 2) );
                TinyWireS.send( *((uint8_t *)&Temperature + 3) );

                TinyWireS.send( *((uint8_t *)&K ) );
                TinyWireS.send( *((uint8_t *)&K + 1) );
                TinyWireS.send( *((uint8_t *)&K + 2) );
                TinyWireS.send( *((uint8_t *)&K + 3) );
}

/*!
   \brief Called when data is received over I2C bus
   \param howMany the number of bytes received
 */
void receiveEvent(uint8_t howMany)
{
        uint8_t cmd;

        cmd = TinyWireS.receive();
        if (cmd == START_MEASUREMENT)
        {
                startEC = true;
        }

        if (cmd == SET_K)
        {
                startK = true;
        }
}

/*!
   \brief Starts I2C, sets pin modes and reads #K from EEPROM
 */
void setup()
{
        TinyWireS.begin(EC_SALINITY);
        TinyWireS.onRequest(requestEvent);
        TinyWireS.onReceive(receiveEvent);

        pinMode(POWER_PIN, OUTPUT);

        K = readK();

        timer1_disable();

        ds18.setResolution(TEMP_12_BIT);

}

/*!
   \brief Checks #startEC and #startK for true and calls their routines
 */
void loop()
{
        set_sleep_mode(SLEEP_MODE_IDLE);
        adc_disable();
        ac_disable();
        sleep_enable();
        sleep_mode();
        sleep_disable();
        adc_enable();
        ac_enable();

        TinyWireS_stop_check();
        if (startEC)
        {
                readEC();
                readTemperature();
                startEC = false;
        }

        if (startK)
        {
                saveK();
                startK = false;
        }

}

/*!
   \brief Reads the electrical conductivity between #EC_PIN and #EC_POWER_PIN.
   Global variable #Conductivity holds the value. Further processings is done on the
   master device side to determine actual EC with cell constant and temperature
   compensation.
 */
void readEC()
{
        float inputV;
        float outputV;
        int analogRaw;

        digitalWrite(POWER_PIN, HIGH);
        analogRead(EC_PIN);
        analogRaw = analogRead(EC_PIN);
        digitalWrite(POWER_PIN, LOW);

        inputV = getVin();

        outputV = (inputV * analogRaw) / 1024.0;
        Conductivity = (outputV * (Resistor + pinResistance)) / (inputV - outputV) - pinResistance;
}

/*!
   \brief Reads the temperature in C from the connected DS18B20 device.
   Global variable #Temperature holds the value;
 */
void readTemperature()
{
        ds18.requestTemperatures();
        Temperature = ds18.getTempCByIndex(0);
}

/*!
   \brief Reads the cell constant #K from EEPROM.
   \return the cell constant float #K
 */
float readK()
{
        float k;
        EEPROM.get(0, k);
        return k;
}

/*!
   \brief Reads the value of #K from master device and saves it to EEPROM.
 */
void saveK()
{
        *((uint8_t *)&K) = TinyWireS.receive();
        *((uint8_t *)&K + 1) = TinyWireS.receive();
        *((uint8_t *)&K + 2) = TinyWireS.receive();
        *((uint8_t *)&K + 3) = TinyWireS.receive();
        EEPROM.put(0, K);
}
