//master

void setup() {
  // Open serial communications:
  Serial.begin(9600);
  Serial.println("Type AT commands!");
  
  // The HC-06 defaults to 9600 according to the datasheet.
  Serial1.begin(9600);
}

void loop() {
  // Read device output if available.
  if (Serial1.available()) {
    delay(100);
    while(Serial1.available()) { // While there is more to be read, keep reading.
      Serial.write(Serial1.read());
    }
  Serial.println("");    
  }
  
  // Read user input if available.
  if (Serial.available()){
    delay(100); // The delay is necessary to get this working!
    while(Serial.available()) {
      Serial1.write(Serial.read());
    }
  }
}
