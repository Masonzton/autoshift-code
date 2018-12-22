
String command = ""; // Stores response of the HC-06 Bluetooth device
int count=0;
int duty=0;
void setup() {
  // Open serial communications:
  Serial.begin(9600);
  Serial.println("Type AT commands!");
  pinMode(13, OUTPUT);
  
  // The HC-06 defaults to 9600 according to the datasheet.
  Serial1.begin(9600);
}

void loop() {
  /*String data=String(count);
  Serial1.println(data);
  count++;
  delay(500);*/
  while(!Serial1.available()) {
    delay(10);
    analogWrite(13, duty);
  }
  duty=Serial1.parseInt();
  analogWrite(13, duty);
  delay(100);
  while(Serial1.available()) {
    int throwaway=Serial1.read();
  }
 }
