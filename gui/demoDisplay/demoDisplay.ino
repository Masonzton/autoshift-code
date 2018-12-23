void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int count=0; count<3500; count+=10) {
    Serial.write(40);
    Serial.write(41);
    Serial.write(count/256);
    Serial.write(count%256);
    Serial.write((3500-count)/256);
    Serial.write((3500-count)%256);
    Serial.write('\r');
    Serial.write('\n');
    //Serial.println(count);
    delay(50);
  }
}
