#include <Arduino.h>
#include <Wire.h>           // Include the I2C library (required)
#include <SparkFunSX1509.h> //Click here for the library: http://librarymanager/All#SparkFun_SX1509

// Rail power and throttle analog pins
#define THROTTLE_IN A1
#define THROTTLE_OUT_1 11
#define THROTTLE_OUT_2 12
#define MAX_THROTTLE 1023
#define DIRECTION 10

// Track position pins
#define NUM_TRACK_POSITIONS 4
#define TRACK_POS_0 2
#define TRACK_POS_1 3
#define TRACK_POS_2 4
#define TRACK_POS_3 5

// Track toggle switch pins
#define NUM_TRACK_TOGGLES 4
#define TRACK_TOGGLE_0 6
#define TRACK_TOGGLE_1 7
#define TRACK_TOGGLE_2 8
#define TRACK_TOGGLE_3 9

// IO expander pins for switch solenoids
#define NUM_TRACK_SWITCHES 8
#define TRACK_SWITCH_0 0
#define TRACK_SWITCH_1 1
#define TRACK_SWITCH_2 2
#define TRACK_SWITCH_3 3
#define TRACK_SWITCH_4 4
#define TRACK_SWITCH_5 5
#define TRACK_SWITCH_6 6
#define TRACK_SWITCH_7 7

/* Diagram of the track layout
        S2
5 |             | 7
--------------------
4 |             | 6
  |             |
  |             |
S1|             |S3
  |             |
  |             |
1 |             | 3
--------------------
0 |             | 2
  |             |
S0|             |
  |             |
  |             |
--------------------
*/

// SX1509 I2C address (set by ADDR1 and ADDR0 (00 by default):
const byte SX1509_ADDRESS = 0x3E; // SX1509 I2C address
SX1509 io;                        // Create an SX1509 object to be used throughout

// Train state struct
typedef struct {
  int currentStation;
  int currentDirection;
  int currentSpeed;
}train_state_t;

train_state_t TrainState = {
  .currentStation = 0,
  .currentDirection = 0,
  .currentSpeed = 0
};

// Track state struct
typedef struct {
  int trackToggle[4];
  int trackSwitch[8];
}track_state_t;

track_state_t TrackState = {
  .trackToggle = {0, 0, 0, 0},
  .trackSwitch = {0, 0, 0, 0, 0, 0, 0, 0}
};

// Loop counter for state machine
unsigned long loopCounter = 0;

// Station count array for testing
volatile int stationCount[NUM_TRACK_POSITIONS] = {0, 0, 0, 0};

void station0ISR()
{
  stationCount[0]++;
}

void station1ISR()
{
  stationCount[1]++;
}

void station2ISR()
{
  stationCount[2]++;
}

void station3ISR()
{
  stationCount[3]++;
}

