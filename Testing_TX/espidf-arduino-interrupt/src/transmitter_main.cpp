#include <Arduino.h>
#include <uart_driver.h>

#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

#define MODULATED_BYTES_MAX_LEN 1028
#define SIGNAL_TO_DATA_RATIO    1
#define BITS_PER_BYTE           8
#define PULSE_DELAY_US          24 //24 /* Represents ~1/2 period for a single pulse. */ // 
#define LED_GPIO                14
#define LED_GPIO_HIGH           BIT0

uint8_t modulatedBytes[MODULATED_BYTES_MAX_LEN];

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
void modulateByte(uint8_t byte, uint8_t *modulatedByte, size_t &modulatedByteLen) {
  /* The number of periods that represent a single bit of data. */

  size_t index = 0U;
  uint8_t nextBit = 0U;

  /* TODO: make sure the modulatedByte buffer is big enough. */
  modulatedByteLen = SIGNAL_TO_DATA_RATIO * BITS_PER_BYTE;

  Serial.print("\n\nOriginal byte (decimal): ");
  Serial.print(byte);
  Serial.print("\nOriginal byte (binary): ");

  for(int bitIndex = 7; bitIndex >= 0; bitIndex--) {
    nextBit = (byte & 0x80) >> 7; // get left-most bit.
    Serial.print(nextBit); // debugging print

    /* Add multiple periods of the same pulse, to modulate the signal. */
    for(size_t pulseCount = 0; pulseCount < SIGNAL_TO_DATA_RATIO; pulseCount++) {
      modulatedByte[index] = nextBit;
      index++;
    }
    byte <<= 1; // left bit-shift once.
  }

  /* debugging print: print the final modulated character. */
  Serial.print("\nModulated byte: ");
  for(size_t index = 0U; index < SIGNAL_TO_DATA_RATIO * BITS_PER_BYTE; index++) {
    Serial.print(modulatedByte[index]);
  }
}

int modulateString(const char *bytes, size_t bytesLen, uint8_t *modulatedBytes, size_t modulatedBytesLen) {
  size_t pos = 0U;
  size_t modulatedLen = 0U;

  /* First add the length of the message to the buffer. */

  /* Modulate the byte representing the length of the message. */
  uint8_t modulatedMsgLen[BITS_PER_BYTE * SIGNAL_TO_DATA_RATIO + 1U];
  modulateByte(bytesLen, &modulatedMsgLen[0U], modulatedLen);

  /* Copy the modulated byte into the modulated string. */
  memcpy(&modulatedBytes[pos], &modulatedMsgLen[0U], modulatedLen);
  pos += modulatedLen;

  /* Modulate the message. */
  for(size_t index = 0U; index < bytesLen; index++) {
    uint8_t modulatedByte[50U];

    /* Modulate the next byte. */
    modulateByte(bytes[index], &modulatedByte[0U], modulatedLen);

    /* Copy the modulated byte into the modulated string. */
    memcpy(&modulatedBytes[pos], &modulatedByte[0U], modulatedLen);

    /* Update pos to point to the end of the buffer. */
    pos += modulatedLen;

    /* Make sure that the pos doesn't overflow the modulatedBytesLen. */
    if(pos >= modulatedBytesLen) {
      Serial.println("ERROR - modulated bytes' size greater than the max size of the buffer!");
      return -1;
    }
  }

  /* debugging print: print the final modulated string. */
  Serial.print("\n\nModulated string: ");
  for(size_t index = 0U; index < pos; index++) {
    Serial.print(modulatedBytes[index]);
  }

  return 0;
}

void pulseBinary1() {
  digitalWrite(LED_GPIO, HIGH);
  // REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) | LED_GPIO_HIGH);
  delayMicroseconds(PULSE_DELAY_US);
  digitalWrite(LED_GPIO, LOW);
  // REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) & (~LED_GPIO_HIGH));
  delayMicroseconds(PULSE_DELAY_US);
}

void pulseBinary0() {
  digitalWrite(LED_GPIO, LOW);
  // REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) & (~LED_GPIO_HIGH));
  delayMicroseconds(PULSE_DELAY_US);
  digitalWrite(LED_GPIO, LOW);
  // REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) & (~LED_GPIO_HIGH));
  delayMicroseconds(PULSE_DELAY_US);
}


