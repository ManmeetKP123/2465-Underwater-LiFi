#include <Arduino.h>
#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"


#define PHOTO_PIN 14
#define BIT_PERIOD_US 50 // period of each transmitted bit pulse
#define BAUD_RATE 115200 // for serial communication
#define SAMPLE_RATE_HZ 200000 // was 50khz before // NOTE SEMI WORKS AT 150kHz??
#define SAMPLES_PER_PERIOD ((SAMPLE_RATE_HZ *  BIT_PERIOD_US) / 1000000) 
#define TIMER_TICK_US (1000000 / SAMPLE_RATE_HZ)
#define TIMER_PRESCALER 80
#define THRESHOLD 500 // Adjust based on photodiode sensitivity
#define BITS_PER_BYTE 8U
#define BUFFER_SIZE 25 // static buffer size to prevent any dynamic allocation
#define BIT_BUFFER_SIZE BITS_PER_BYTE*BUFFER_SIZE
#define SAMPLE_SIZE BIT_BUFFER_SIZE*SAMPLES_PER_PERIOD

/**
 * VOLATILE DECLARATIONS
*/
volatile uint8_t risingPulseCount = 0;
volatile uint8_t fallingPulseCount = 0;
volatile uint8_t lastState = 0;
volatile bool byteReady =  false;
volatile uint32_t sampleCounter = 0; // for 20000 data points for 20kHz sampling freq
volatile bool periodComplete = false;
volatile unsigned long lastSampleTime = 0;
volatile bool samplingComplete = false;
volatile bool processing = false;
volatile uint32_t onesCount = 0;
volatile uint32_t zerosCount = 0;

/**
 * GLOBAL VARIABLES
*/
uint8_t byteBuffer[BUFFER_SIZE]; // for decoding the incoming bits
uint8_t bitBuffer[BIT_BUFFER_SIZE];
uint8_t sampleBuffer[SAMPLE_SIZE];
uint8_t cleanedBuffer[SAMPLE_SIZE];
uint8_t bufferIndex = 0;
uint32_t bitbufferIndex = 0;
hw_timer_t *timer = NULL;
uint8_t currentByte = 0;
uint8_t bitCount = 0;
bool lengthDetected = false;
uint16_t lengthOfMessage = 0;
char fullMessage[BUFFER_SIZE];

/**
 * HELPER FUNCTIONS
*/

void IRAM_ATTR samplingISR() {
  uint8_t currentBit = (GPIO.in >> PHOTO_PIN) & 1;
  sampleBuffer[sampleCounter] = currentBit;
  sampleCounter++;
  

  if (sampleCounter >= SAMPLE_SIZE) {
    timerDetachInterrupt(timer);
    samplingComplete = true;
    //digitalWrite(8,0);
  }
}
void begin_samplingISR(){
  detachInterrupt(PHOTO_PIN);
  timerWrite(timer,0);
  timerAlarmEnable(timer);
}
void IRAM_ATTR startOfFrameISR() {
  int currentState = (GPIO.in >> PHOTO_PIN) & 1;

  if (currentState){ // currentState HI so rising edge detection
    risingPulseCount++;
  } else { // currentState LO so falling edge detection
    fallingPulseCount++;
  }

  if (risingPulseCount >=8 && fallingPulseCount >= 8) { // we've detected 16 successful transitions
    detachInterrupt(PHOTO_PIN);
    attachInterrupt(PHOTO_PIN, begin_samplingISR, RISING);
  }
}

void remove_inital_values() {
  bool past_SOF = false;
  int j=0;
  for(int i=1; i<sizeof(sampleBuffer)/sizeof(sampleBuffer[0]);i++){
    int previous_value = sampleBuffer[i-1];
    int current_value = sampleBuffer[i];
    if(current_value==1 && previous_value==0){
      past_SOF = true;
    }
    if (past_SOF){
      cleanedBuffer[j++]=sampleBuffer[i];
    }
  }
}

