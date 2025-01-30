#include <Arduino.h>
#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

#define PHOTO_PIN 14
#define MODULATION_FREQ 1 // for VLC
#define BAUD_RATE 115200 // for serial communication
#define THRESHOLD 500 // Adjust based on photodiode sensitivity
#define START_BIT_DURATION 8U
#define BUFFER_SIZE 256

/**
 * VOLATILE DECLARATIONS
*/
volatile bool sofDetected = false;
volatile uint8_t risingPulseCount = 0;
volatile uint8_t fallingPulseCount = 0;
volatile uint8_t lastState = 0;

/**
 * GLOBAL VARIABLE
*/
hw_timer_t *My_timer = NULL;
TaskHandle_t Task1;
byte bitBuffer[BUFFER_SIZE]; // for decoding the incoming bits
int bufferIndex = 0;


void IRAM_ATTR onTimer(){
  digitalWrite(32, HIGH);
  digitalWrite(32, LOW);
}

void IRAM_ATTR start_measurement() {
  int currentState = digitalRead(PHOTO_PIN);
  if (currentState){ // currentState HI so rising edge detection
    risingPulseCount++;
  } else { // currentState LO so falling edge detection
    fallingPulseCount++;
  }

  if (risingPulseCount >=8 && fallingPulseCount >= 8) { // we've detected 16 successful transitions
    sofDetected = true;
    detachInterrupt(PHOTO_PIN); // detach until we need to start detecting SOF again
  }
}

void Task1code( void * parameter) {
  for(;;) {
    digitalWrite(32, HIGH);
    digitalWrite(32, LOW);
    vTaskDelay(1);
  }
}

void setup(){
  pinMode(32,OUTPUT);
  
  xTaskCreatePinnedToCore( // create a task on core 0 to make sure that still works with core 1 interrupts disabled later
      Task1code, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &Task1,  /* Task handle. */
      0); /* Core where the task should run */

  // My_timer = timerBegin(0, 80, true);
  // timerAttachInterrupt(My_timer, &onTimer, true);
  // timerAlarmWrite(My_timer, 1000, true);
  // timerAlarmEnable(My_timer);
  ESP_INTR_DISABLE(XT_TIMER_INTNUM); // disables the tick interrupt

  Serial.begin(BAUD_RATE);
  pinMode(PHOTO_PIN, INPUT);
  attachInterrupt(PHOTO_PIN, start_measurement, CHANGE);
}

void loop() {
  if (sofDetected) {
    Serial.println("Start of frame detected");
    sofDetected = false;
  }
}