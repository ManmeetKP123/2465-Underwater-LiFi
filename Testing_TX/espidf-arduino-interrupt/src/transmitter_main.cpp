// 

#include <Arduino.h>
#include <uart_driver.h>

#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

#define MODULATED_BYTES_MAX_LEN 1028
#define MESSAGE_BYTES_MAX       64
#define SIGNAL_TO_DATA_RATIO    1
#define BITS_PER_BYTE           8
#define PULSE_DELAY_US          25 // 50000 /* Represents ~1/2 period for a single pulse. */

#define LED_GPIO                14
#define LED_GPIO_HIGH           BIT0

/*! 
 * @brief The FSM state of the transmitter.
 */
enum FsmState {
  STARTUP,  // initial state
  IDLE,     // waiting to receive the next messagge to transmit
  TRANSMIT, // transmitting the message
  FAULT     // error state
};

/* Variable to store the current FSM state. */
FsmState fsmState;

// Add 1 extra space in the buffers for the null terminator.
uint8_t messageBytes[MESSAGE_BYTES_MAX + 1U];
uint8_t modulatedBytes[MODULATED_BYTES_MAX_LEN + 1U];

hw_timer_t *My_timer = NULL;
TaskHandle_t Task1;

/* Functions for encoding and sending data. */
void modulateByte(uint8_t byte, uint8_t *modulatedByte, size_t &modulatedByteLen) {
  /* The number of periods that represent a single bit of data. */

  size_t index = 0U;
  uint8_t nextBit = 0U;

  /* TODO: make sure the modulatedByte buffer is big enough. */
  modulatedByteLen = SIGNAL_TO_DATA_RATIO * BITS_PER_BYTE;

  // Serial.print("\n\nOriginal byte (decimal): ");
  // Serial.print(byte);
  // Serial.print("\nOriginal byte (binary): ");

  for(int bitIndex = 7; bitIndex >= 0; bitIndex--) {
    nextBit = (byte & 0x80) >> 7; // get left-most bit.
    // Serial.print(nextBit); // debugging print

    /* Add multiple periods of the same pulse, to modulate the signal. */
    for(size_t pulseCount = 0; pulseCount < SIGNAL_TO_DATA_RATIO; pulseCount++) {
      modulatedByte[index] = nextBit;
      index++;
    }
    byte <<= 1; // left bit-shift once.
  }

  /* debugging print: print the final modulated character. */
  // Serial.print("\nModulated byte: ");
  // for(size_t index = 0U; index < SIGNAL_TO_DATA_RATIO * BITS_PER_BYTE; index++) {
  //   Serial.print(modulatedByte[index]);
  // }
}

int modulateString(const uint8_t *bytes, size_t bytesLen, uint8_t *modulatedBytes, size_t modulatedBytesLen) {
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
    if(pos >= modulatedBytesLen-8) {
      Serial.println("ERROR - message size greater than the max size of the buffer! Will only send allowed amount");
      return -1;
    }
  }

  /* debugging print: print the final modulated string. */
  // Serial.print("\n\nModulated string: ");
  // for(size_t index = 0U; index < pos; index++) {
  //   Serial.print(modulatedBytes[index]);
  // }

  // Serial.println("\n\n");

  return 0;
}

void pulseBinary1() {
  digitalWrite(LED_GPIO, LOW);
  delayMicroseconds(PULSE_DELAY_US);
  digitalWrite(LED_GPIO, HIGH);
  delayMicroseconds(PULSE_DELAY_US);
}

void pulseBinary0() {
  digitalWrite(LED_GPIO, LOW);
  delayMicroseconds(PULSE_DELAY_US);
  digitalWrite(LED_GPIO, LOW);
  delayMicroseconds(PULSE_DELAY_US);
}

/*!
 * @brief   Outputs the Start of Frame bit sequence
 * @details Outputs the following sequence: 11111111, modulated, to indicate the start of the message.
 */
void outputStartOfFrame() {
  /* Output a high. */
  digitalWrite(LED_GPIO,HIGH);
  delayMicroseconds(PULSE_DELAY_US);
  for(int i = 0; i < BITS_PER_BYTE-1; i++) {
      digitalWrite(LED_GPIO,LOW);
      delayMicroseconds(PULSE_DELAY_US*2);
  }
  digitalWrite(LED_GPIO,HIGH);
  delayMicroseconds(PULSE_DELAY_US);
}

void fsmHandleIdleState() {
  Serial.println("Waiting for user input.");
  uint8_t numBytesReceived = 0U;

  while(Serial.available() == 0) {
    // wait.
  }

  while(Serial.available() > 0) {
    uint8_t receivedByte = Serial.read();
    messageBytes[numBytesReceived] = receivedByte;
    numBytesReceived++;
  }

  if(numBytesReceived > 0) {
    // Add null terminator.
    messageBytes[numBytesReceived] = '\0';

    // Debug prints.
    // Serial.println("Message:");
    // for(uint8_t index = 0; index < numBytesReceived; index++) {
    //   Serial.println(messageBytes[index]);
    // }

    Serial.println("Message received.");

    // Move to the transmit state.
    fsmState = TRANSMIT;
  }  
}

void fsmHandleTransmitState() {
  uint8_t messageLen = strlen((const char *)messageBytes);
  modulateString(messageBytes, messageLen, modulatedBytes, MODULATED_BYTES_MAX_LEN);
  
 /* Output the start-of-frame sequence. */
  outputStartOfFrame();
    
  /* Output the signal. */
  for(int i = 0; i < (messageLen + 1U) * BITS_PER_BYTE * SIGNAL_TO_DATA_RATIO; i++) {
    if(modulatedBytes[i] == 1) {
      /* Output a high. */
      pulseBinary1();
    } else {
      /* Output a low. */
      pulseBinary0();
    }
  }

  // Output final low to indicate the end of the message.
  pulseBinary0();

  Serial.println("Message transmitted.");
  digitalWrite(LED_GPIO, LOW);

  // Move back to IDLE state.
  fsmState = IDLE;
  
  return;
  
  /* Outputting a square wave. */
  // 21kHz --> delay 24 us
  // 45kHz --> delay 10 us
  // 10kHz --> delay 50 us
  // 1kHz --> delay 500 us
  // digitalWrite(LED_GPIO, HIGH);
  // delayMicroseconds(24);
  // digitalWrite(LED_GPIO, LOW);
  // delayMicroseconds(24);
}

void setup(){
  fsmState = STARTUP;

  pinMode(LED_GPIO, OUTPUT);
  Serial.begin(115200);

  // Move into the IDLE state.
  fsmState = IDLE;

}

void loop(){
  // switch(fsmState) {
  //   case STARTUP:
  //     Serial.println("Should not be in the STARTUP state. Move to FAULT");
  //     fsmState = FAULT;
  //     break;
  //   case IDLE:
  //     fsmHandleIdleState();
  //     delay(1000);
  //     break;
  //   case TRANSMIT:
  //     fsmHandleTransmitState();
  //     delay(500);
  //     break;
  //   case FAULT:
  //     Serial.println("FAULT state reached.");
  //     delay(5000);
  //     break;
  // }
  digitalWrite(LED_GPIO, HIGH);
  delay(1000);
  digitalWrite(LED_GPIO, LOW);
  delay(1000);
}

