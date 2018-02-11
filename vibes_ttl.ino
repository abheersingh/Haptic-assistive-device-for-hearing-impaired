  /*
* 
* Tunables : TTL, Number of samples, Trigger Threshold
* 
* An array of SoundSamples holds the previously collected loudest sound levels.
* Each sample has a correxponding TTL. Samples with 0 TTL are effectively 0.
* We also keep index of currently highest (loudest) and lowest sound sample 
* and the sample with lowest TTL.
*
* On getting a new sound sample:
* Walk through the array of SoundSamples, reduce ttl of all samples 
* by time period, zeroing our the samples for which  the ttl has expired i.e.
* the new ttl is 0 or -ve). 
*
* If the new sample is louder than the lowest sample, it replaces the sample
* with lowest TTL. If it also crosses the trigger level, we trigger.
*
* Trigger threshold is a tunable and should be dynamic. It will therefore generally 
* require update when new sound sample is added to the sound sample cache.
*
* Possible triggers : 2 * the average of the sound samples and louder than the loudest
* sample in the cache.
*
*/


// Set to 1 to enable debug messages on serial
#define DEBUG  0


// Tunables
#define SAMPLES   10        // Number of seconds
#define TTL       4 * 1000000  // num * 1000000 micro seconds
#define SAMPLEPERIOD  45   // 6 micro seconds
#define VIBETHRESHOLD  2   // We buzz if the current sample is VIBETHRESHOLD higher than current average

// Input pin - black wire on zx-sound board connects here. Red goes to 5V, yellow to ground.
#define inputPin A0

typedef struct {
  long int value;
  long int ttl;
} SoundSample;


SoundSample samples[SAMPLES];

//These are indices in the samples array
int lowestTTLIndex;
int lowestSoundIndex;
int highestSoundIndex;


int total = 0;                  // the running total
float average = 0;                // the average
float threshold;



void checkVibeThreshold(int currentSample);
void checkVibeThreshold2(int currentSample);
void checkVibeThresholdWithRange(int currentSample);
void adjustAllIndices();
void decrementTTLOfAllSamples();
void printIndicesAndSamples();


void setup()
{
  // initialize serial communication with computer:
  Serial.begin(115200);                   

#if DEBUG
  Serial.println("Initializing samples");
#endif

  lowestSoundIndex = 0;
  highestSoundIndex = 0;
  lowestTTLIndex = 0;

  for (int thisSample = 0; thisSample < SAMPLES; thisSample++)
  {  
    samples[thisSample].value = analogRead(inputPin);
    samples[thisSample].ttl = TTL;
    total = samples[thisSample].value + total;
    
    if(samples[thisSample].value > samples[highestSoundIndex].value) {
      highestSoundIndex = thisSample;
    } else if (samples[thisSample].value < samples[lowestSoundIndex].value) {
      lowestSoundIndex = thisSample;
    }
    
    samples[thisSample].ttl = (TTL - SAMPLEPERIOD * SAMPLES) + ((thisSample + 1) * SAMPLEPERIOD);
    delayMicroseconds(SAMPLEPERIOD);
  }
  average = total/SAMPLES;



  Serial.println("Done initializing. Here are the values ...");

#if DEBUG
  printIndicesAndSamples();  
#endif

}


void loop() {
  
  int samplesAdjusted = 0;

  int currentSample = analogRead(inputPin);
  
#if DEBUG
  Serial.print("currentSample = "); Serial.println(currentSample);
#endif

// checkVibeThreshold(currentSample);
 // checkVibeThreshold2(currentSample);
  checkVibeThresholdWithRange(currentSample);  
  
  
  decrementTTLOfAllSamples();
  
  
  if(samples[lowestTTLIndex].ttl <= 0) {
    total = total - samples[lowestTTLIndex].value;
    samples[lowestTTLIndex].value = 0;
    lowestSoundIndex = lowestTTLIndex;
  }

  if( currentSample >= samples[lowestSoundIndex].value) {
    total = total - samples[lowestSoundIndex].value + currentSample;
    average = total/SAMPLES;
    samples[lowestSoundIndex].value = currentSample;
    samples[lowestSoundIndex].ttl = TTL;
    
    samplesAdjusted = 1;
  }

  
  if (samplesAdjusted) {
    adjustAllIndices();
  }
  
  delayMicroseconds(SAMPLEPERIOD);


#if DEBUG
  Serial.println("Updated values ...");
  printIndicesAndSamples();  
//  delay(2000); // Delay for couple of seconds so the values can be read.
#endif

   
}

