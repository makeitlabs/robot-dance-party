#include <Bot_IR.h>
#include <Bot_Motor.h>
#include <Bot_RemoteType.h>

// Requires _2020Bot_Library and TimerOne
// Both are installable via the Arduino Library Manager.

// Default remote will be the standard 2020 Bot remote
// If you have a different one that is defined in Bot_Remote.h
// select it here.
// Otherwise, look at Bot_Remote.h for the required constants,
// and use the Remote Receiver example to decode your specific
// remote control.  NOT ALL REMOTES WILL WORK!

//#define RemoteType VelikkaRemote

#include <Bot_Remote.h>

Bot_IR ir;
Bot_Motor motor;

// Arduino standard LED
const uint8_t Led = 13;

// IR Frequency adjustment items
unsigned int irFrequency = 30000;
const unsigned int IrStep = 2000;
const unsigned int IrMax = 50000;
const unsigned int IrMin = 20000;
bool irDetect = false;

// define the speed of motor
int speed = 100;
int motorOffset = 0;
const int MotorOffsetMax = 10;
const int MotorOffsetMin = -10;
const int MotorOffsetStep = 2;
const unsigned long Timeout = 5000;

typedef enum {None, Forward, Backward, Left, Right, Stop} motion_t;
typedef enum {Remote, Autonomous} robotMode_t;

motion_t robotAction = None;

// behavior requested by bumpers
motion_t bumperRequest;

robotMode_t robotMode = Remote;
unsigned long beatMillis = 60000 / 120; 
unsigned long lastBeat;
unsigned long nextBeat;

const unsigned int MAX_MOVES = 5;
const unsigned int MAX_STEPS = 8;
motion_t danceMoves[MAX_MOVES][8] = {
  { Left, Right, Forward, Backward, Left, Right, Forward, Backward }, 
  { Left, Forward, Left, Forward, Left, Forward, Left, Forward  }, 
  { Right, Forward, Right, Forward, Right, Forward, Right, Forward },
  { Left, Left, Left, Left, Left, Left, Left, Left },
  { Right, Right, Right, Right, Right, Right, Right, Right } 

};

int moveIndex = 0;
int stepIndex = 0;

bool dancing = false;

void setup() {
  Serial.begin(57600);

  ir.setup();
  motor.setup();

  pinMode(Led, OUTPUT);     
   
  for ( int x = 0; x < 3; x++ ) 
  {
    digitalWrite(Led,HIGH);
    delay(100);
    digitalWrite(Led,LOW);
    delay(50);
  }

  lastBeat = millis() + beatMillis;
}

motion_t bumperBehavior() {
  static motion_t lastBumped = None;
  static motion_t bumpAction = None;
  motion_t bumped = None;

  if (irDetect) {
    bumped = Forward;
  }

  // decide on action
  if (bumped != None) {  
    if ((bumpAction != None) && (lastBumped == bumped)) {
      // stay on target...
      // leave bumpAction alone
    }
    else {
      if (bumped == Forward) {
        bumpAction = random(0,2) ? Left : Right;
      }
      else if (bumped == Left) {
        bumpAction = Right;
      }
      else if (bumped == Right) {
        bumpAction = Left;
      }
    }
  }
  else {
    bumpAction = None;
  }

  lastBumped = bumped;
    
  return bumpAction;
}

motion_t arbitrate() {
  motion_t arb = Forward;
  
  if (bumperRequest != None) {
    arb = bumperRequest;
  }

  return arb;
}