void thresholding_output(){
  const int oneHigh = 10;   // Ideal high duration for a 1 (50 µs)
  const int zeroHigh = 5;   // Ideal high duration for a 0 (25 µs)
  const int tolerance = 2;  // Allowable margin for jitter
  int i=0;
  int k=0;
  while (i < sizeof(cleanedBuffer)/sizeof(cleanedBuffer[0])) {
      if (cleanedBuffer[i] == 1) {
          int count = 0;
          // Count the continuous high samples
          while (i < sizeof(cleanedBuffer)/sizeof(cleanedBuffer[0]) && cleanedBuffer[i] == 1) {
              count++;
              i++;
          }
  
          if (count > oneHigh + tolerance) {
              int remainder = count % oneHigh;
              int numOnes = count/oneHigh;
              int numZeros = 0;
              if (remainder >= oneHigh - tolerance){
                numOnes++;
              }
              else if(remainder >= zeroHigh - tolerance){
                numZeros++;
              }
              for(int j=0; j<numOnes;j++){
                bitBuffer[k++] = 1;
              }
              for(int j=0;j< numZeros;j++){
                bitBuffer[k++] = 0;
              }
          } else {
              // For shorter pulses, decide based on the duration.
              if (count >= oneHigh - tolerance){
                bitBuffer[k++]=1;
              }
              else if (count >= zeroHigh - tolerance){
                bitBuffer[k++]=0;
              }
              // Otherwise, ignore as noise.
          }
      } else {
          // Skip low samples until the next high pulse.
          i++;
      }
  }

//   for(int i=0; i< sizeof(bitBuffer)/sizeof(bitBuffer[0]);i++){
//     Serial.println(bitBuffer[i]);
//  }
}

void output_transmission() {
  for(int j=0;j<BITS_PER_BYTE;j++){
    uint8_t bitValue =  bitBuffer[j];
    currentByte = (currentByte << 1) | bitValue;
    zerosCount=0;
    onesCount=0;
  }
  Serial.print("Length of Message: ");
  Serial.println(currentByte);

  lengthOfMessage = currentByte;
  currentByte = 0;

  for(int k=1;k<lengthOfMessage+1;k++){
    for(int j=k*BITS_PER_BYTE;j<(k+1)*BITS_PER_BYTE;j++){
      uint8_t bitValue =  bitBuffer[j];
      currentByte = (currentByte << 1) | bitValue;
      zerosCount=0;
      onesCount=0;
    }    
    fullMessage[bufferIndex] = static_cast<char>(currentByte);
    bufferIndex++;
    currentByte = 0;
  }

  Serial.print("Full Message Recieved: ");
  Serial.println(fullMessage);

}


/**
 * MAIN
*/
void setup(){
  pinMode(8,OUTPUT);
  ESP_INTR_DISABLE(XT_TIMER_INTNUM); // disables the tick interrupt
  Serial.begin(BAUD_RATE);
  pinMode(PHOTO_PIN, INPUT);
  //attachInterrupt(PHOTO_PIN, startOfFrameISR, CHANGE);
  attachInterrupt(PHOTO_PIN, begin_samplingISR, RISING);

  timer = timerBegin(0, TIMER_PRESCALER, true);
  timerAttachInterrupt(timer, &samplingISR, true);
  timerAlarmWrite(timer, TIMER_TICK_US, true);
  Serial.println("Receiving Initiated...");
}

