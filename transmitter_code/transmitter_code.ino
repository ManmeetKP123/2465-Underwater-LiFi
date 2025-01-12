// int low_freq = 1;  // Frequency in Hz, you can change this value
// int high_freq = 10; 
// int low_period = 1000 / low_freq;   // Total period in milliseconds for one cycle
// int high_period = 1000 / high_freq;
// int halfPeriodLo = low_period / 2;     // Half period for high and low states
// int halfPeriodHi = high_period / 2;
// int count = 0;

// const int highFreqThreshold = 10;  // Threshold in microseconds for high frequency (adjust as needed)
// const int lowFreqThreshold = 1;  // Threshold in microseconds for low frequency (adjust as needed)

// void setup() {
//   pinMode(13, OUTPUT);  // initialize digital pin LED_BUILTIN as an output
//   Serial.begin(9600);  // Initialize serial communication
// }
// /**
// --|__|-|__|-
// **/

// void loop() {
//   delay(1000);
//   if (count < 4){
//     digitalWrite(13, HIGH);          // Set pin HIGH
//     delay(halfPeriodHi);               // Wait for half period
//     digitalWrite(13, LOW);
//     delay(halfPeriodHi);
//     digitalWrite(13, HIGH);          // Set pin HIGH
//     delay(halfPeriodHi);               // Wait for half period
//     digitalWrite(13, LOW);
//     delay(halfPeriodHi);
//     digitalWrite(13, HIGH);          // Set pin HIGH
//     delay(halfPeriodHi);               // Wait for half period
//     digitalWrite(13, LOW);
//     delay(halfPeriodHi);

//     digitalWrite(13, HIGH);          // Set pin HIGH
//     delay(halfPeriodLo);               // Wait for half period
//     digitalWrite(13, LOW);
//     delay(halfPeriodLo);
//     digitalWrite(13, HIGH);          // Set pin HIGH
//     delay(halfPeriodLo);               // Wait for half period
//     digitalWrite(13, LOW);
//     delay(halfPeriodLo);

//     count++;
//   }
// }

#define LED_PIN 9
#define BAUD_RATE 50000 // Bits per second
#define BIT_DELAY (1000 / BAUD_RATE) // Microseconds per bit

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void sendByte(byte data) {
  // Send Start Bit (LOW)
  digitalWrite(LED_PIN, HIGH);
  delay(BIT_DELAY); 
  digitalWrite(LED_PIN, LOW);
  delay(BIT_DELAY); 

  // //Send Data Bits (LSB first)
  // for (int i = 0; i < 8; i++) {
  //   digitalWrite(LED_PIN, (data >> i) & 1); // Send each bit
  //   delay(BIT_DELAY);
  // }

  // // Send Stop Bit (HIGH)
  // digitalWrite(LED_PIN, HIGH);
  // delay(BIT_DELAY);
}

void loop() {
  byte message = 0b10101010; // Example byte to send
  sendByte(message);
  // delay(1000);
}