/*!
 * @brief   Outputs the Start of Frame bit sequence
 * @details Outputs the following sequence: 11111111, modulated, to indicate the start of the message.
 */
void outputStartOfFrame() {
  /* Output a high. */
  for(int i = 0; i < BITS_PER_BYTE * SIGNAL_TO_DATA_RATIO; i++) {
      pulseBinary1();
  }
}

void setup(){
  pinMode(LED_GPIO, OUTPUT);
  Serial.begin(115200);

  /* Debugging - hardcode the string to send. */
  const char message[] = "hello";
  int ret = modulateString(&message[0U], strlen(message), &modulatedBytes[0U], MODULATED_BYTES_MAX_LEN);

  /* Delay one second before starting transmission. */
  delay(3000);

  /* Output the start-of-frame sequence. */
  outputStartOfFrame();
  

  // Serial.println("\nSent SOF");

  uint8_t val = 0;
  /* Output the signal. */
  for(int i = 0; i < (strlen(message) + 1) * BITS_PER_BYTE * SIGNAL_TO_DATA_RATIO; i++) {
    if(modulatedBytes[i] == 1) {
      /* Output a high. */
      pulseBinary1();
    } else {
      /* Output a low. */
      pulseBinary0();
    }
    val++;
  }
  //Serial.println(val);
  Serial.println("\nSent message.");

  // xTaskCreatePinnedToCore( // create a task on core 0 to make sure that still works with core 1 interrupts disabled later
  //     Task1code, /* Function to implement the task */
  //     "Task1", /* Name of the task */
  //     10000,  /* Stack size in words */
  //     NULL,  /* Task input parameter */
  //     0,  /* Priority of the task */
  //     &Task1,  /* Task handle. */
  //     0); /* Core where the task should run */

  // My_timer = timerBegin(0, 80, true);
  // timerAttachInterrupt(My_timer, &onTimer, true);
  // timerAlarmWrite(My_timer, 1000, true);
  // timerAlarmEnable(My_timer);
  // ESP_INTR_DISABLE(XT_TIMER_INTNUM); // disables the tick interrupt

  // // SENDING SECOND MESSAGE

  // /* Delay 3 second before starting transmission. */
  // delay(3000);

  // /* Output the start-of-frame sequence. */
  // outputStartOfFrame();

  // Serial.println("\nSent second SOF");

  // val = 0;
  // /* Output the signal. */
  // for(int i = 0; i < strlen(message) * BITS_PER_BYTE * SIGNAL_TO_DATA_RATIO; i++) {
  //   if(modulatedBytes[i] == 1) {
  //     /* Output a high. */
  //     pulseBinary1();
  //   } else {
  //     /* Output a low. */
  //     pulseBinary0();
  //   }
  //   val++;
  // }
  // Serial.print(val);
  // Serial.print("\nSent message.");

  /* TESTING photodiode - just outputting straight DC signal*/
  // digitalWrite(LED_GPIO, HIGH);
}

void loop(){
  /* Outputting a square wave. */
  // 45kHz --> delay 10 us
  // 10kHz --> delay 50 us
  // 1kHz --> delay 500 us
  // digitalWrite(LED_GPIO, HIGH);
  // delayMicroseconds(50);
  // digitalWrite(LED_GPIO, LOW);
  // delayMicroseconds(50);
  
  /* Debugging - hardcode the string to send. */
  // const char message[] = "hello";
  // int ret = modulateString(&message[0U], strlen(message), &modulatedBytes[0U], MODULATED_BYTES_MAX_LEN);

  // /* Delay one second before starting transmission. */
  // delay(5000);

  // /* Output the start-of-frame sequence. */
  // outputStartOfFrame();

  // Serial.println("\nSent SOF");

  // uint8_t val = 0;
  // /* Output the signal. */
  // for(int i = 0; i < strlen(message) * BITS_PER_BYTE * SIGNAL_TO_DATA_RATIO; i++) {
  //   if(modulatedBytes[i] == 1) {
  //     /* Output a high. */
  //     pulseBinary1();
  //   } else {
  //     /* Output a low. */
  //     pulseBinary0();
  //   }
  //   val++;
  // }
  // Serial.print(val);
  // Serial.print("\nSent message.");
}

