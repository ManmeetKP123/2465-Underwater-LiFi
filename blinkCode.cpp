int frequency = 5;  // Frequency in Hz, you can change this value
int period = 1000 / frequency;   // Total period in milliseconds for one cycle
int halfPeriod = period / 2;     // Half period for high and low states

void setup() {
  pinMode(13, OUTPUT);  // initialize digital pin LED_BUILTIN as an output
}

void loop() {
  // Calculate the delay based on the desired frequency
  

  // Generate the square wave
  digitalWrite(13, HIGH);          // Set pin HIGH
  delay(halfPeriod);               // Wait for half period
  digitalWrite(13, LOW);           // Set pin LOW
  delay(halfPeriod);               // Wait for half period
}