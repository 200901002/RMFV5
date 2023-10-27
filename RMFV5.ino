#include <Adafruit_NeoPixel.h>
#include <TimerOne.h>

#define SAMPLES 10

#define LED_PIN 7
#define RPhase 2
#define LED_COUNT 57
#define ZERO_ERROR 0.8
#define MIN_FREQ   1.1
#define MAX_FREQ   25

volatile unsigned long risingEdges[SAMPLES];
volatile byte index = 0;  
unsigned long startTime;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

float frequency = 0;

unsigned long endTime = 0;
bool risingEdge = false;
bool measureFrequency = true;

// A global variable to store the frequency value updated by the interrupt handler
volatile float currentFrequency = 0;

// A state variable to keep track of the animation state
int animationState = 0;

// A timer variable to keep track of the animation time
unsigned long animationTimer = 0;

void setup() {
  Timer1.initialize(1000); // 1ms period
  Timer1.attachInterrupt(calculateFrequency);

  pinMode(RPhase, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RPhase), handleInterrupt, CHANGE);
  strip.begin();       
  strip.show();
  strip.setBrightness(255);
}
void loop() {

  // Use the global variable instead of calling the function again
  frequency = currentFrequency;

  if(frequency > MIN_FREQ && frequency < MAX_FREQ){
    float timePeriod = (1 / frequency) * 1000;
    // Call the non-blocking version of the animation function
    northSouthChasingNonBlocking(timePeriod / LED_COUNT);
    delay(timePeriod / (LED_COUNT * 4)); 
    delay(timePeriod / (LED_COUNT * 4));
  } 
  else if(frequency > MAX_FREQ){
    float timePeriod = (1 / MAX_FREQ) * 1000;
    // Call the non-blocking version of the animation function
    northSouthChasingNonBlocking(timePeriod / LED_COUNT);
  }
  else{
    strip.clear();
    strip.setPixelColor(LED_COUNT / 2, strip.Color(255, 255, 0));
    strip.show();
    delay(500);
    strip.clear();
    strip.show();
    delay(500);
  }
}

// A non-blocking version of the animation function that uses a state machine and a timer
void northSouthChasingNonBlocking(float wait) {
  
  // Get the current time in milliseconds
  unsigned long currentTime = millis();

  // Check if the wait time has elapsed since the last state change
  if(currentTime - animationTimer >= wait) {
    
    // Update the timer variable with the current time
    animationTimer = currentTime;

    // Increment the state variable and wrap around if it reaches LED_COUNT
    animationState++;
    if(animationState >= LED_COUNT) {
      animationState = 0;
    }

    // Calculate the red and blue indices based on the state variable
    int redIndex = animationState;
    int blueIndex = (animationState + (LED_COUNT / 2)) % LED_COUNT;

    // Clear the strip and set the pixels according to the indices
    strip.clear(); 
    strip.setPixelColor(redIndex, strip.Color(255, 0,0));
    strip.setPixelColor(blueIndex, strip.Color(0, 0, 255));
    
    // Show the strip without blocking
    strip.show();
    
  }
  
}

void handleInterrupt() {

  
   // Check if this is a rising edge or a falling edge
   if(digitalRead(RPhase) == HIGH) {
     risingEdge = true;
   } else {
     risingEdge = false;
   }

   // If this is a rising edge, record the time and increment the index
   if(risingEdge) {

     if(index == 0) {
       startTime = micros(); // Use micros() instead of millis() for higher accuracy
     } else {
       risingEdges[index] = micros() - startTime; // Use micros() instead of millis() for higher accuracy
     }

     index++;

     if(index >= SAMPLES) {
       index = SAMPLES -1; // Do not wrap around the index, keep it at the last position
     }
     
   }
  
}

void calculateFrequency() {

   unsigned long total = 0;
   int validIntervals = SAMPLES -1; // Assume there are SAMPLES -1 valid intervals

   for(int i=1; i<SAMPLES; i++) {

     // Check if there is a valid interval between two rising edges
     if(risingEdges[i] > risingEdges[i-1]) {
       // Add the interval to the total
       total += risingEdges[i] - risingEdges[i-1]; 
     } else {
       // If not, decrement the valid intervals count
       validIntervals--;
     }
     
   }

   // Check if there are any valid intervals
   if(validIntervals > 0) {
     // Calculate the average period using the valid intervals
     unsigned long period = total / validIntervals;

     // Calculate the frequency using the period
     frequency = 1000000.0 / period; // Use 1000000.0 instead of 1000.0 because micros() is used

     // Update the global variable with the frequency value
     currentFrequency = frequency;
   } else {
     // If not, set the frequency to zero
     frequency = 0;
     currentFrequency = 0;
   }
   
}