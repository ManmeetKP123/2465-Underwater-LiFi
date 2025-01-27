#include <Arduino.h>
#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

#define PHOTO_PIN A0
#define BAUD_RATE 9600 // Bits per second
#define BIT_DELAY (1000 / BAUD_RATE) // milliseconds per bit
#define THRESHOLD 500 // Adjust based on photodiode sensitivity
#define START_BIT_DURATION 8U
#define BUFFER_SIZE 256

hw_timer_t *My_timer = NULL;
TaskHandle_t Task1;
byte bitBuffer[BUFFER_SIZE]; // for decoding the incoming bits
int bufferIndex = 0;

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

byte readByte() {
  /**
   * Demodulation code for the actual photodiode based on thresholding
  */
  // byte data = 0;

  // // Wait for Start Bit (LOW)
  // while (analogRead(PHOTO_PIN) > THRESHOLD) {
  //   // Do nothing until start bit is detected
  // }
  // delay(BIT_DELAY / 2); // Wait half a bit duration for alignment

  // // Read Data Bits
  // for (int i = 0; i < 8; i++) {
  //   delay(BIT_DELAY); // Wait for the next bit
  //   if (analogRead(PHOTO_PIN) > THRESHOLD) {
  //     data |= (1 << i); // Set the bit if HIGH
  //   }
  // }

  // // Wait for Stop Bit
  // delay(BIT_DELAY);

  // return data;

  /**
   * Demodulation code for the serial line from tx esp
  */
 byte data = 0;

 // start of frame = 8 HIGH bits
 for (int i = 0; i < 8; i++){

  delay(BIT_DELAY / 2); // waiting for the bit to stabilize; nyquist theorem or whatever
  if (digitalRead(PHOTO_PIN) == HIGH){
    data |= (1 << i);
  }

  delay(BIT_DELAY / 2); // wait for the next bit; poll at least twice in the bit duration
 }

 // wait for the stop bit (HIGH as well??)
//  delay(BIT_DELAY);

 return data;
}

bool detectStartOfFrame() {
  int startBitDuration = 0;
  bool startDetected = false;

  while (digitalRead(PHOTO_PIN) == LOW) {
    // do nothing
  }

  while (digitalRead(PHOTO_PIN) == HIGH && startBitDuration < START_BIT_DURATION) {
    startBitDuration++;
    delay(BIT_DELAY);

    if (startBitDuration >= START_BIT_DURATION) {
      startDetected = true;
      break;
    }
  }

  // return whether or not we've seen the start of frame pattern
  return startDetected;
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

  Serial.begin(BAUD_RATE);
  pinMode(PHOTO_PIN, INPUT);
}

void loop() {
  if (detectStartOfFrame()) { // only proceed once we've detected the start
    byte receivedByte;
    while (1) {
      receivedByte = readByte();

      if (receivedByte != 0){ // checking for garbage values or null bytes
          Serial.print("Received Byte: \n");
          Serial.println(receivedByte, BIN);

          // decode the byte into its ascii from
          char decodedChar = (char) receivedByte;
          Serial.println("Decoded char: ");
          Serial.println(decodedChar);
      }

      delay(100); // NEED TO adjust this based on our own timing
    }
  }
  byte receivedByte = readByte();
  Serial.print("Received Byte: \n");
  Serial.println(receivedByte, BIN);
  delay(900);
}