void checkVibeThreshold(int currentSample) {

/*  if(currentSample > average) {
    Serial.print("currentSample : "); Serial.println(currentSample);
    Serial.print("Average : "); Serial.println(average);
  }
*/
  threshold = VIBETHRESHOLD*average;
  if((currentSample > threshold) /* && (currentSample > samples[highestSoundIndex].value)*/){
    Serial.print("currentSample : "); Serial.println(currentSample);    
    Serial.print("Average : "); Serial.println(average);    
    Serial.print("Threshold : "); Serial.println(threshold);    
    Serial.println("Triggered !!! "); 
    Serial.print("samples[highestSoundIndex].value : "); Serial.println(samples[highestSoundIndex].value);
    
    printIndicesAndSamples();
    
  }

}





void checkVibeThreshold2(int currentSample) {
  
  int factor= (int(average) / 64);



  if(currentSample <= 288) {
    threshold = (2 - float(factor*0.125)) *average;
    if( currentSample >= threshold) {
      Serial.print("currentSample : "); Serial.println(currentSample);    
      Serial.print("Average : "); Serial.println(average);    
      Serial.print("Threshold : "); Serial.println(threshold);    
      Serial.println("Triggered !!! "); 
      printIndicesAndSamples();
    
    }} else if ( (currentSample >= average) || (currentSample >= 960) ){
      threshold = average;      
      Serial.print("currentSample : "); Serial.println(currentSample);    
      Serial.print("Average : "); Serial.println(average);    
      Serial.print("Threshold : "); Serial.println(threshold);    
      Serial.println("Triggered !!! "); 
      printIndicesAndSamples();
    }

}

void checkVibeThresholdWithRange(int currentSample) {
  

  if(average < 16) {
    threshold = 4 * average;
  } else if(average < 32) {
    threshold = 2.5 * average;
  } else if (average < 64) {
    threshold = 2 * average;
  } else if (average < 128) {
    threshold = 1.75 * average;
  } else if (average < 192) {
   threshold = 1.5 * average;
  } else if (average < 256) {
   threshold = 1.25 * average;
  } else if (average < 320) {
    threshold = 1.125 * average;
  } else {
    threshold = average;
  }

  if (currentSample > threshold){
        Serial.print("currentSample : "); Serial.println(currentSample);    
        Serial.print("Average : "); Serial.println(average);    
        Serial.print("Threshold : "); Serial.println(threshold);    
        Serial.println("Triggered !!! "); 
        printIndicesAndSamples();
      }      
      
       
  
  
 

}



void decrementTTLOfAllSamples() {
  
  for(int sampleNumber = 0; sampleNumber < SAMPLES; sampleNumber++)
  {
    samples[sampleNumber].ttl = samples[sampleNumber].ttl - SAMPLEPERIOD;
  }
  
}


void adjustAllIndices() {
  
  lowestSoundIndex = 0;
  highestSoundIndex = 0;
  lowestTTLIndex = 0;

  for(int sampleNumber = 0; sampleNumber < SAMPLES; sampleNumber++)
  {
    if(samples[sampleNumber].ttl < samples[lowestTTLIndex].ttl) {
      lowestTTLIndex = sampleNumber;
    }

     if(samples[sampleNumber].value < samples[lowestSoundIndex].value) {
       lowestSoundIndex = sampleNumber;
    }
    
    if(samples[sampleNumber].value > samples[highestSoundIndex].value) {
      highestSoundIndex = sampleNumber;
    }
    
  }
  Serial.print(average); Serial.print(" , "); Serial.println(threshold);
  
}
  


void printIndicesAndSamples() {
  
 Serial.print("At "); Serial.println(millis());
  
  for (int thisSample = 0; thisSample < SAMPLES; thisSample++)
  { 
    Serial.print("sample["); Serial.print(thisSample); Serial.print("] : value = "); 
    Serial.print(samples[thisSample].value); Serial.print(", ttl = "); 
    Serial.println(samples[thisSample].ttl);
    
  }

  Serial.print("Total = "); Serial.print(total); Serial.print(" Average = "); Serial.println(average);
  Serial.println(); Serial.println();    
  
  Serial.print("lowestTTLIndex : "); Serial.println(lowestTTLIndex);
  Serial.print("lowestSoundIndex : "); Serial.println(lowestSoundIndex);
  Serial.print("highestSoundIndex : "); Serial.println(highestSoundIndex);
  Serial.println();

}

