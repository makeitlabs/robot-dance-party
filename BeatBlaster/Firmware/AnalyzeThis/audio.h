// Interface with MSGEQ7 chip for audio analysis

#define AUDIODELAY 8

// Pin definitions
#define ANALOGPIN 0
#define STROBEPIN 4
#define RESETPIN 5

// Smooth/average settings
#define SPECTRUMSMOOTH 0.25
#define PEAKDECAY 0.05
#define NOISEFLOOR 75

// AGC settings
#define AGCSMOOTH 0.15
#define GAINUPPERLIMIT 15.0
#define GAINLOWERLIMIT 0.1

// Global variables
unsigned int spectrumValue[7];  // holds raw adc values
float spectrumDecay[7] = {0};   // holds time-averaged values
float spectrumPeaks[7] = {0};   // holds peak values
float audioAvg = 300.0;
float gainAGC = 1.0;
float beatAvg1 = 0;             // holds previous beatAvg
float beatAvg2 = 0;             // holds previous previous beatAvg
float specCombo1 = 0;          // holds specCombo
float specCombo2 = 0;          // holds previous specCombo
float specCombo3 = 0;          // holds previous previous specCombo
float specCombo4 = 0;          // holds previous previous previous specCombo
float beatSmooth1 = 0;         // holds beatSmooth
float beatSmooth2 = 0;         // holds previous beatSmooth
float beatSmooth3 = 0;         // holds previous previous beatSmooth

