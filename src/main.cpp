// Copyright (c) 2018 Justin Decker

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
   \brief EC Salinity firmware ver 1c

   ufire.co for links to documentation, examples, and libraries
   github.com/u-fire/ec-salinity-probe for feature requests, bug reports, and  questions
   questions@ufire.co to get in touch with someone

 */

#include <main.h>

double readADC(int channel)
{
  uint32_t total       = 0UL;
  uint16_t sampleCount = 4096;

  for (uint16_t i = 0; i < sampleCount; i++) {
    total += analogRead(channel);
  }

  total = total >> 6;
  double proportional = (total * 1.0) / (0b00000001 << 6);
  return proportional;
}

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
  if ((howMany < 1) || (howMany > 16))
  {
    return;
  }

  reg_position = TinyWireS.receive();
  howMany--;
  if (!howMany)
  {
    return;
  }

  while (howMany--)
  {
    *((uint8_t *)&i2c_register + reg_position) = TinyWireS.receive();

    if (reg_position == EC_TASK_REGISTER)
    {
      if (i2c_register.TASK == EC_MEASURE_EC) runEC = true;
      if (i2c_register.TASK == EC_MEASURE_TEMP) runTemp = true;
      if (i2c_register.TASK == EC_CALIBRATE_PROBE) runCalibrateProbe = true;
      if (i2c_register.TASK == EC_CALIBRATE_LOW) runCalibrateLow = true;
      if (i2c_register.TASK == EC_CALIBRATE_HIGH) runCalibrateHigh = true;
      if (i2c_register.TASK == EC_I2C) runI2CAddress = true;
      if (i2c_register.TASK == EC_DRY) runDry = true;
    }

    // save things when all 4 bytes of the float have been received
    if (reg_position == (EC_K_REGISTER + 3)) EEPROM.put(EC_K_REGISTER, i2c_register.K);
    if (reg_position == (EC_CALIBRATE_REFHIGH_REGISTER + 3)) EEPROM.put(EC_CALIBRATE_REFHIGH_REGISTER, i2c_register.referenceHigh);
    if (reg_position == (EC_CALIBRATE_REFLOW_REGISTER + 3)) EEPROM.put(EC_CALIBRATE_REFLOW_REGISTER, i2c_register.referenceLow);
    if (reg_position == (EC_CALIBRATE_READHIGH_REGISTER + 3)) EEPROM.put(EC_CALIBRATE_READHIGH_REGISTER, i2c_register.readingHigh);
    if (reg_position == (EC_CALIBRATE_READLOW_REGISTER + 3)) EEPROM.put(EC_CALIBRATE_READLOW_REGISTER, i2c_register.readingLow);
    if (reg_position == (EC_CALIBRATE_OFFSET_REGISTER + 3)) EEPROM.put(EC_CALIBRATE_OFFSET_REGISTER, i2c_register.calibrationOffset);
    if (reg_position == (EC_TEMP_COMPENSATION_REGISTER)) EEPROM.put(EC_TEMP_COMPENSATION_REGISTER, i2c_register.tempConstant);
    if (reg_position == (EC_CONFIG_REGISTER)) EEPROM.put(EC_CONFIG_REGISTER, i2c_register.CONFIG);

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
  pinMode(EC_PIN, INPUT);

  // set prescaler
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);

  EEPROM.get(EC_I2C_ADDRESS_REGISTER,        EC_SALINITY);
  EEPROM.get(EC_K_REGISTER,                  i2c_register.K);
  EEPROM.get(EC_CALIBRATE_REFHIGH_REGISTER,  i2c_register.referenceHigh);
  EEPROM.get(EC_CALIBRATE_REFLOW_REGISTER,   i2c_register.referenceLow);
  EEPROM.get(EC_CALIBRATE_READHIGH_REGISTER, i2c_register.readingHigh);
  EEPROM.get(EC_CALIBRATE_READLOW_REGISTER,  i2c_register.readingLow);
  EEPROM.get(EC_CALIBRATE_OFFSET_REGISTER,   i2c_register.calibrationOffset);
  EEPROM.get(EC_DRY_REGISTER,                i2c_register.dry);
  EEPROM.get(EC_TEMP_COMPENSATION_REGISTER,  i2c_register.tempConstant);
  EEPROM.get(EC_CONFIG_REGISTER,             i2c_register.CONFIG);

  i2c_register.version = VERSION;
  i2c_register.tempC   = -127;

  // if the EEPROM was blank, the i2c address hasn't been changed, make it the default address of 0x3c.
  if (EC_SALINITY == 0xff)
  {
    EC_SALINITY = EC_SALINITY_DEFAULT_ADDRESS;
  }

  // check for first time powerup and set default config
  if (i2c_register.CONFIG.buffer == 0b111111)
  {
    i2c_register.CONFIG.useTempCompensation = 0;
    i2c_register.tempConstant               = 0;
    i2c_register.CONFIG.useDualPoint        = 0;
    i2c_register.CONFIG.buffer              = 0;
    EEPROM.put(EC_CONFIG_REGISTER, i2c_register.CONFIG);
  }

  TinyWireS.begin(EC_SALINITY);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
}

