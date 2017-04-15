#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyWireS.h>
#include <EEPROM.h>

// i2c setup
#define EC_SALINITY 0x13
#define START_MEASUREMENT 0x00
#define SET_K 0x01

// ec setup pins
#define EC_PIN 3
#define POWER_PIN 1
//#define PIN_R 25         // pin resistance 22.5 @5 11.5@3

#define DS18_PIN 4
OneWire oneWire(DS18_PIN);
DallasTemperature ds18(&oneWire);

bool startEC = false;
bool startCalibrate = false;
bool startK = false;

void readEC();
void readTemperature();
void saveK();
float readK();
double doubleMap(double x, double in_min, double in_max, double out_min, double out_max);
long readVcc();
float getVin();

float getVin()
{
  long voltage = readVcc();
  return doubleMap(double(voltage), 0, 6000, 0, 6);
}

double doubleMap(double x, double in_min, double in_max, double out_min, double out_max)
{
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long readVcc() {
        // Read 1.1V reference against AVcc
        // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
        ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
        ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
        ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

        tws_delay(2); // Wait for Vref to settle
        ADCSRA |= _BV(ADSC); // Start conversion
        while (bit_is_set(ADCSRA,ADSC)) ;  // measuring

        uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
        uint8_t high = ADCH; // unlocks both

        long result = (high<<8) | low;

        result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
        return result; // Vcc in millivolts
}