void loop() {
  static unsigned long timeoutTimer = 0;
  static unsigned long delayTimer = 0;
  uint32_t irCode;
  static unsigned long motionTimeout = Timeout;

  static unsigned long pressTime;
  static unsigned long lastPressTime = 0;
  unsigned long delta;
  
  if (ir.rxDataReady()) {
    
    irCode = ir.rxData();
    
    if (irCode != Remote_no_code) {
      Serial.print("Received: 0x");
      Serial.println(irCode, HEX);
    }

    switch (irCode) {
      case Remote_play:
      case Remote_hash:
        robotAction = None;
        lastPressTime = pressTime;
        pressTime = millis();
        delta = pressTime - lastPressTime;

        if ( robotMode == Autonomous ) { 
          dancing = true;
          Serial.println( delta );
          if ( (delta < 1200) && (delta > 200) ) { // only capture 50 - 300 bpm pulses
            beatMillis = delta;
            delayTimer = pressTime;
            nextBeat = pressTime;
         }
        } else {
          robotMode = Autonomous;
          delayTimer = pressTime;
          nextBeat = pressTime;
          lastBeat = pressTime;
        }
        break;
      case Remote_prev:
      case Remote_asterisk:
        robotAction = Stop;
        robotMode = Remote;
        motionTimeout = Timeout;
        dancing = false;
        break;
      case Remote_2:
      case Remote_up_arrow:
        robotAction = Forward;
        motionTimeout = Timeout;
        break;
      case Remote_8:
      case Remote_down_arrow:
        robotAction = Backward;
        motionTimeout = Timeout;
        break;
      case Remote_4:
      case Remote_left_arrow:
        robotAction = Left;
        motionTimeout = 100;
        break;
      case Remote_6:
      case Remote_right_arrow:
        robotAction = Right;
        motionTimeout = 100;
        break;
      case Remote_5:
      case Remote_ok:
        robotAction = Stop;
        motionTimeout = Timeout;
        break;
      case Remote_vol_plus:
      case Remote_3:
        irFrequency += IrStep;
        irFrequency = constrain(irFrequency, IrMin, IrMax);
        break;
      case Remote_vol_minus:
      case Remote_1:
        irFrequency -= IrStep;
        irFrequency = constrain(irFrequency, IrMin, IrMax);
        break;      
      case Remote_9:
        motorOffset += MotorOffsetStep;
        motorOffset = constrain(motorOffset, MotorOffsetMin, MotorOffsetMax);
        motor.motorOffset(motorOffset);
        break;
      case Remote_7:
        motorOffset -= MotorOffsetStep;
        motorOffset = constrain(motorOffset, MotorOffsetMin, MotorOffsetMax);
        motor.motorOffset(motorOffset);
        break;
      default:
        robotAction = None;
        break;
    }
    timeoutTimer = millis() + motionTimeout;
  }
  
  if (robotMode == Remote) {
    if (millis() >= timeoutTimer) {
      robotAction = Stop;
      timeoutTimer = millis() + motionTimeout;
    }

    switch (robotAction) {
      case Forward:
        motor.forward(speed);
        break;
      case Backward:
        motor.backward(speed);
        break;
      case Left:
        motor.left(speed);
        break;
      case Right:
        motor.right(speed);
        break;
      case None:
        break;
      default:
        motor.stop();
        break;
    }
  }
  else {
    // autonomous mode

   if (millis() >= delayTimer) {
    
      motion_t move = Stop;
      if ( dancing ) { 
        if ( millis() >= nextBeat ) { 
          lastBeat = millis();
          nextBeat += beatMillis;
          move = danceMoves[moveIndex][stepIndex];
          stepIndex++;

          if ( stepIndex == MAX_STEPS ) {
            stepIndex = 0;
            moveIndex++;
            if ( moveIndex == MAX_MOVES ) moveIndex = 0;
          }
        }
      } else  {
        irDetect = ir.ping();
       
        bumperRequest = bumperBehavior();
        move = arbitrate();
      }

      switch (move) {
        case Forward:
          motor.forward(speed);
          delayTimer = millis() + 50;
          break;
        case Backward:
          motor.backward(speed);
          delayTimer = millis() + 50;
          break;        
        case Left:
          motor.left(speed);
          delayTimer = millis() + 100;
          break;
        case Right:
          motor.right(speed);
          delayTimer = millis() + 100;
          break;        
        case Stop:
          motor.stop();
          delayTimer = millis() + 50;
          break;
        default:
          break;
      }
    }        
  }

  
} // end loop
 
 
