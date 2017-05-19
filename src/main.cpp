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
   \brief EC Salinity firmware rev 1
 */

#include <main.h>

void requestEvent()
{
        TinyWireS.send(*((uint8_t *)&i2c_register + reg_position));

        reg_position++;
        if (reg_position >= reg_size)
        {
                reg_position = 0;
        }
}

void receiveEvent(uint8_t howMany)
{
        if (howMany < 1 || howMany > 16)
        {
                return;
        }

        reg_position = TinyWireS.receive();
        howMany--;
        if (!howMany)
        {
                return;
        }

        while(howMany--)
        {
                *((uint8_t *)&i2c_register + reg_position) = TinyWireS.receive();

                if (reg_position == EC_TASK_REGISTER)
                {
                        if (i2c_register.TASK == EC_MEASURE_EC) runEC = true;
                        if (i2c_register.TASK == EC_MEASURE_TEMP) runTemp = true;
                        if (i2c_register.TASK == EC_CALIBRATE_K) runCalibrateK = true;
                        if (i2c_register.TASK == EC_CALIBRATE_LOW)runCalibrateLow = true;
                        if (i2c_register.TASK == EC_CALIBRATE_HIGH) runCalibrateHigh = true;
                }

                if (reg_position == EC_K_REGISTER) EEPROM.put(EC_K_REGISTER, i2c_register.K);
                //if (reg_position == EC_CONFIG_REGISTER) EEPROM.put(EC_CONFIG_REGISTER, i2c_register.CONFIG);
                //if (reg_position == EC_ACCURACY_REGISTER) EEPROM.put(EC_ACCURACY_REGISTER, i2c_register.accuracy);
                if (reg_position == EC_CALIBRATE_REFHIGH_REGISTER) EEPROM.put(EC_CALIBRATE_REFHIGH_REGISTER, i2c_register.referenceHigh);
                if (reg_position == EC_CALIBRATE_REFLOW_REGISTER) EEPROM.put(EC_CALIBRATE_REFLOW_REGISTER, i2c_register.referenceLow);
                if (reg_position == EC_CALIBRATE_READHIGH_REGISTER) EEPROM.put(EC_CALIBRATE_READHIGH_REGISTER, i2c_register.readingHigh);
                if (reg_position == EC_CALIBRATE_READLOW_REGISTER) EEPROM.put(EC_CALIBRATE_READLOW_REGISTER, i2c_register.readingLow);

                reg_position++;
                if (reg_position >= reg_size)
                {
                        reg_position = 0;
                }
        }

}

void setup()
{
        timer1_disable();
        ds18.setResolution(TEMP_12_BIT);
        pinMode(POWER_PIN, OUTPUT);

        i2c_register.CONFIG.useTempCompensation = true;
        i2c_register.CONFIG.useDualPoint = false;
        i2c_register.tempConstant = 0xFF;
        i2c_register.accuracy = 9;
        i2c_register.version = 1;

        EEPROM.get(EC_K_REGISTER, i2c_register.K);
        //EEPROM.get(EC_CONFIG_REGISTER, i2c_register.CONFIG);
        //EEPROM.get(EC_ACCURACY_REGISTER, i2c_register.accuracy);
        EEPROM.get(EC_CALIBRATE_REFHIGH_REGISTER, i2c_register.referenceHigh);
        EEPROM.get(EC_CALIBRATE_REFLOW_REGISTER, i2c_register.referenceLow);
        EEPROM.get(EC_CALIBRATE_READHIGH_REGISTER, i2c_register.readingHigh);
        EEPROM.get(EC_CALIBRATE_READLOW_REGISTER, i2c_register.readingLow);

        TinyWireS.begin(EC_SALINITY);
        TinyWireS.onReceive(receiveEvent);
        TinyWireS.onRequest(requestEvent);
}

void loop()
{
        sleep();

        TinyWireS_stop_check();

        if (runTemp)
        {
                ds18.requestTemperatures();
                i2c_register.tempC = ds18.getTempCByIndex(0);
                runTemp = false;
        }

        if (runEC)
        {
                i2c_register.mS = measureConductivity();
                runEC = false;
        }

        if (runCalibrateK)
        {
                calibrateK();
                runCalibrateK = false;
        }

        if (runCalibrateLow)
        {
                calibrateLow();
                runCalibrateLow = false;
        }

        if (runCalibrateHigh)
        {
                calibrateHigh();
                runCalibrateHigh = false;
        }
}