void loop() {
  if (samplingComplete) {
    samplingComplete = false;
    remove_inital_values();
    thresholding_output();
    output_transmission();
    



    // finding length of message
    // for(int j=0;j<BITS_PER_BYTE;j++){
    //   for (int i=j*SAMPLES_PER_PERIOD;i<=(j+1)*SAMPLES_PER_PERIOD;i++){
    //     Serial.println(cleanedBuffer[i]);
    //     if (cleanedBuffer[i]){
    //       onesCount++;
    //     }
    //     else {
    //       zerosCount++;
    //     }
    //   }
    //   uint8_t bitValue =  (zerosCount > (onesCount - (SAMPLES_PER_PERIOD / 2))) ? 0 : 1;
    //   currentByte = (currentByte << 1) | bitValue;
    //   zerosCount=0;
    //   onesCount=0;
    // }
    // Serial.print("Length of Message: ");
    // Serial.println(currentByte);

    // lengthOfMessage = currentByte;
    // currentByte = 0;

    // for(int k=1;k<lengthOfMessage+1;k++){
    //   for(int j=k*BITS_PER_BYTE;j<(k+1)*BITS_PER_BYTE;j++){
    //     for (int i=j*SAMPLES_PER_PERIOD;i<=(j+1)*SAMPLES_PER_PERIOD;i++){
    //       Serial.println(cleanedBuffer[i]);
    //       if (cleanedBuffer[i]){
    //         onesCount++;
    //       }
    //       else {
    //         zerosCount++;
    //       }
    //     }
    //     uint8_t bitValue =  (zerosCount > (onesCount - (SAMPLES_PER_PERIOD / 2))) ? 0 : 1;
    //     currentByte = (currentByte << 1) | bitValue;
    //     zerosCount=0;
    //     onesCount=0;
    //   }
    //   //Serial.println(currentByte);
      
    //   fullMessage[bufferIndex] = static_cast<char>(currentByte);
    //   bufferIndex++;
    //   currentByte = 0;
    // }

    // Serial.print("Full Message Recieved: ");
    // Serial.println(fullMessage);






    // processing bits

    // samplingComplete = false;
    // // Adding but to buffer in time efficient way - no processing, just checking if buffer is full
    // uint8_t bitValue = (onesCount > (zerosCount - (SAMPLES_PER_PERIOD / 2))) ? 1 : 0;
    // // uint8_t bitValue = (onesCount > zerosCount) ? 1 : 0;
    // bitBuffer[bitbufferIndex] = bitValue;
    // bitbufferIndex++;
    // if (bitbufferIndex>= BIT_BUFFER_SIZE){
    //   timerDetachInterrupt(timer);
    //   samplingComplete = false;
    //   processing_buffer();
    // }
    // onesCount = 0;
    // zerosCount = 0;
    
  }
}

// Graveyard:


    // Computational code - just saving bit values, and if this doesnt work just take sample values
  //   currentByte = (currentByte << 1) | bitValue;
  //   bitCount++;

  //   if (bitCount >= BITS_PER_BYTE){
  //     byteBuffer[bufferIndex] = currentByte; 
  //     bufferIndex++;
  //     if (bufferIndex >= BUFFER_SIZE){
  //       timerAlarmDisable(timer);
  //       samplingComplete = false;
  //       processing_buffer();
  //     }
  //   }
  //   // reset for the next sampling interval
  //   onesCount = 0;
  //   zerosCount = 0;
  //   currentByte = 0;
  //   bitCount = 0;

 //Serial.println(samplingComplete);
  // if (sofDetected) {
  //   detachInterrupt(PHOTO_PIN); // detach until we need to start detecting SOF again
  //   // Serial.println("Start of frame detected");
  //   sofDetected = false;
    
  //   timerAlarmEnable(timer);   // Start sampling
  //   bufferIndex = 0;
  //   currentByte = 0;
  //   Serial.println("SOF detected");
  // };
   

    // if (bitCount >= BITS_PER_BYTE){
    //   if (!lengthDetected){
    //     lengthOfMessage = currentByte;
    //     // Serial.print("Expected number of bytes: ");
    //     // Serial.println(lengthOfMessage);
    //     lengthDetected = true;
    //   } 
    //   else {   
    //     byteBuffer[bufferIndex] = currentByte; 
    //     bufferIndex++;
    //     Serial.print("ASCII value of the incoming byte: ");
    //     Serial.println(static_cast<char>(currentByte));
    //     if (bufferIndex >= lengthOfMessage) {
    //         timerAlarmDisable(timer);
    //         bufferIndex = 0;
    //         currentByte = 0;
    //         bitCount = 0;
    //         samplingComplete = false;
    //         lengthDetected = false;
    //         for (int i = 0; i < lengthOfMessage; i++) {
    //         fullMessage[i] = static_cast<char>(byteBuffer[i]);
    //             }
    //         fullMessage[lengthOfMessage] = '\0'; // Null-terminate the string
    
    //         // Print the complete message
    //         Serial.print("Complete message received: ");
    //         Serial.println(fullMessage);
    //         attachInterrupt(PHOTO_PIN, startOfFrameISR, CHANGE);
    //       }
    //   }
    // //   Serial.print("ASCII value of the incoming byte: ");
    // //   Serial.println(static_cast<char>(currentByte));
    //   currentByte = 0;
    //   bitCount = 0;