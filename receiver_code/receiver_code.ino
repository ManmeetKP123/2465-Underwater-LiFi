// char decodedBits[8];              // Store the decoded bits
// int bitIndex = 0;

// const int highFreqThreshold = 10;  // Threshold in microseconds for high frequency (adjust as needed)
// const int lowFreqThreshold = 1;  // Threshold in microseconds for low frequency (adjust as needed)

// void setup() {
//   pinMode(12, INPUT); 
//   Serial.begin(9600);  // Initialize serial communication
// }
// /**
// --|__|-|__|-
// **/

// void loop() {
//   float pulseWidth = pulseIn(12, HIGH, 2*1000000); // Measure HIGH duration
//   pulseWidth = (pulseWidth/1000000.0) * 2.0;
//   Serial.println(pulseWidth);
  
//   if ((pulseWidth <= 0.2) && (pulseWidth >= 0.09)) {
//     decodedBits[bitIndex++] = '1'; // Detected high frequency as '1'
//   } else if (pulseWidth >= 0.90) {
//     decodedBits[bitIndex++] = '0'; // Detected low frequency as '0'
//   }
  
//   // If we have read 5 bits, print the pattern
//   if (bitIndex == 5) {
//     decodedBits[bitIndex] = '\0'; // Null-terminate the string
//     Serial.print("Decoded Pattern: ");
//     Serial.println(decodedBits);
//     bitIndex = 0; // Reset for the next pattern
//   }
// }
#define PHOTO_PIN A0
#define BAUD_RATE 9600 // Bits per second
#define BIT_DELAY (1000 / BAUD_RATE) // Microseconds per bit
#define THRESHOLD 500 // Adjust based on photodiode sensitivity

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(PHOTO_PIN, INPUT);
}

byte readByte() {
  byte data = 0;

  // Wait for Start Bit (LOW)
  while (analogRead(PHOTO_PIN) > THRESHOLD) {
    // Do nothing until start bit is detected
  }
  delay(BIT_DELAY / 2); // Wait half a bit duration for alignment

  // Read Data Bits
  for (int i = 0; i < 8; i++) {
    delay(BIT_DELAY); // Wait for the next bit
    if (analogRead(PHOTO_PIN) > THRESHOLD) {
      data |= (1 << i); // Set the bit if HIGH
    }
  }

  // Wait for Stop Bit
  delay(BIT_DELAY);

  return data;
}

void loop() {
  byte receivedByte = readByte();
  Serial.print("Received Byte: \n");
  Serial.println(receivedByte, BIN);
  delay(900);
}