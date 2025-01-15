#include <Arduino.h>
#include <uart_driver.h>

#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

hw_timer_t *My_timer = NULL;
TaskHandle_t Task1;

void IRAM_ATTR onTimer(){
  digitalWrite(32, HIGH);
  digitalWrite(32, LOW);
}

void Task1code( void * parameter) {
  for(;;) {
    digitalWrite(32, HIGH);
    digitalWrite(32, LOW);
    vTaskDelay(1);
  }
}

/* Functions for encoding and sending data. */
void modulateByte(char byte, uint8_t *modulatedByte, size_t &modulatedByteLen) {
  /* The number of periods that represent a single bit of data. */
  size_t signalToDataRatio = 3U;

  size_t index = 0U;
  uint8_t nextBit = 0U;

  /* TODO: make sure the modulatedByte buffer is big enough. */
  modulatedByteLen = signalToDataRatio * 8U;

  printf("Original character: ");
  for(int bitIndex = 7; bitIndex >= 0; bitIndex--) {
    nextBit = byte & 0x80; // get left-most bit.
    printf("%d", nextBit); // debugging print

    /* Add multiple periods of the same pulse, to modulate the signal. */
    for(size_t pulseCount = 0; pulseCount < signalToDataRatio; pulseCount++) {
      modulatedByte[index] = nextBit;
    }

    byte <<= 1; // left bit-shift once.
  }

  /* debugging print: print the final modulated character. */
  printf("\n\nModulated character: ");
  for(size_t index = 0U; index < signalToDataRatio * 8U; index++) {
    printf("%d", nextBit);
  }
}

void modulateString(const char *bytes, size_t modulatedBytesLen) {
  uint8_t modulatedString[1048U];
  size_t pos = 0U;

  for(size_t index = 0U; index < modulatedBytesLen; index++) {
    uint8_t modulatedByte[24U];
    size_t modulatedLen = 0U;

    /* Modulate the next byte. */
    modulateByte(bytes[index], &modulatedByte[0U], modulatedLen);

    /* Copy the modulated byte into the modulated string. */
    memcpy(&modulatedString[pos], &modulatedByte[0U], modulatedLen);

    /* Update pos to point to the end of the buffer. */
    pos += modulatedLen;
  }

  /* debugging print: print the final modulated string. */
  printf("\n\nModulated string: ");
  for(size_t index = 0U; index < pos; index++) {
    printf("%d", modulatedString[index]);
  }
}

void setup(){
  pinMode(8,OUTPUT);
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

  /* Debugging - hardcode the string to send. */
  const char message[] = "hello";
  modulateString(&message[0U], strlen(message));
}

void loop(){
  digitalWrite(8,HIGH); // toggle a pin quickly - if everything is working there will be no gaps in the toggling signal
  digitalWrite(8,LOW);
}
