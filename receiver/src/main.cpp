#include <Arduino.h>
#include "driver/dac_common.h"

#define BITS_PER_BYTE           8

// Define GPIOs here for explicitness, so they are not used for another purpose.
#define DAC_GPIO                DAC1  // GPIO 25

#define DAC_RESOLUTION          255   // 8-bit DAC
#define DAC_VREF                3.3   // VDD3P3_RTC

uint8_t convertAnalogToDigital(float analogValue) {
  return (uint8_t)(analogValue / DAC_VREF * DAC_RESOLUTION);
}

void setup(){
  Serial.begin(115200);
  dac_output_enable(DAC_CHANNEL_1); // corresponds to DAC1

  // Whatever the Vref you chose for the noise floor, overcompensate a bit on the high side to account for truncation.
  float diffAmpVoltageRef = 2.36;

  // Convert the analog voltage to digital value and output through the DAC.
  uint8_t diffAmpVoltageDigital = convertAnalogToDigital(diffAmpVoltageRef);
  dac_output_voltage(DAC_CHANNEL_1, diffAmpVoltageDigital);
}

void loop(){
  // TODO: Implement the loop function.
}
