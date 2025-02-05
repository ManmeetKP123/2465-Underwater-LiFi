#include <Arduino.h>
#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

#define PHOTO_PIN 14
#define MODULATION_FREQ 1 // for VLC
#define BAUD_RATE 115200 // for serial communication
#define SAMPLING_TIME_MS 100
#define SAMPLES_PER_PERIOD 10
#define THRESHOLD 500 // Adjust based on photodiode sensitivity
#define START_BIT_DURATION 8U
#define BUFFER_SIZE 8

/**
 * VOLATILE DECLARATIONS
*/
volatile bool sofDetected = false;
volatile uint8_t risingPulseCount = 0;
volatile uint8_t fallingPulseCount = 0;
volatile uint8_t lastState = 0;
volatile bool byteReady =  false;
volatile uint8_t sampleBuffer[SAMPLES_PER_PERIOD];
volatile uint8_t bitCounter = 0;
volatile bool periodComplete = false;
volatile unsigned long lastSampleTime = 0; 

/**
 * GLOBAL VARIABLES
*/
uint8_t bitBuffer[BUFFER_SIZE]; // for decoding the incoming bits
int bufferIndex = 0;
bool lengthDetected = false;
hw_timer_t *timer = NULL;
bool detectedOnce = false;

/**
 * HELPER FUNCTIONS
*/
void IRAM_ATTR startOfFrameISR() {
  // read the GPIO pin directly via port manipulation, digitalRead might cause jitters
  // int currentState = (GPIO_IN_REG & (1 << PHOTO_PIN)) ? 1 : 0;
 
  int currentState = digitalRead(PHOTO_PIN);

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
  sampleBuffer[bitCounter] = digitalRead(PHOTO_PIN);
  bitCounter++;

  if (bitCounter >= SAMPLES_PER_PERIOD) {
    periodComplete = true;
    bitCounter = 0;
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

  timer = timerBegin(0, 80, true);
  // slowest the timer interrupt can be implemented is 1220.7Hz;
  timerAttachInterrupt(timer, &samplingISR, true);
  // count up from 0 to 99999 for 1s timer; sampling rate of 20kHz
  timerAlarmWrite(timer, SAMPLING_TIME_MS * 1000, true);
  Serial.println("AHHHHHHHHHH");
}

void loop() {
  if (sofDetected) {
    detachInterrupt(PHOTO_PIN); // detach until we need to start detecting SOF again
    Serial.println("Start of frame detected");
    sofDetected = false;
    
    timerAlarmEnable(timer);   // Start sampling

    bufferIndex = 0;
  } else if (periodComplete) {
    periodComplete = false;

    int onesCount = 0, zerosCount = 0;

    for (int i = 0; i < SAMPLES_PER_PERIOD; i++) {
      if (sampleBuffer[i]){
        onesCount++;
      } else {
        zerosCount++;
      }
    }

    uint8_t pulseBit = (onesCount > (zerosCount - 5)) ? 1 : 0;
    bitBuffer[bufferIndex] = pulseBit;
    bufferIndex++;


    if (bufferIndex >= 8U) {
      Serial.print("Collected Byte: ");
      for (int i = 0; i < 8U; i++){
        Serial.print(bitBuffer[i]);
      }
      Serial.println();
      bufferIndex = 0;
    }
  }

  // if (byteReady) {
  //   byteReady = false;

  //   Serial.print("Expected length of signal: ");
  //   Serial.println(currentByte, DEC);
  // }
}