void loop()
{
  low_power();
  TinyWireS_stop_check();
  if (runTemp)
  {
    ds18.requestTemperatures();
    i2c_register.tempC = ds18.getTempCByIndex(0);
    runTemp            = false;
  }

  TinyWireS_stop_check();
  if (runEC)
  {
    measureConductivity();
    runEC = false;
  }

  TinyWireS_stop_check();
  if (runCalibrateProbe)
  {
    calibrateProbe();
    runCalibrateProbe = false;
  }

  TinyWireS_stop_check();
  if (runCalibrateLow)
  {
    calibrateLow();
    runCalibrateLow = false;
  }

  TinyWireS_stop_check();
  if (runCalibrateHigh)
  {
    calibrateHigh();
    runCalibrateHigh = false;
  }

  TinyWireS_stop_check();
  if (runI2CAddress)
  {
    setI2CAddress();
    runI2CAddress = false;
  }

  TinyWireS_stop_check();
  if (runDry)
  {
    calibrateDry();
    runDry = false;
  }
}

float measureConductivity()
{
  float inputV, outputV, mS, resistance;
  uint32_t analogRaw;

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

  pinMode(SINK, OUTPUT);
  digitalWrite(SINK, LOW);

  analogRaw = readADC(EC_PIN);

  digitalWrite(POWER_PIN, LOW);
  pinMode(POWER_PIN, INPUT);
  pinMode(SINK,      INPUT);
  digitalWrite(SINK, LOW);

  inputV  = getVin();
  outputV = (inputV * analogRaw) / 1024.0;

  resistance = Resistor * (1 / ((inputV / outputV) - 1));
  mS         = ((100000 * i2c_register.K) / resistance);

  // Compensate for temperature if configured.
  if (i2c_register.CONFIG.useTempCompensation)
  {
    mS =  mS / (1.0 + i2c_register.tempCoef * (i2c_register.tempC - i2c_register.tempConstant));
  }

  // Use single point adjustment, ignoring if NaN
  if (i2c_register.calibrationOffset == i2c_register.calibrationOffset)
  {
    mS = mS - (mS * i2c_register.calibrationOffset);
  }

  if (i2c_register.CONFIG.useDualPoint)
  {
    // Use dual point calibration.
    float referenceRange = i2c_register.referenceHigh - i2c_register.referenceLow;
    float readingRange   = i2c_register.readingHigh - i2c_register.readingLow;

    mS = (((mS - i2c_register.readingLow) * referenceRange) / readingRange) + i2c_register.referenceLow;
  }

  // Check if the probe is dry/disconnected
  if (mS <= i2c_register.dry) mS = -1;

  i2c_register.mS = mS;
  _salinity(i2c_register.tempC);
  return mS;
}

void calibrateProbe()
{
  i2c_register.calibrationOffset = NAN;
  float mS = measureConductivity();

  i2c_register.calibrationOffset = (mS - i2c_register.solutionEC) / mS;
  EEPROM.put(EC_CALIBRATE_OFFSET_REGISTER, i2c_register.calibrationOffset);
}

void calibrateLow()
{
  i2c_register.referenceLow = i2c_register.solutionEC;
  measureConductivity();
  i2c_register.readingLow = i2c_register.mS;
  EEPROM.put(EC_CALIBRATE_REFLOW_REGISTER,  i2c_register.referenceLow);
  EEPROM.put(EC_CALIBRATE_READLOW_REGISTER, i2c_register.readingLow);
}

void calibrateHigh()
{
  i2c_register.referenceHigh = i2c_register.solutionEC;
  measureConductivity();
  i2c_register.readingHigh = i2c_register.mS;
  EEPROM.put(EC_CALIBRATE_REFHIGH_REGISTER,  i2c_register.referenceHigh);
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

  if ((temp == -127) || (temp = 0.0))
  {
    temp = 25;
  }
  r  = (i2c_register.mS * 1000) / 42900;
  r /= (c0 + temp * (c1 + temp * (c2 + temp * (c3 + temp * c4))));

  r2 = sqrtf(r);
  ds = b0 + r2 * (b1 + r2 * (b2 + r2 * (b3 + r2 * (b4 + r2 * b5))));
  ds = ds * ((temp - 15.0) / (1.0 + 0.0162 * (temp - 15.0)));

  i2c_register.salinityPSU = a0 + r2 * (a1 + r2 * (a2 + r2 * (a3 + r2 * (a4 + r2 * a5)))) + ds;

  if ((i2c_register.salinityPSU < 2) || (i2c_register.salinityPSU > 42))
  {
    i2c_register.salinityPSU = -1;
    return;
  }

  if ((temp < -2) || (temp > 35))
  {
    i2c_register.salinityPSU = -1;
    return;
  }
}

void setI2CAddress()
{
  // for convenience, the solution register is used to send the address
  EEPROM.put(EC_I2C_ADDRESS_REGISTER, i2c_register.solutionEC);
  TinyWireS.begin(i2c_register.solutionEC);
}

void calibrateDry()
{
  measureConductivity();
  i2c_register.dry = i2c_register.mS;
  EEPROM.put(EC_DRY_REGISTER, i2c_register.dry);
}
