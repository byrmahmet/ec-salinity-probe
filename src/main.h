#include <Arduino.h>
#include <TinyWireS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/sleep.h>
#include <EEPROM.h>

#define VERSION 0x1c
#define EC_SALINITY_DEFAULT_ADDRESS 0x3C

#define ACCURACY 6

uint8_t EC_SALINITY = 0x3C; /*!< EC Salinity probe I2C address */
#define EC_MEASURE_EC 80
#define EC_MEASURE_TEMP 40
#define EC_CALIBRATE_PROBE 20
#define EC_CALIBRATE_LOW 10
#define EC_CALIBRATE_HIGH 8
#define EC_I2C 1
#define EC_DRY 81

#define EC_VERSION_REGISTER 0             /*!< version register */
#define EC_MS_REGISTER 1                  /*!< mS register */
#define EC_TEMP_REGISTER 5                /*!< temperature in C register */
#define EC_K_REGISTER 9                   /*!< cell constant register */
#define EC_SOLUTION_REGISTER 13           /*!< calibration solution register */
#define EC_TEMPCOEF_REGISTER 17           /*!< temperatue coefficient register */
#define EC_CALIBRATE_REFHIGH_REGISTER 21  /*!< reference low register */
#define EC_CALIBRATE_REFLOW_REGISTER 25   /*!< reference high register */
#define EC_CALIBRATE_READHIGH_REGISTER 29 /*!< reading low register */
#define EC_CALIBRATE_READLOW_REGISTER 33  /*!< reading high register */
#define EC_CALIBRATE_OFFSET_REGISTER 37   /*!< caliration offset */
#define EC_SALINITY_PSU 41                /*!< Salinity register */
#define EC_DRY_REGISTER 45                /*!< Dry calibration register */
#define EC_TEMP_COMPENSATION_REGISTER 49  /*!< temperature compensation register */
#define EC_CONFIG_REGISTER 50             /*!< config register */
#define EC_TASK_REGISTER 51               /*!< task register */

#define EC_I2C_ADDRESS_REGISTER 200

struct config
{
  uint8_t useDualPoint        : 1; // 0
  uint8_t useTempCompensation : 1; // 1
  uint8_t buffer              : 6; // 2-7
};

struct rev1_register {
  uint8_t version;           // 0
  float   mS;                // 1-4
  float   tempC;             // 5-8
  float   K;                 // 9-12
  float   solutionEC;        // 13-16
  float   tempCoef;          // 17-20
  float   referenceHigh;     // 21-24
  float   referenceLow;      // 25-28
  float   readingHigh;       // 29-32
  float   readingLow;        // 33-36
  float   calibrationOffset; // 37-40
  float   salinityPSU;       // 41-44
  float   dry;               // 45-48
  uint8_t tempConstant;      // 49
  config  CONFIG;            // 50
  uint8_t TASK;              // 51
} i2c_register;

volatile uint8_t reg_position;
const uint8_t    reg_size = sizeof(i2c_register);

#define DS18_PIN 5
#define EC_PIN 3
#define POWER_PIN 1
#define SINK 4

#define adc_disable() (ADCSRA &= ~(1 << ADEN)) // disable ADC (before power-off)
#define adc_enable() (ADCSRA |=  (1 << ADEN))  // re-enable ADC
#define ac_disable() ACSR    |= _BV(ACD);      // disable analog comparator
#define ac_enable() ACSR     &= _BV(ACD)       // enable analog comparator
#define timer1_disable() PRR |= _BV(PRTIM1)    // disable timer1_disable

OneWire oneWire(DS18_PIN);
DallasTemperature ds18(&oneWire);

float measureConductivity();
void  calibrateProbe();
void  calibrateLow();
void  calibrateHigh();

void  sleep();
void  _salinity(float temp);
void  setI2CAddress();
void  calibrateDry();

bool runEC             = false;
bool runTemp           = false;
bool runCalibrateProbe = false;
bool runCalibrateHigh  = false;
bool runCalibrateLow   = false;
bool runI2CAddress     = false;
bool runDry            = false;

static const int pinResistance = 25;
static const int Resistor      = 500;

#ifndef cbi
# define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif // ifndef cbi
#ifndef sbi
# define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif // ifndef sbi

void inline low_power()
{
  set_sleep_mode(SLEEP_MODE_IDLE);
  adc_disable();
  ac_disable();
  sleep_enable();
  sleep_mode();
  sleep_disable();
  adc_enable();
  ac_enable();
}

float getVin()
{
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || \
  defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
  defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
  defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
  #else // if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) ||
  // defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif // if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) ||
  // defined(__AVR_ATmega2560__)

  tws_delay(2);
  ADCSRA |= _BV(ADSC);

  while (bit_is_set(ADCSRA, ADSC));

  uint8_t low  = ADCL;
  uint8_t high = ADCH;

  long result = (high << 8) | low;

  result = 1125300L / result;
  return (double(result) - 0) * (6 - 0) / (6000 - 0) + 0;
}
