//slave
int duty=0;
void setup() {  
  // The HC-06 defaults to 9600 according to the datasheet.
  Serial1.begin(9600);
  pinMode(13, OUTPUT);
}

void loop() {
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