// Method to handle the track switching logic
void switchTrack(int currentStation, int currentDirection) 
{
  // Based on the direction, switch the track
  if(currentDirection == 1) {
    // Switch the track based on the current station
    switch(currentStation) {
      case 0:
        // Station 0 heading forward, dominant toggle is toggle 0
        if(digitalRead(TRACK_TOGGLE_0) == HIGH) {
          // Heading straight, ensure switch 0 and switch 1 are HIGH
          io.digitalWrite(TRACK_SWITCH_0, HIGH);
          io.digitalWrite(TRACK_SWITCH_1, HIGH);
          // 2 - 7 are don't care
          SerialUSB.println("Train at station 0, heading towards station 1");
          SerialUSB.println("TRACK_SWITCH_0 : HIGH, TRACK_SWITCH_1 : HIGH");
        } else {
          // Station 0 heading right, ensure switch 0 is LOW and switch 2 is LOW
          io.digitalWrite(TRACK_SWITCH_0, LOW);
          io.digitalWrite(TRACK_SWITCH_2, HIGH);
          // 1, 3 - 7 are don't care
          SerialUSB.println("Train at station 0, remainging on the same track");
          SerialUSB.println("TRACK_SWITCH_0 : LOW, TRACK_SWITCH_2 : HIGH");
        }
        break;
      case 1:
        // Station 1 heading forward, dominant toggle is toggle 1
        if(digitalRead(TRACK_TOGGLE_1) == HIGH) {
          // Heading straight, ensure switch 4 and switch 5 are HIGH
          io.digitalWrite(TRACK_SWITCH_4, HIGH);
          io.digitalWrite(TRACK_SWITCH_5, HIGH);
          // 0, 1, 2, 3, 6, 7 are don't care
          SerialUSB.println("Train at station 1, heading towards station 2");
          SerialUSB.println("TRACK_SWITCH_4 : HIGH, TRACK_SWITCH_5 : HIGH");
        } else {
          // Station 1 heading right, ensure switch 4 is LOW and switch 6 is LOW
          io.digitalWrite(TRACK_SWITCH_4, LOW);
          io.digitalWrite(TRACK_SWITCH_6, HIGH);
          // 0, 1, 2, 3, 5, 7 are don't care
          SerialUSB.println("Train at station 1, remainging on the same track");
          SerialUSB.println("TRACK_SWITCH_4 : LOW, TRACK_SWITCH_6 : HIGH");
        }
        break;
      case 2:
        // Station 2 heading forward, dominant toggle is toggle 2
        if(digitalRead(TRACK_TOGGLE_2) == HIGH) {
          // Heading straight, ensure switch 7 and switch 6 are HIGH
          io.digitalWrite(TRACK_SWITCH_7, HIGH);
          io.digitalWrite(TRACK_SWITCH_6, HIGH);
          // 0, 1, 2, 3, 4, 5 are don't care
          SerialUSB.println("Train at station 2, heading towards station 3");
          SerialUSB.println("TRACK_SWITCH_7 : HIGH, TRACK_SWITCH_6 : HIGH");
        } else {
          // Station 2 heading right, ensure switch 7 is LOW and switch 5 is LOW
          io.digitalWrite(TRACK_SWITCH_7, LOW);
          io.digitalWrite(TRACK_SWITCH_5, HIGH);
          // 0, 1, 2, 3, 4, 6 are don't care
          SerialUSB.println("Train at station 2, remainging on the same track");
          SerialUSB.println("TRACK_SWITCH_7 : LOW, TRACK_SWITCH_5 : HIGH");
        }
        break;
      case 3:
        // Station 3 heading forward, dominant toggle is toggle 3
        if(digitalRead(TRACK_TOGGLE_3) == HIGH) {
          // Heading straight, ensure switch 3 and switch 2 are HIGH
          io.digitalWrite(TRACK_SWITCH_3, HIGH);
          io.digitalWrite(TRACK_SWITCH_2, HIGH);
          // 0, 1, 4, 5, 6, 7 are don't care
          SerialUSB.println("Train at station 3, heading towards station 0");
          SerialUSB.println("TRACK_SWITCH_3 : HIGH, TRACK_SWITCH_2 : HIGH");
        } else {
          // Station 3 heading right, ensure switch 3 is LOW and switch 1 is LOW
          io.digitalWrite(TRACK_SWITCH_3, LOW);
          io.digitalWrite(TRACK_SWITCH_1, LOW);
          // 0, 2, 4, 5, 6, 7 are don't care
          SerialUSB.println("Train at station 3, remainging on the same track");
          SerialUSB.println("TRACK_SWITCH_3 : LOW, TRACK_SWITCH_1 : LOW");
        }
        break;
      default:
        // Do nothing
        break;
    }
  } else {
    // TODO: Handle train in reverse
  }
}

void updateTrainThrottle()
{
  TrainState.currentDirection = (digitalRead(DIRECTION) == HIGH) ? 1 : -1;
  TrainState.currentSpeed = (analogRead(THROTTLE_IN) * 100) / MAX_THROTTLE;

  // Apply the throttle
  if(TrainState.currentDirection == 1) {
    // Forward
    analogWrite(THROTTLE_OUT_1, (TrainState.currentSpeed * 255) / 100);
    analogWrite(THROTTLE_OUT_2, 0);
  } else {
    // Reverse
    analogWrite(THROTTLE_OUT_1, 0);
    analogWrite(THROTTLE_OUT_2, (TrainState.currentSpeed * 255) / 100);
  }
}

void updateTrackToggle()
{
  // Read the track toggle switches
  TrackState.trackToggle[0] = digitalRead(TRACK_TOGGLE_0);
  TrackState.trackToggle[1] = digitalRead(TRACK_TOGGLE_1);
  TrackState.trackToggle[2] = digitalRead(TRACK_TOGGLE_2);
  TrackState.trackToggle[3] = digitalRead(TRACK_TOGGLE_3);
}

void updateCurrentStation()
{
  // TODO: this should be done in an interrupt
}

