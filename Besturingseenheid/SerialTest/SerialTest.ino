String inputData = "";  // Array for incoming data
byte buffer[5];

void setup() {
  Serial.begin(9600);
  
  // reserve 10 bytes for the inputString:
  inputData.reserve(10);
}

void loop() {
  // Write a temperature of 20 degrees every couple of seconds
  buffer[0] = 2;          // packet id 2
  buffer[1] = 128 + 20;   // 20 degrees
  Serial.write(buffer, 2);
  delay(2000);
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    inputData += (char)Serial.read();

    // Parse the incoming command here
  }
}
