#include <Arduino.h>
#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

#define PHOTO_PIN 14
#define BIT_PERIOD_MS 50 // period of each transmitted bit pulse
#define BAUD_RATE 115200 // for serial communication
#define SAMPLE_RATE_HZ 50000
#define SAMPLES_PER_PERIOD ((SAMPLE_RATE_HZ *  BIT_PERIOD_MS) / 1000)
#define TIMER_TICK_US (1000000 / SAMPLE_RATE_HZ)
#define TIMER_PRESCALER 80
#define THRESHOLD 500 // Adjust based on photodiode sensitivity
#define BITS_PER_BYTE 8U
#define BUFFER_SIZE 255 // static buffer size to prevent any dynamic allocation

/**
 * VOLATILE DECLARATIONS
*/
volatile bool sofDetected = false;
volatile uint8_t risingPulseCount = 0;
volatile uint8_t fallingPulseCount = 0;
volatile uint8_t lastState = 0;
volatile bool byteReady =  false;
volatile uint8_t sampleBuffer[SAMPLES_PER_PERIOD];
volatile uint32_t sampleCounter = 0; // for 20000 data points for 20kHz sampling freq
volatile bool periodComplete = false;
volatile unsigned long lastSampleTime = 0;
volatile bool samplingComplete = false;
volatile uint32_t onesCount = 0;
volatile uint32_t zerosCount = 0;

/**
 * GLOBAL VARIABLES
*/
uint8_t byteBuffer[BUFFER_SIZE]; // for decoding the incoming bits
uint8_t bufferIndex = 0;
hw_timer_t *timer = NULL;
uint8_t currentByte = 0;
uint8_t bitCount = 0;
bool lengthDetected = false;
uint16_t lengthOfMessage = 0;

/**
 * HELPER FUNCTIONS
*/
void IRAM_ATTR startOfFrameISR() {
  // read the GPIO pin directly via port manipulation, digitalRead might cause jitters
  // int currentState = (GPIO_IN_REG & (1 << PHOTO_PIN)) ? 1 : 0;
 
  int currentState = (GPIO.in >> PHOTO_PIN) & 1;

  if (currentState){ // currentState HI so rising edge detection
    risingPulseCount++;
  } else { // currentState LO so falling edge detection
    fallingPulseCount++;
  }

  if (risingPulseCount >=8 && fallingPulseCount >= 8) { // we've detected 16 successful transitions
    sofDetected = true;
  }
}

void IRAM_ATTR samplingISR() {
  uint8_t currentBit = (GPIO.in >> PHOTO_PIN) & 1;

  if (currentBit) {
    onesCount++;
  } else {
    zerosCount++;
  }
  sampleCounter++;

  if (sampleCounter >= SAMPLES_PER_PERIOD) {
    samplingComplete = true;
    sampleCounter = 0;
  }
}

/**
 * MAIN
*/
void setup(){
  pinMode(32,OUTPUT);
  ESP_INTR_DISABLE(XT_TIMER_INTNUM); // disables the tick interrupt

  Serial.begin(BAUD_RATE);
  pinMode(PHOTO_PIN, INPUT);
  attachInterrupt(PHOTO_PIN, startOfFrameISR, CHANGE);

  timer = timerBegin(0, TIMER_PRESCALER, true);
  // slowest the timer interrupt can be implemented is 1220.7Hz;
  timerAttachInterrupt(timer, &samplingISR, true);
  // count up from 0 to 9999 for 1s timer; sampling rate of 20kHz
  timerAlarmWrite(timer, TIMER_TICK_US, true);
  Serial.println("AHHHHHHHHHH");
}

void loop() {
  if (sofDetected) {
    detachInterrupt(PHOTO_PIN); // detach until we need to start detecting SOF again
    // Serial.println("Start of frame detected");
    sofDetected = false;
    
    timerAlarmEnable(timer);   // Start sampling
    bufferIndex = 0;
    currentByte = 0;
  } else if (samplingComplete) {
    samplingComplete = false;

    uint8_t bitValue = (onesCount > (zerosCount - (SAMPLES_PER_PERIOD / 2))) ? 1 : 0;
    currentByte = (currentByte << 1) | bitValue;

    bitCount++;

    // reset for the next sampling interval
    onesCount = 0;
    zerosCount = 0;

    if (bitCount >= BITS_PER_BYTE){
      if (!lengthDetected){
        lengthOfMessage = currentByte;
        Serial.print("Expected number of bytes: ");
        Serial.println(lengthOfMessage);
        lengthDetected = true;
      } else {   
          byteBuffer[bufferIndex] = currentByte; 
          bufferIndex++;
          if (bufferIndex >= lengthOfMessage) {
            timerAlarmDisable(timer);
            bufferIndex = 0;
            attachInterrupt(PHOTO_PIN, startOfFrameISR, CHANGE);
          }
      }
      Serial.print("ASCII value of the incoming byte: ");
      Serial.println(static_cast<char>(currentByte));
      currentByte = 0;
      bitCount = 0;
    }
  }
}