void displaySystemState()
{
  // Print the Trainstate struct in JSON format
  SerialUSB.print("{");
  SerialUSB.print("\"currentStation\": ");
  SerialUSB.print(TrainState.currentStation);
  SerialUSB.print(", ");
  SerialUSB.print("\"currentDirection\": ");
  SerialUSB.print(TrainState.currentDirection);
  SerialUSB.print(", ");
  SerialUSB.print("\"currentSpeed\": ");
  SerialUSB.print(TrainState.currentSpeed);
  SerialUSB.print("}");
  SerialUSB.println();

  // Print the TrackState struct in JSON format
  SerialUSB.print("{");
  SerialUSB.print("\"trackToggle\": [");
  for (int i = 0; i < NUM_TRACK_TOGGLES; i++)
  {
    SerialUSB.print(TrackState.trackToggle[i]);
    if (i < NUM_TRACK_TOGGLES - 1)
    {
      SerialUSB.print(", ");
    }
  }
  SerialUSB.print("], ");
  SerialUSB.print("\"trackSwitch\": [");
  for (int i = 0; i < NUM_TRACK_SWITCHES; i++)
  {
    SerialUSB.print(TrackState.trackSwitch[i]);
    if (i < NUM_TRACK_SWITCHES - 1)
    {
      SerialUSB.print(", ");
    }
  }
  SerialUSB.print("]");
  SerialUSB.print("}");
  SerialUSB.println();

  // Print the station count array in JSON format
  SerialUSB.print("{");
  SerialUSB.print("\"stationCount\": [");
  for (int i = 0; i < NUM_TRACK_POSITIONS; i++)
  {
    SerialUSB.print(stationCount[i]);
    if (i < NUM_TRACK_POSITIONS - 1)
    {
      SerialUSB.print(", ");
    }
  }
  SerialUSB.print("]");
  SerialUSB.print("}");
  SerialUSB.println();
}

void testRelay()
{
  static int toggle = 0;
  // Loop through the relays one at a time and delay for 100 ms and switch their state
  for(int i = TRACK_SWITCH_0; i <= TRACK_SWITCH_7; i++) {
    io.digitalWrite(i, toggle);
    delay(100);
  }

  // Toggle the relay state
  toggle = !toggle;
}

void setup() {
  // Launch the serial port
  SerialUSB.begin(9600);
  SerialUSB.println("Hello, world!");

  // Set up throttle pins
  pinMode(THROTTLE_IN, INPUT);
  pinMode(DIRECTION, INPUT);

  // Set up track position pins as input pullup, TODO: register this to an ISR
  pinMode(TRACK_POS_0, INPUT_PULLUP);
  pinMode(TRACK_POS_1, INPUT_PULLUP);
  pinMode(TRACK_POS_2, INPUT_PULLUP);
  pinMode(TRACK_POS_3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TRACK_POS_0), station0ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(TRACK_POS_1), station1ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(TRACK_POS_2), station2ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(TRACK_POS_3), station3ISR, FALLING);

  // Set up track toggle switch pins as input pullup
  pinMode(TRACK_TOGGLE_0, INPUT_PULLUP);
  pinMode(TRACK_TOGGLE_1, INPUT_PULLUP);
  pinMode(TRACK_TOGGLE_2, INPUT_PULLUP);
  pinMode(TRACK_TOGGLE_3, INPUT_PULLUP);

  // Initialize the I2C bus
  Wire.begin();

  // Call io.begin(<address>) to initialize the SX1509. If it
  // successfully communicates, it'll return 1.
  if (io.begin(SX1509_ADDRESS) == false)
  {
    SerialUSB.println("Failed to communicate. Check wiring and address of SX1509.");
    while (1)
      ; // If we fail to communicate, loop forever.
  }

  // Set all of the SX1509 pins for the relay to outputs
  io.pinMode(TRACK_SWITCH_0, OUTPUT);
  io.pinMode(TRACK_SWITCH_1, OUTPUT);
  io.pinMode(TRACK_SWITCH_2, OUTPUT);
  io.pinMode(TRACK_SWITCH_3, OUTPUT);
  io.pinMode(TRACK_SWITCH_4, OUTPUT);
  io.pinMode(TRACK_SWITCH_5, OUTPUT);
  io.pinMode(TRACK_SWITCH_6, OUTPUT);
  io.pinMode(TRACK_SWITCH_7, OUTPUT);

  // Set all of the SX1509 pins for the relay to LOW
  io.digitalWrite(TRACK_SWITCH_0, LOW);
  io.digitalWrite(TRACK_SWITCH_1, LOW);
  io.digitalWrite(TRACK_SWITCH_2, LOW);
  io.digitalWrite(TRACK_SWITCH_3, LOW);
  io.digitalWrite(TRACK_SWITCH_4, LOW);
  io.digitalWrite(TRACK_SWITCH_5, LOW);
  io.digitalWrite(TRACK_SWITCH_6, LOW);
  io.digitalWrite(TRACK_SWITCH_7, LOW);
}

void loop() {
  // Every 250ms update the train and track state
  delay(250);
  updateTrainThrottle();
  updateTrackToggle();
  
  // Every second print the system state
  if (loopCounter % 4 == 0)
  {
    displaySystemState();
  }

  loopCounter++;
}
