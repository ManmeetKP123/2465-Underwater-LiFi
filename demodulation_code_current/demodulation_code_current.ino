const int demodPin = 2; // Pin to receive FSK signal
double lastPulse = 0;
double pulseWidth = 0;
bool receiving = false;
int *ptr = (int*)malloc(10 * sizeof(int));
int i = 0;

void setup() {
  Serial.begin(9600);
  pinMode(demodPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(demodPin), measurePulseWidth, RISING);
}

void loop() {
  if (receiving) {
    receiving = false;
    
    // Filter for frequency corresponding to '0'
    if (pulseWidth > 180 && pulseWidth < 300) {
      ptr[i] = 0;
      Serial.println("0"); // Detected '0' frequency
      i++;
    } 
    // Filter for frequency corresponding to '1'
    else if (pulseWidth > 46 && pulseWidth < 52) {
      ptr[i] = 1;
      Serial.println("1"); // Detected '1' frequency
      i++;
    } 
    // Handle unexpected long gaps
    else if (pulseWidth > 700) { 
      Serial.println("Long gap detected - Resetting.");
      i = 0;  // Reset index if long gap indicates new transmission
    } 
    else {
      Serial.println("Whoops");
    }
  }
}

void measurePulseWidth() {
  unsigned long now = micros();
  
  if (lastPulse == 0) {
    lastPulse = now;
    return;
  }
  
  pulseWidth = (now - lastPulse) / 1000.0; // Convert to milliseconds
  lastPulse = now;
  
  // Only set receiving to true if pulseWidth is within a valid range
  if (pulseWidth > 0 && pulseWidth < 1400) { 
    receiving = true;
  }
}

