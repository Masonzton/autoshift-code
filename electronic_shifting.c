#include "system.h"
#include "usart.h"
#include "port.h"
#include "delay.h"
#include "tcc.h"


//digitial pin code
void configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(CS, &config_port_pin);
	port_pin_set_config(CLOCK, &config_port_pin);
	port_pin_set_config(IGNCUT,&config_port_pin);
}

//digital pin macros
#define digitalWrite(p,t) port_pin_set_output_level(p,t)
#define digitalRead(p) port_pin_get_input_level(p)

//pwm code 

//configuration structures for pwm pins
struct tcc_module tcc_instance;
struct tcc_config config_tcc;

//setup for pins, default to 0 compare value
static void configure_tcc0()
{
	//module tcc0 is being used for this
	tcc_get_config_defaults(&config_tcc, L_MODULE);
	config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV1;
	config_tcc.counter.period = 0xFFFF;
	config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	//setup L1 pin
	config_tcc.compare.match[L1] = (0xFFFF/ 0xFFFF);
	config_tcc.pins.enable_wave_out_pin[L1_OUTPUT] = true;
	config_tcc.pins.wave_out_pin[L1_OUTPUT]        = L1_PIN;
	config_tcc.pins.wave_out_pin_mux[L1_OUTPUT]    = L1_MUX;
	//setup L2 pin
	config_tcc.compare.match[L2] = (0xFFFF/ 0xFFFF);
	config_tcc.pins.enable_wave_out_pin[L2_OUTPUT] = true;
	config_tcc.pins.wave_out_pin[L2_OUTPUT]        = L2_PIN;
	config_tcc.pins.wave_out_pin_mux[L2_OUTPUT]    = L2_MUX;
	//setup L3 pin
	config_tcc.compare.match[L3] = (0xFFFF/ 0xFFFF);
	config_tcc.pins.enable_wave_out_pin[L3_OUTPUT] = true;
	config_tcc.pins.wave_out_pin[L3_OUTPUT]        = L3_PIN;
	config_tcc.pins.wave_out_pin_mux[L3_OUTPUT]    = L3_MUX;
	//intilize everything
	tcc_init(&tcc_instance, L_MODULE, &config_tcc);
	tcc_enable(&tcc_instance);
	tcc_disable_double_buffering(&tcc_instance);
}
//macro for writing pwm signal to pin 
//duty cycle is 1/x %
//use: analogWrite(Ln, value)
//? would this be better to write so that x is between 0,255 and duty cycle increases with x? 
//if so: this should work well enough (based on linear scale)
//#define analogWrite(c,x) tcc_set_compare_value(&tcc_instance,c,256*x+255)
#define analogWrite(c,x) tcc_set_compare_value(&tcc_instance,c,0xFFFF/x)


//PORTA pins (high side bridge) //digital pins
#define H1 PIN_PA08
#define H2 PIN_PA10
#define H3 PIN_PB10

#define CONF_PWM_MODULE      TCC0
#define CONF_PWM_CHANNEL     3 //this number is equal to W0=3 pin
#define CONF_PWM_OUTPUT      3 //I think its the same as the one above but don't really understand it
#define CONF_PWM_OUT_PIN     PIN_PA11F_TCC0_WO3
#define CONF_PWM_OUT_MUX     MUX_PA11F_TCC0_WO3

//digital pins, used with Timer3 library (low side bridge)
#define L_MODULE TCC0
#define L1_PIN PIN_PA09E_TCC0_WO1
#define L1_MUX MUX_PA09E_TCC0_WO1
#define L1 1 //this number is equal to W0=3 pin //this is the channel shortened it so it would be easier to call functions
#define L1_OUTPUT 1 //I think its the same as the one above but don't really understand it

#define L2_PIN PIN_PA11F_TCC0_WO3
#define L2_MUX MUX_PA11F_TCC0_WO3
#define L2 3 //channel
#define L2_OUTPUT 3

#define L3_PIN PIN_PB11F_TCC0_WO5
#define L3_MUX MUX_PB11F_TCC0_WO5
#define L3 5 //channel
#define L3_OUTPUT 5

//encoder digital pins
#define CS PIN_PA23
#define CLOCK PIN_PA22
#define DATA PIN_PA21

//ignition cut digital pin
#define IGNCUT PIN_PB03

//shift button digital pins
//#define SHIFT_UP 18
//#define SHIFT_DOWN 16 