void doAnalogs() {

  static PROGMEM const byte spectrumFactors[7] = {6, 8, 8, 8, 7, 7, 10};

  // reset MSGEQ7 to first frequency bin
  digitalWrite(RESETPIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(RESETPIN, LOW);
  delayMicroseconds(10);
   

  // store sum of values for AGC
  unsigned int analogsum = 0;

  // cycle through each MSGEQ7 bin and read the analog values
  for (int i = 0; i < 7; i++) {

    // set up the MSGEQ7
    digitalWrite(STROBEPIN, LOW);
    delayMicroseconds(25); // allow the output to settle

    // read the analog value
    spectrumValue[i] = (analogRead(ANALOGPIN)+analogRead(ANALOGPIN)+analogRead(ANALOGPIN))/3;
    digitalWrite(STROBEPIN, HIGH);
    delayMicroseconds(30);
    //Serial.println( spectrumValue[i] );
    // noise floor filter
    if (spectrumValue[i] < NOISEFLOOR) {
      spectrumValue[i] = 0;
    } else {
      spectrumValue[i] -= NOISEFLOOR;
    }

    // apply correction factor per frequency bin
    spectrumValue[i] = (spectrumValue[i]*pgm_read_byte(spectrumFactors+i));
    spectrumValue[i] /= 10;

    // prepare average for AGC
    analogsum += spectrumValue[i];

    // apply current gain value
    spectrumValue[i] *= gainAGC;

    // process time-averaged values
    spectrumDecay[i] = (1.0 - SPECTRUMSMOOTH) * spectrumDecay[i] + SPECTRUMSMOOTH * spectrumValue[i];

    // process peak values
    if (spectrumPeaks[i] < spectrumDecay[i]) spectrumPeaks[i] = spectrumDecay[i];
    spectrumPeaks[i] = spectrumPeaks[i] * (1.0 - PEAKDECAY);

  }

  // Calculate audio levels for automatic gain
  audioAvg = (1.0 - AGCSMOOTH) * audioAvg + AGCSMOOTH * (analogsum / 7.0);

  // Calculate gain adjustment factor
  gainAGC = 300.0 / audioAvg;
  if (gainAGC > GAINUPPERLIMIT) gainAGC = GAINUPPERLIMIT;
  if (gainAGC < GAINLOWERLIMIT) gainAGC = GAINLOWERLIMIT;
  //Serial.println(gainAGC);

}

// Attempt at beat detection
byte beatTriggered = 0;
byte peak = 0;
#define beatLevel 15.0
#define beatDeadzone 50.0
#define beatDelay 290
#define shortbeat 290 //msec
#define longbeat 1200 //msec
#define BEATSMOOTHER 0.50
float lastBeatVal = 0;

unsigned long currentMillis; // store current loop's millis value


byte beatDetect() {
  static float beatAvg = 0;
  static float beatSmooth = 500; //msec
  static unsigned long lastBeatMillis;
  float specCombo = 0.45 * spectrumDecay[0] + 0.55 * spectrumDecay[1] + 0 * spectrumDecay[5];
  //beatAvg = (1.0 - AGCSMOOTH) * beatAvg + AGCSMOOTH * specCombo;
  
  beatSmooth = (1.0 - BEATSMOOTHER) * beatSmooth + BEATSMOOTHER * (currentMillis - lastBeatMillis);

  Serial.print(currentMillis); Serial.print(",");
  Serial.print(lastBeatMillis); Serial.print(",");
  Serial.print(peak); Serial.print(",");
  //Serial.print(beatTriggered); Serial.print(",");
  //Serial.print(lastBeatVal); Serial.print(",");
  Serial.print(beatAvg); Serial.print(",");
  Serial.print(beatAvg1); Serial.print(",");
  Serial.print(beatAvg2); Serial.print(",");
  Serial.print(specCombo); Serial.print(",");
  Serial.print(specCombo1); Serial.print(",");
  Serial.print(specCombo2); Serial.print(",");
  Serial.print(specCombo3); Serial.print(",");
  Serial.print(specCombo4); Serial.print(",");
  Serial.print(beatSmooth); Serial.print(",");
  Serial.print(beatSmooth1); Serial.print(",");
  Serial.print(beatSmooth2); Serial.print(",");
  Serial.print(beatSmooth3); Serial.println(",");

  if(specCombo1 > specCombo && specCombo1 > specCombo2 && (currentMillis - lastBeatMillis) > shortbeat && peak == 0){
    peak = 1;
    //beatAvg2 = beatAvg1;
    //beatAvg1 = beatAvg;
    specCombo4 = specCombo3;
    specCombo3 = specCombo2;
    specCombo2 = specCombo1;
    specCombo1 = specCombo;
    beatSmooth3 = beatSmooth2;
    beatSmooth2 = beatSmooth1;
    beatSmooth1 = beatSmooth;
  } else {
    peak = 0;
    //beatAvg2 = beatAvg1;
    //beatAvg1 = beatAvg;
    specCombo4 = specCombo3;
    specCombo3 = specCombo2;
    specCombo2 = specCombo1;
    specCombo1 = specCombo;
    beatSmooth3 = beatSmooth2;
    beatSmooth2 = beatSmooth1;
    beatSmooth1 = beatSmooth;
  }



  if(peak == 1 && (currentMillis - lastBeatMillis) >= longbeat && specCombo1 > 200){
    
    lastBeatMillis = currentMillis;
    
    //peak = 0;
    return 1;
  } else if(peak == 1 && (currentMillis - lastBeatMillis) < longbeat && (beatSmooth1 + beatSmooth2 + beatSmooth3)/3 > 0.80 * beatSmooth && (beatSmooth1 + beatSmooth2 + beatSmooth3)/3 < 1.20 * beatSmooth){
    
    lastBeatMillis = currentMillis;
    
    //peak = 0;
    return 1;
  } else {
    peak = 0;
    return 0;
  }
  
}
/*
byte specBeatTriggered[7];
float specLastBeatVal[7];
float specBeatAvg[7];
unsigned long specLastBeatMillis[7];


void initSpecBeatDetector() {
  for ( int i = 0 ; i < 7 ; i++ ) {
     specBeatTriggered[i] = 0;
     specLastBeatVal[i] = 0;
     specBeatAvg[i] = 0; 
     specLastBeatMillis[i] = 0;
  }
}


byte specBeatDetect() {
  int count = 0;
 // Serial.print( "Triggers:" );
  for ( int i=0 ; i < 7 ; i++ ) {
     
    specBeatAvg[i] = (1.0 - AGCSMOOTH) * specBeatAvg[i] + AGCSMOOTH * spectrumDecay[i];
  //  Serial.print(specBeatAvg[i]);
    if (specLastBeatVal[i] < specBeatAvg[i]) specLastBeatVal[i] = specBeatAvg[i];
    if ((spectrumDecay[i] - specBeatAvg[i]) > beatLevel && specBeatTriggered[i] == 0 && 
      currentMillis - specLastBeatMillis[i] > beatDelay) {
      specBeatTriggered[i] = 1;
    //  Serial.print(i);
//    Serial.println(currentMillis - specLastBeatMillis[i] );
      specLastBeatVal[i] = spectrumDecay[i];
      specLastBeatMillis[i] = currentMillis;
      count++;
    } else if ((specLastBeatVal[i] - spectrumDecay[i]) > beatDeadzone) {
      specBeatTriggered[i] = 0;
    //  Serial.print('_');
    } else {
    //  Serial.print('_');
    }
  }
 // Serial.println();
  if ( count > 1) {
    return 1;
  } else {
    return 0;
  }
}
*/
