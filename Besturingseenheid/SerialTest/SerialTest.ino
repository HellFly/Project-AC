String inputBuffer = "";  // buffer for incoming data
byte sendBuffer[5];

void setup() {
  Serial.begin(9600);

  // reserve 16 bytes for the input data:
  inputBuffer.reserve(16);
}

void loop() {
  // Write a temperature of 20 degrees every couple of seconds
  sendBuffer[0] = 2;          // packet id 2 for temperature
  sendBuffer[1] = 128 + 20;   // 20 degrees
  Serial.write(sendBuffer, 2);
  delay(2000);
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    inputBuffer += (char)Serial.read();
  }

  parseData();
}

void parseData() {
  byte data[inputBuffer.length()];
  inputBuffer.getBytes(data, inputBuffer.length());

  byte c = data[0];
  byte p1 = data[1];
  byte p2 = data[2];
  byte p3 = data[3];

  if (c == 0) { // Please send all data
    if (p1 == 1) { // Send from temperature unit
      sendBuffer[0] = 2;          // packet id 2 for temperature
      sendBuffer[1] = 128 + 20;   // 20 degrees
      Serial.write(sendBuffer, 2);
      inputBuffer = "";
    }
  }
  else if (c == 20) { // Max opening distance shutters
    if (p1 == 1) { // For temperature unit
      int value = p2*256 + p3; // Max opening distance in cm
      inputBuffer = "";
    }
  }
  else { // no valid value for the packet id, so clear the data
    inputBuffer = "";
  }
}

void send_light(int light) {
 byte val1;
  byte val2;

  if (light < 0) {
    val1 = 0;
    val2 = 0;
  }
  else if (light > 255*256+255) { // if light value > max value able to send
    val1 = 255;
    val2 = 255;
  }
  else {
    val1 = (byte)light / 256;
    val2 = (byte)light % 256;
  }


  byte buffer[3];
  buffer[0] = 1;
  buffer[1] = val1;
  buffer[2] = val2;
  Serial.write(buffer, 3);
}

void send_temperature(int temp) {
	temp += 128; // offset of 128 degrees
	byte val;

	if (temp < 0) {
		val = 0;
	}
	else if (temp > 255) {
		val = 255;
	}
	else {
		val = (byte)temp;
	}

	byte buffer[2];
	buffer[0] = 2;
	buffer[1] = val;
	Serial.write(buffer, 2);
}

void send_shutter_lightunit(bool is_open) {
  byte buffer[3];
  buffer[0] = 3;
  buffer[1] = 0;
  buffer[2] = (byte)is_open;
  Serial.write(buffer, 3);
}

void send_shutter_tempunit(bool is_open) {
  byte buffer[3];
  buffer[0] = 3;
  buffer[1] = 1;
  buffer[2] = (byte)is_open;
  Serial.write(buffer, 3);
}