//general definitions
#define PWM_PERIOD 400
#define WATCHDOG 800
#define IGNITION_CUT_TIME 250
#define INTEGRAL_LIMIT 400*INTEGRAL_DIVISOR
#define NEG_INTEGRAL_LIMIT -400*INTEGRAL_DIVISOR
#define CONTROL_LIMIT 1000
#define SECOND_CONTROL_LIMIT 600
#define SECOND_TIME_DELAY 200
#define DEFAULT_PHASE_ADVANCE 90
#define INTEGRAL_DIVISOR 10
#define DERIV_DIVISOR 1
#define POS_ERROR_LIMIT 2
#define NEG_POS_ERROR_LIMIT -2

const int gears[6]={125,21,228,433,634,840};

int setPos, currentPos, kP, kI, kD;
int integral=0;
int prevError=0;

// vars for determining motor step from shaft angle
int valsPerStep=57;
int stepOffset=-197; // projected lowest step 1 minus 1/2 of the difference between steps
int currentGear=0;

//
bool buttonFlag=false;
bool neutralFlag=false;

int getPos() { //TODO: make this run on interrupts instead of taking clock cycles (Currently 200uS)
  digitalWrite(CS, LOW);
  int pos = 0;
  for (int i=0; i<10; i++) {
    digitalWrite(CLOCK, LOW);
    digitalWrite(CLOCK, HIGH);
    
    pos|=digitalRead(DATA)<<9-i;
  }
  digitalWrite(CS, HIGH);
  //account for encoder offset
  pos-=700;
  if (pos<0) {
    pos+=1024;
  }
  
  return pos;
}

void motorStep(int stp, long power, bool secondLimit=false) {
  if (secondLimit) {
    if (power>SECOND_CONTROL_LIMIT) {
      power=SECOND_CONTROL_LIMIT;
    }
  }
  else if (power>CONTROL_LIMIT) {
    power=CONTROL_LIMIT;
  }
  switch(stp) {
    case 0:
      PORTA=0;
      analogWrite(L1, 0);
      analogWrite(L2, 0);
      analogWrite(L3, 0);
      break;
    case 1: // high 1 low 2
      PORTA=0;
      analogWrite(L1, 0);
      analogWrite(L3, 0);
      analogWrite(L2, power);
      PORTA=(1<<H1);
      break;
    case 2: // high 1 low 3
      PORTA=0;
      analogWrite(L1, 0);
      analogWrite(L2, 0);
      analogWrite(L3, power);
      PORTA=(1<<H1);
      break;
    case 3: // high 2 low 3
      PORTA=0;
      analogWrite(L1, 0);
      analogWrite(L2, 0);
      analogWrite(L3, power);
      PORTA=(1<<H2);
      break;
    case 4: // high 2 low 1
      PORTA=0;
      analogWrite(L2, 0);
      analogWrite(L3, 0);
      analogWrite(L1, power);
      PORTA=(1<<H2);
      break;
    case 5: // high 3 low 1
      PORTA=0;
      analogWrite(L2, 0);
      analogWrite(L3, 0);
      analogWrite(L1, power);
      PORTA=(1<<H3);
      break;
    case 6: // high 3 low 2
      PORTA=0;
      analogWrite(L1, 0);
      analogWrite(L3, 0);
      analogWrite(L2, power);
      PORTA=(1<<H3);
      break;
    default:
      PORTA=0;
      analogWrite(L1, 0);
      analogWrite(L2, 0);
      analogWrite(L3, 0);
      break;
  }
}

void setPIDVals(int kp=0, int ki=0, int kd=0) {
  kP=kp;
  kI=ki;
  kD=kd;
}

int PID(int setPoint, int currentPoint) {
  int error=setPoint-currentPoint;
  integral+=error;
  if(integral>INTEGRAL_LIMIT) {
    integral=INTEGRAL_LIMIT;
  }
  else if(integral<NEG_INTEGRAL_LIMIT) {
    integral=NEG_INTEGRAL_LIMIT;
  }
  int deriv=error-prevError;
  prevError=error;
  return kP*error+kI*integral/INTEGRAL_DIVISOR+kD*deriv/DERIV_DIVISOR; 
}

void motorMove(int setPoint, bool ignitionCut=false) {
  int Step, phaseCorrectPos, controlValue;
  int phaseAdvance=DEFAULT_PHASE_ADVANCE;
  int holdCount=1;
  int count=0;
  int posCount=0;
  currentPos=0;
  Serial.println("moving");
  if (ignitionCut) {
    digitalWrite(IGNCUT, HIGH);
  }
  while(posCount<holdCount && count<WATCHDOG) {
    currentPos=getPos();
    //if (currentPos-setPoint>POS_ERROR_LIMIT&&currentPos-setPoint>NEG_POS_ERROR_LIMIT) {
    if (count==IGNITION_CUT_TIME) {
      digitalWrite(IGNCUT, LOW);
    }
    if ((currentPos-setPoint<2)&&(currentPos-setPoint>-2)) {
      posCount++;
    }
    else {
      posCount=0;
    }
    controlValue=PID(setPoint, currentPos);
    //Serial.print(currentPos);
    //Serial.print(", ");
    //Serial.println(controlValue);
    
    if (controlValue<0) {
      phaseCorrectPos=currentPos-phaseAdvance;
      controlValue*=-1;
    }
    else if (controlValue>0) {
      phaseCorrectPos=currentPos+phaseAdvance;
    }

    Step=int((phaseCorrectPos-stepOffset)/valsPerStep)%6+1;
    if (count<SECOND_TIME_DELAY) {
      motorStep(Step,controlValue);
    }
    else {
      motorStep(Step, controlValue, true);
    }
    count++;
    delay(1);
  }
  digitalWrite(IGNCUT, LOW);
  motorStep(0,0);
}

