#define DIR 6
#define PWM 5
#define POS A6
#define CIN 250
#define COUT 100
#define NUM_PULSE 12
#define NUM_PULSE2 8
const int changeRPM[] = {2200,2300,2400,2600};

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
    data[count] = rpm2;
    count++;
    if (count>7){
      count = 0;
    }
    rpm2=average(data);
  }
  prevTime2=currentTime;
  updateflag2 = true;
}
int average( int a[]) {
  long sum = 0;
  int len = sizeof(a)/sizeof(int);
  for (int i = 0;i<len;i++){
    sum += a[i];
  }
  return sum/len;
}

void clutchPOS(int pos) {
  while(analogRead(POS)-pos < -10 || analogRead(POS)-pos >10) {
    //safety to not move if goes to far, make higher than 5?
    if (analogRead(POS)>CIN+10) {
      return;
    }
    if (analogRead(POS) <pos){
      digitalWrite(DIR,HIGH);
      analogWrite(PWM,255);
    }
    if(analogRead(POS)>pos){
      digitalWrite(DIR,LOW);
      analogWrite(PWM,100);
    }
  }
  digitalWrite(PWM,LOW);
}


void shiftDOWN() {
  //put clutch in 
  clutchPOS(CIN);
  //send serial message to shift down
  Serial.write('2');
  //keep sending message every 10 milleseconds until it has any response back
  //message can be anything
  int watchdog = 0;
  while (!Serial.available() and watchdog<80) {
    delay(10);  
    watchdog++; 
  }  
  if (!Serial.available()){
    failedResponseCounter++;
  }
  else{
    //char a = Serial.read();
    currentGear = Serial.read();//par(a);
  }
  clutchPOS(COUT);
  //wait for message that shift succesful
}

void shiftUP() {
  Serial.write('1');
  int watchdog=0;
  while (!Serial.available() and watchdog <100) {
    delay(10);
    watchdog++;
  }
  if (!Serial.available()) {
    failedResponseCounter++;
  }
  else {
    //char a = Serial.read();
    currentGear = Serial.read();//par(a); /////////////////
  }
}

void neutral() {
  clutchPOS(CIN);
  Serial.write('5');
  int watchdog = 0;
  while (watchdog <100 and !Serial.available()){
    delay(10);
    watchdog++;
  }
  if (!Serial.available()){
    failedResponseCounter++;
  }
  else{
    //char a = Serial.read();
    currentGear = Serial.read();//par(a);
  }
  if (currentGear==0){
    clutchPOS(COUT);
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
  Serial.write('8');
  /*while(!Serial.available()){
    delay(10);
  }*/
  //char a = Serial.read();
  return Serial.read();//par(a);
}

void launch() {
  int pos = COUT;
  while(analogRead(POS)-pos < -10 || analogRead(POS)-pos >10) {
    //safety to not move if goes to far, make higher than 5?
    if (analogRead(POS)>CIN+5) {
      return;
    }
    if (analogRead(POS) <pos){
      digitalWrite(DIR,LOW);
      analogWrite(PWM,50);
    }
    if(analogRead(POS)>pos){
      digitalWrite(DIR,HIGH);
      analogWrite(PWM,50);
    }
  }
  digitalWrite(PWM,LOW);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  Serial.begin(57600);
  attachInterrupt(digitalPinToInterrupt(2), interruptIRQ, RISING);
  attachInterrupt(digitalPinToInterrupt(3), NewinterruptIRQ, RISING);
  pinMode(POS,INPUT);
  pinMode(PWM,OUTPUT);
  pinMode(DIR,OUTPUT);
  clutchPOS(COUT);
  currentGear = getGear();
}

void loop() {
  // put your main code here, to run repeatedly:
  //print rpm values when update flage is true
  
  /*if (updateflag1) {
    Serial.print("S1: ");
    Serial.println(rpm);
    updateflag1 = false;
  }
  if (updateflag2) {
    Serial.print("S2: ");
    Serial.println(rpm2);
    updateflag2 = false;
  }*/  
  //clutchPOS(position)  moves clutch to position
  if (rpm < changeRPM[currentGear-2])shiftDOWN();
  if (rpm > 3700)shiftUP();
  if (currentGear==1 and rpm<2000){
    clutchPOS(CIN);
    doNeutral = true;
  }
  if (doNeutral){
    nwatchdog++; 
    if (rpm<2000) doNeutral =  false; nwatchdog =0;
    if (nwatchdog>80){
      neutral();
      nwatchdog = 0;
      doNeutral = false;
    }
  }
  if (currentGear==1 and rpm>2400) {
    launch();
  }
  if (failedResponseCounter > 2){
    delay(1000);
    failedResponseCounter = 0;
  }
  delay(50);
  wait_send_get_gear++;
  
  if (wait_send_get_gear >20) {
    currentGear = getGear();
    wait_send_get_gear = 0;
  }
  
}
