#include <Bot_IR.h>
//#include <Bot_Motor.h>
#include <Bot_Remote.h>
#include <Bot_RemoteType.h>

Bot_IR ir;

// Arduino standard LED
const uint8_t Led = 13;

// Lets try something different...
#include "audio.h"


void setup()
{
  Serial.begin(57600);	//Init the baudrate

  ir.setup();
  //initSpecBeatDetector();
  pinMode(Led, OUTPUT);
  pinMode(STROBEPIN, OUTPUT );     
  pinMode(RESETPIN, OUTPUT );     

  for ( int x = 0; x < 5; x++ ) 
  {
    digitalWrite(Led,HIGH);
    delay(200);
    digitalWrite(Led,LOW);
    delay(200);
  }
}


int beats_per_minute = 102;
int milli_delay = 60000. / beats_per_minute;
unsigned long  next_beat = 0;
void loop()
{
  currentMillis = millis(); // save the current timer value
  int got_beat;
  doAnalogs();

  for(int i=0;i<7;i++)            
  {
 //   Serial.print(spectrumValue[i])  ;//Write the DC value of the seven bands
 //   if(i<6)  Serial.print(",");
 //   else Serial.println();
  }
  got_beat = beatDetect();
  //got_beat = specBeatDetect();
  if ( /*(currentMillis >= next_beat ) || */ got_beat ) {
     if ( 1 || next_beat - currentMillis > milli_delay / 10 ) {
       Serial.print(currentMillis); Serial.print( ","); Serial.print(next_beat); Serial.print( ",");
       digitalWrite(Led, HIGH);
       delay(5); // 
       digitalWrite(Led, LOW);
       if (!ir.txBusy()) {
         ir.txData(Remote_hash);
    
       }
     }
     if ( got_beat ) { 
       Serial.println("BEAT");
     }
     next_beat = currentMillis + milli_delay;
  }
  delay(20);
}