//TODO: write this before adding system to case
void calibrateMotor() { 
  /* pseudo code
   *  numToSubtract=value of any step 1 minus half step width (values between steps)
   *  find lower bound
   *  find upper boung
   *  find gear detent positions 
   */
} 

void selectGear(int gear, bool ignitionCut=false) { // TODO: set all the shifting protection here
    motorMove(gears[gear], ignitionCut);
}

int getCurrentGear() {
  int startPos=getPos();
  int tempGear=0;
  if (startPos<70) {
    tempGear=1;
  }
  else if (startPos<180) {
    tempGear=0;
  }
  else if (startPos<280) {
    tempGear=2;
  }
  else if (startPos<500) {
    tempGear=3;
  }
  else if (startPos<700) {
    tempGear=4;
  }
  else {
    tempGear=5;
  }
  return tempGear;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Ready");

  Serial1.begin(57600);
  
/*  pinMode(CS, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(DATA, INPUT);
  pinMode(IGNCUT, OUTPUT);
  pinMode(SHIFT_UP, INPUT_PULLUP);
  pinMode(SHIFT_DOWN, INPUT_PULLUP);
  */
  DDRA=B00000111;
  PORTA=0;
  
  Timer3.initialize(PWM_PERIOD);
  Timer3.pwm(L1, 0);
  Timer3.pwm(L2, 0);
  Timer3.pwm(L3, 0);
  
  motorStep(0,0);
  
  digitalWrite(CS, HIGH);
  digitalWrite(CLOCK, LOW);

  //get current gear
  currentGear=getCurrentGear();

}

void main() {
	
while(true) {
  //Test ignition cut
  /*digitalWrite(IGNCUT, HIGH);
  delay(500);
  digitalWrite(IGNCUT, LOW);
  delay(500);*/
  
  // Test gear selection
  /*setPIDVals(10,0,30);
  Serial.println("select gear");
  while(!Serial.available());
  int gearSel=Serial.parseInt();
  selectGear(gearSel);
  //selectGear(3);
  //motorMove(433);
  delay(1000);
  //selectGear();
  //motorMove(634);
  //delay(1000);*/

  // test encoder
  /*Serial.println(getPos());
  delay(500);*/

  // Test motorMove
  /*setPIDVals(10,1,0);
  Serial.print("currentPos is: ");
  Serial.println(getPos());
  Serial.println("set position");
  while(!Serial.available());
  setPos=Serial.parseInt();
  motorMove(setPos);
  delay(100);*/

  // Test motorStep
  /*delay(500);
  Serial.println(getPos());
  delay(50);
  motorStep(0,0);
  Serial.println("step");
  while(!Serial.available());
  int STP=Serial.parseInt();
  Serial.println(STP);
  //Serial.println("power");
  //while(!Serial.available());
  //int POW=Serial.parseInt();
  motorStep(STP, 500);*/

  //Prelim Shift Code
  if (Serial1.available()) {
    char a=Serial1.read();
    Serial.print(a);
    bool reshift=false;
    if (a=='1' or a=='4') {
      currentGear++;
      if (currentGear>5) {
        currentGear=5;
        reshift=true;
      }
      if (reshift==false) {
        setPIDVals(10,0,30);
        selectGear(currentGear, true);
      }
      reshift=false;
    }
    else if (a=='2' or a=='6') {
      currentGear--;
      if (currentGear<1) {
        currentGear=1;
        reshift=true;
      }
      if (reshift==false) {
        setPIDVals(10,0,30);
        selectGear(currentGear, false);
      }
      reshift=false;
    }
    else if (a=='5') {
      if (currentGear==1 or currentGear==2) {
        currentGear=0;
        setPIDVals(10,0,30);
      selectGear(currentGear, false);
      }
    }
    Serial1.write(currentGear);
  }
  currentGear=getCurrentGear();
  Serial.println(currentGear);
  delay(100);
}
}
