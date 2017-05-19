#include <Arduino.h>
#include <TinyWireS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/sleep.h>
#include <EEPROM.h>
#include <RunningMedian.h>

#define EC_SALINITY 0x3C                      /*!< EC Salinity probe I2C address */
#define EC_MEASURE_EC 80
#define EC_MEASURE_TEMP 40
#define EC_CALIBRATE_K 20
#define EC_CALIBRATE_LOW 10
#define EC_CALIBRATE_HIGH 8

#define EC_VERSION_REGISTER 0                 /*!< version register */
#define EC_MS_REGISTER 1                      /*!< mS register */
#define EC_TEMP_REGISTER 5                    /*!< temperature in C register */
#define EC_K_REGISTER 9                       /*!< cell constant register */
#define EC_SOLUTION_REGISTER 13               /*!< calibration solution register */
#define EC_TEMPCOEF_REGISTER 17               /*!< temperatue coefficient register */
#define EC_CALIBRATE_REFHIGH_REGISTER 21      /*!< reference low register */
#define EC_CALIBRATE_REFLOW_REGISTER 25       /*!< reference high register */
#define EC_CALIBRATE_READHIGH_REGISTER 29     /*!< reading low register */
#define EC_CALIBRATE_READLOW_REGISTER 33      /*!< reading high register */
#define EC_SALINITY_PSU 37                    /*!< Salinity register */
#define EC_TEMP_COMPENSATION_REGISTER 41      /*!< temperature compensation register */
#define EC_ACCURACY_REGISTER 42               /*!< accuracy register */
#define EC_CONFIG_REGISTER 43                 /*!< config register */
#define EC_TASK_REGISTER 44                   /*!< task register */

#define DS18_PIN 4
#define EC_PIN 3
#define POWER_PIN 1

#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC
#define ac_disable() ACSR |= _BV(ACD);        // disable analog comparator
#define ac_enable() ACSR &= _BV(ACD)         // enable analog comparator
#define timer1_disable() PRR |= _BV(PRTIM1)   // disable timer1_disable

OneWire oneWire(DS18_PIN);
DallasTemperature ds18(&oneWire);

struct config
{
        byte useDualPoint           : 1;
        byte useTempCompensation    : 1;
        byte buffer                 : 6;
};

struct rev1_register {
        byte version;       // 0
        float mS;           // 1-4
        float tempC;        // 5-8
        float K;            // 9-12
        float solutionEC;   // 13-16
        float tempCoef;     // 17-20
        float referenceHigh; // 21-24
        float referenceLow; // 25-28
        float readingHigh;  // 29-32
        float readingLow;   // 33-36
        float salinityPSU;  // 37-40
        byte tempConstant;  // 41
        byte accuracy;      // 42
        config CONFIG;      // 43
        byte TASK;          // 44
} i2c_register;

volatile byte reg_position;
const byte reg_size = sizeof(i2c_register);

float measureConductivity();
void calibrateK();
void calibrateLow();
void calibrateHigh();
void inline sleep();
void _salinity(float temp);

bool runEC = false;
bool runTemp = false;
bool runCalibrateK = false;
bool runCalibrateHigh = false;
bool runCalibrateLow = false;

static const int pinResistance = 23;
static const int Resistor = 500;
float conductivity;

void inline sleep()
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
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
        ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
        ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
        ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

        tws_delay(2);
        ADCSRA |= _BV(ADSC);
        while (bit_is_set(ADCSRA,ADSC));

        uint8_t low  = ADCL;
        uint8_t high = ADCH;

        long result = (high<<8) | low;

        result = 1125300L / result;
        return (double(result) - 0) * (6 - 0) / (6000 - 0) + 0;
}
