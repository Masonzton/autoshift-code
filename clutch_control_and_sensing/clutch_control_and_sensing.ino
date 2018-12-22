#include <SoftwareSerial.h>
#include <Wire.h>

#define DIR 6
#define PWM 5
#define POS A6
#define CIN 290
#define COUT 120
#define NUM_PULSE 12
#define NUM_PULSE2 12
#define TX 8
#define RX 9
#define BT_POWER 10
const int changeRPM[] = {2200,2300,2400,2600};
const int changeUpRPM[] = {3500, 3500, 3500, 3500};

int data[8];
long prevTime=0;
long prevTime2=0;
long rpm=0;
long rpm2=0;
long prevRPM=0;
long prevRPM2=0;
int count = 0;
bool updateflag1 = false;
bool updateflag2 = false;
int currentGear = 1;
int failedShiftCounter=0;
int failedResponseCounter=0;
bool doNeutral = false;
int nwatchdog =0;
int wait_send_get_gear = 0;
bool isStopped=true;

SoftwareSerial btSerial = SoftwareSerial(RX, TX);


//functions
void interruptIRQ() {
  long currentTime=micros();
  if (prevTime<currentTime) {
    rpm=60*1000000/((currentTime-prevTime)*NUM_PULSE);
  }
  prevTime=currentTime;
  updateflag1 = true;
}

void NewinterruptIRQ() {
  long currentTime=micros();
  if (prevTime2<currentTime) {
    rpm2=60*1000000/((currentTime-prevTime2)*NUM_PULSE2);
    //Serial.println("start");
    //Serial.println(rpm2);
    data[count] = rpm2;
    count++;
    if (count>7){
      count = 0;
    }
    rpm2=average(data, sizeof(data));
    for (int i=0; i<8; i++) {
      //Serial.print(i);
      //Serial.print(": ");
      //Serial.println(data[i]);
    }
    //Serial.println(rpm2);
  }
  prevTime2=currentTime;
  updateflag2 = true;
}

int average( int a[], int len) {
  long sum = 0;
  len /= sizeof(int);
  for (int i = 0;i<len;i++){
    sum += a[i];
  }
  return sum/len;
}

int getSpeed() {
  
}

void clutchPOS(int pos, int power) {
  if (pos==CIN) {
    btSerial.println("I");
  }
  else {
    btSerial.println("O");
  }
  while(analogRead(POS)-pos < -10 || analogRead(POS)-pos >10) {
    //safety to not move if goes to far, make higher than 5?
    if (analogRead(POS)>CIN+10) {
      return;
    }
    if (analogRead(POS) <pos){
      digitalWrite(DIR,HIGH);
      analogWrite(PWM,power);
    }
    if(analogRead(POS)>pos){
      digitalWrite(DIR,LOW);
      analogWrite(PWM,power/2);
    }
  }
  digitalWrite(PWM,LOW);
}


void shiftDOWN() {
  //put clutch in 
  btSerial.println("D");
  clutchPOS(CIN, 255);
  //send serial message to shift down
  Serial.write('2');

  int watchdog = 0;
  while (!Serial.available() and watchdog<80) {
    delay(10);  
    watchdog++; 
  }  
  if (!Serial.available()){
    failedResponseCounter++;
    btSerial.println("F");
  }
  else{
    currentGear = getGear();
    btSerial.println(currentGear);
  }
  clutchPOS(COUT, 255);
  //wait for message that shift succesful
}

void shiftUP() {
  Serial.write('1');
  btSerial.println("U");
  int watchdog=0;
  while (!Serial.available() and watchdog <100) {
    delay(10);
    watchdog++;
  }
  if (!Serial.available()) {
    failedResponseCounter++;
    btSerial.println("F");
  }
  else {
    currentGear = getGear();
    btSerial.println(currentGear);
  }
}

void neutral() {
  clutchPOS(CIN, 255);
  Serial.write('5');
  int watchdog = 0;
  while (watchdog <100 and !Serial.available()){
    delay(10);
    watchdog++;
  }
  delay(500);
  currentGear=getGear();
  if (currentGear==0){
    clutchPOS(COUT, 255);
    btSerial.println("N");
  }
}


int par(char a) {
  switch (a) {
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    default:
      return 100;
  }
}

int getGear() {
  int a=0;
  while(Serial.available()) {
    a=Serial.read();
  }
  Serial.write('8');
  while(!Serial.available());
  a = Serial.read();
  return a;//par(a);
}

void launch() {
  if (currentGear==0) {
    clutchPOS(CIN, 255);
    shiftUP();
  }
  clutchPOS(COUT, 200);
  isStopped=false;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  pinMode(BT_POWER, OUTPUT);
  digitalWrite(BT_POWER, HIGH);
  btSerial.begin(9600);
  Serial.begin(57600);
  attachInterrupt(digitalPinToInterrupt(2), interruptIRQ, RISING);
  attachInterrupt(digitalPinToInterrupt(3), NewinterruptIRQ, RISING);
  pinMode(POS,INPUT);
  pinMode(PWM,OUTPUT);
  pinMode(DIR,OUTPUT);
  clutchPOS(COUT, 255);
  delay(1000);
  currentGear = getGear();
  clutchPOS(CIN,255);
  delay(500);
  clutchPOS(COUT, 255);
}

void loop() {
   
  //clutchPOS(position)  moves clutch to position
  if (currentGear>1) {
    if (rpm2 < changeRPM[currentGear-2])shiftDOWN();
  }
  if (currentGear<5 and currentGear!=0) {
    if (rpm2 > changeUpRPM[currentGear-1])shiftUP();
  }
  if (currentGear==1 and rpm2<2000){
    clutchPOS(CIN, 255);
    doNeutral=true;
    isStopped=true;
  }
  else {
    doNeutral=false;
    nwatchdog=0;
  }
  if (doNeutral and currentGear!=0){
    nwatchdog++;
    if (nwatchdog>80){
      neutral();
      nwatchdog = 0;
      doNeutral = false;
    }
  }
  if ((currentGear==1 or currentGear==0) and rpm2>2600 and isStopped) {
    launch();
  }
  if (failedResponseCounter > 2){
    delay(1000);
    failedResponseCounter = 0;
  }
  delay(50);
  wait_send_get_gear++;
  btSerial.println(rpm2);
  if (wait_send_get_gear >20) {
    currentGear = getGear();
    wait_send_get_gear = 0;
    btSerial.println(currentGear);
  } 
}

//void loop() {
//  delay(5000);
//  char data[10];
//  String dataString="rpm: " + String(rpm2);
//  dataString.toCharArray(data, 10);
//  Wire.beginTransmission(10);
//  Wire.write(data);
//  Wire.endTransmission();
//  currentGear=getGear();
//  dataString="gear: " + String(currentGear);
//  dataString.toCharArray(data, 10);
//  Wire.beginTransmission(10);
//  Wire.write(data);
//  Wire.endTransmission();
//  if (currentGear==1) {
//    Serial.write('5');
//  }
//  else if (currentGear==0) {
//    Serial.write('1');
//  }
//  else {
//    Serial.write('2');
//  }
//}
