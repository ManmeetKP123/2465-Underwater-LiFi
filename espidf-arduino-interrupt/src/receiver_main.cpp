#include <Arduino.h>
#include <uart_driver.h>

#define BAUD_RATE 9600
#define OVERSAMPLING_FACTOR 3
#define SAMPLE_RATE (BAUD_RATE * OVERSAMPLING_FACTOR) // in sampler per second
#define DEMOD_PIN 2U

volatile byteBuffer[OVERSAMPLING_FACTOR * 8]; // buffer for oversampling one byte at a time
volatile size_t sampleIndex = 0;
volatile bool preambleDetected = false;
volatile bool newDataReady = false;

// variables for the edge-triggered preamble 
volatile uint8_t preamblePattern = 0xAA; // hardcoded preamble pattern, change this #TODO
volatile uint8_t oversampledBits = 0; // collected bits during preamble detection
volatile bool waitingForPreamble = true; // flag to track whether we are still waiting for preamble

// function for storing the oversampled bits after the preamble has been detected
// we use edge-trigger for preamble detection and
// timer / oversampling for the rest of the sequence
void IRAM_ATTR timerIsrHandle() {
    // port manipulation instead of digitalRead for better computational efficiency
    bool currentState = (PIND & (1 << DEMOD_PIN)) != 0; // check the state of PD2 or pin 2

    // store the signal in a buffer
    byteBuffer[sampleIndex++] = currentState;

    // once the buffer is full, we have one byte sampled 
    if (sampleIndex >= OVERSAMPLING_FACTOR * 8) {
            newDataReady = true;
            sampleIndex = 0;
        }
}

// edge-triggering for the preamble
void IRAM_ATTR gpioIsrHandle() {
    // read the pin 
    bool currentState = (PIND & (1 << DEMOD_PIN)) != 0;

    // shift the bits we've collected so far 
    oversampledBits = (oversampledBits << 1) | currentState;

    // check for the preamble pattern once we have a full byte 
    if (++sampleIndex >= 8) {
        if (oversampledBits == preamblePattern) {
            preambleDetected = true;
            waitingForPreamble = false;

            // debug print, to let us know that we've detected the preamble and now moving to timer stuff
            Serial.println("Preamble detected. Starting oversampling...");

            // now attach the interrupt to the timer function for oversampling 
            Timer1.initialize(1000000 / SAMPLE_RATE) // timer1 function by default takes values in microseconds
            Timer1.attachInterrupt(timerIsrHandle);

            detachInterrupt(digitalPinToInterrupt(DEMOD_PIN));
        }
        sampleIndex = 0;
    }
}

uint8_t oversampleBit(uint8_t *buffer, size_t bitIndex) {
    uint8_t onesCount = 0;
    
    // oversample the current bit by the oversampling_factor times
    for (size_t i = 0; i < OVERSAMPLING_FACTOR: i++) {
        if (buffer[bitIndex * OVERSAMPLING_FACTOR + i] == HIGH) {
            onesCount++;
        }
    }

    return (onesCount >= OVERSAMPLING_FACTOR / 2) ? 1 : 0;
}

// function for detecting the preamble 
bool detectPreamble(uint8_t *buffer) {
    uint8_t oversampledVals = 0;

    // loop through the 8 bits of the byte and perform oversampling 
    for (size_t i = 0; i < 8; i++){
        uint8_t bit = oversampleBit(buffer, i);

        // left shift and bit mask to store the current bit in our pattern
        oversampledVals = (oversampledVals << 1) | bit;
    }

    // compare the collected bits to our preamble pattern
    return oversampledVals == preamblePattern;
}

// demodulate the collected byte 
uint8_t demodulateByte(uint8_t *buffer) {
    uint8_t decodedByte = 0; 

    // loop through the bits in our byteBuffer 
    for (size_t i = 0; i < 8; i++){
        uint8_t bit = oversampleBit(buffer, i);
        decodedByte = (decodedByte << 1) | bit;
    }

    return decodedByte;
}

// setting up the interrupt function 
void setup() {
    // starting serial communication for debugging
    Serial.begin(115200);

    pinMode(DEMOD_PIN, INPUT);
    // equivalent code is
    // DDRD = B00000000; (0-7 digital pins are set to input)

    // trigger on both rising and falling edges
    attachInterrupt(digitalPinToInterrupt(DEMOD_PIN), gpioIsrHandle, CHANGE);
}

void loop() { 
    // only process the data when the flag is set
        if (newDataReady) {
            newDataReady = false;

            // process the data 
            if (preamnleDetected) {
                // already have the preamble, demodulate the byte
                uint8_t demodulatedByte = demodulateByte(signalBuffer);
                Serial.print("Decoded Byte: ");
                Serial.println(demodulatedByte, DEC); // print the decoded byte as a decimal

                // reset the buffer
        }
    }
}