float measureConductivity()
{
        float inputV;
        float outputV;
        int analogRaw;
        RunningMedian conductivityMedian = RunningMedian(i2c_register.accuracy);
        int middleThird = i2c_register.accuracy / 3;

        int i = i2c_register.accuracy;
        while(i--)
        {
                digitalWrite(POWER_PIN, HIGH);
                analogRead(EC_PIN);
                analogRaw = analogRead(EC_PIN);
                conductivityMedian.add(analogRaw);
                digitalWrite(POWER_PIN, LOW);
                tws_delay(10);
        }

        inputV = getVin();

        outputV = (inputV * conductivityMedian.getAverage(middleThird)) / 1024.0;
        conductivity = (outputV * (Resistor + pinResistance)) / (inputV - outputV) - pinResistance;

        float mS = 1000 / (conductivity * i2c_register.K);

        if (i2c_register.CONFIG.useTempCompensation)
        {
                if (i2c_register.tempConstant == 0xFF)
                {
                        mS  =  mS / (1 + i2c_register.tempCoef * (i2c_register.tempC - 25.0));
                        _salinity(i2c_register.tempC);
                }
                else
                {
                        mS  =  mS / (1 + i2c_register.tempCoef * (i2c_register.tempConstant - 25.0));
                        _salinity(i2c_register.tempConstant);
                }
        }

        if (i2c_register.CONFIG.useDualPoint)
        {
                float referenceRange = i2c_register.referenceHigh - i2c_register.referenceLow;
                float readingRange = i2c_register.readingHigh - i2c_register.readingLow;

                mS = (((mS - i2c_register.readingLow) * referenceRange) / readingRange) + i2c_register.referenceLow;
        }
        return mS;
}

void calibrateK()
{
        float ec;
        RunningMedian conductivityMedian = RunningMedian(i2c_register.accuracy);

        int middleThird = i2c_register.accuracy / 3;

        int i = i2c_register.accuracy;
        while(i--)
        {
                measureConductivity();
                conductivityMedian.add(conductivity);
                tws_delay(250);
        }

        ec  =  i2c_register.solutionEC * (1 + i2c_register.tempCoef * (i2c_register.tempC - 25.0));

        i2c_register.K = 1000 / ( conductivityMedian.getAverage(middleThird) * ec );
        EEPROM.put(EC_K_REGISTER, i2c_register.K);
}

void calibrateLow()
{
        i2c_register.referenceLow = i2c_register.solutionEC;
        measureConductivity();
        i2c_register.readingLow = i2c_register.mS;
        EEPROM.put(EC_CALIBRATE_REFLOW_REGISTER, i2c_register.referenceLow);
        EEPROM.put(EC_CALIBRATE_READLOW_REGISTER, i2c_register.readingLow);
}

void calibrateHigh()
{
        i2c_register.referenceHigh = i2c_register.solutionEC;
        measureConductivity();
        i2c_register.readingHigh = i2c_register.mS;
        EEPROM.put(EC_CALIBRATE_REFHIGH_REGISTER, i2c_register.referenceHigh);
        EEPROM.put(EC_CALIBRATE_READHIGH_REGISTER, i2c_register.readingHigh);
}

void _salinity(float temp)
{
        float r, ds, r2;

        float a0 = 0.008;
        float a1 = -0.1692;
        float a2 = 25.3851;
        float a3 = 14.0941;
        float a4 = -7.0261;
        float a5 = 2.7081;

        float b0 = 0.0005;
        float b1 = -0.0056;
        float b2 = -0.0066;
        float b3 = -0.0375;
        float b4 = 0.0636;
        float b5 = -0.0144;

        float c0 = 0.6766097;
        float c1 = 0.0200564;
        float c2 = 0.0001104259;
        float c3 = -0.00000069698;
        float c4 = 0.0000000010031;

        r = (i2c_register.mS * 1000) / 42900;
        r /= (c0 + temp * (c1 + temp * (c2 + temp * (c3 + temp * c4))));

        r2 = sqrtf(r);
        ds = b0 + r2 * (b1 + r2 * (b2 + r2 * (b3 + r2 * (b4 + r2 * b5))));
        ds = ds * ((temp - 15.0) / (1.0 + 0.0162 * (temp - 15.0)));

        // the formula to convert between EC and PSU is not valid for values
        // below 2 and above 42, or for temperatures below -2C or above 35C.
        i2c_register.salinityPSU = a0 + r2 * (a1 + r2 * (a2 + r2 * (a3 + r2 * (a4 + r2 * a5)))) + ds;

        if (i2c_register.salinityPSU < 2 || i2c_register.salinityPSU > 42)
        {
                i2c_register.salinityPSU = -1;
                return;
        }

        if (i2c_register.tempC < -2 || i2c_register.tempC > 35)
        {
                i2c_register.salinityPSU = -1;
                return;
        }
}
