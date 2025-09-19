//  DASHBOARD UNIT


#include <Arduino_CAN.h>

// --- DBU Input Pin Definitions ---
// Using INPUT_PULLUP, so a pressed button connects the pin to GND.
const int START_BUTTON_PIN = 3;
// const int KILL_SWITCH_PIN = 3; // REMOVED: Handled electrically
const int AUTONOMOUS_SWITCH_PIN = 6;

// --- DBU Output Pin Definitions ---
const int LV_ON_LIGHT_PIN = 13;   // ADDED: LV Power Indicator
const int HV_ON_LIGHT_PIN = 12;
const int AUTONOMOUS_LIGHT_PIN = 11;
const int WARNING_LIGHT_PIN = 10;

// --- CAN IDs (as defined in our protocol) ---
const int DASHBOARD_INPUTS_ID = 0x210;
const int VEHICLE_STATE_ID = 0x300; // Listening for this ID from the SSU

// --- Vehicle State Machine Enum (must match SSU's) ---
enum VehicleState {
  IDLE,
  PRE_CHARGING,
  READY_TO_DRIVE,
  ERROR
};

void setup() {
  Serial.begin(115200);

  // Configure input pins with internal pull-up resistors
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(AUTONOMOUS_SWITCH_PIN, INPUT_PULLUP);

  // Configure output pins
  pinMode(LV_ON_LIGHT_PIN, OUTPUT); // ADDED
  pinMode(HV_ON_LIGHT_PIN, OUTPUT);
  pinMode(AUTONOMOUS_LIGHT_PIN, OUTPUT);
  pinMode(WARNING_LIGHT_PIN, OUTPUT);

  // Set initial output states to OFF
  digitalWrite(HV_ON_LIGHT_PIN, LOW);
  digitalWrite(AUTONOMOUS_LIGHT_PIN, LOW);
  digitalWrite(WARNING_LIGHT_PIN, LOW);

  // Initialize CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    digitalWrite(LV_ON_LIGHT_PIN, HIGH); // Still turn on LV light
    // Blink the warning light if CAN fails
    while (1) {
      digitalWrite(WARNING_LIGHT_PIN, !digitalRead(WARNING_LIGHT_PIN));
      delay(200);
    }
  }
  
  Serial.println("DBU CAN Node Initialized.");
  digitalWrite(LV_ON_LIGHT_PIN, HIGH); // ADDED: Turn on LV light on successful boot
}

void loop() {
  // 1. Broadcast the state of the dashboard's inputs periodically
  static unsigned long lastBroadcastTime = 0;
  if (millis() - lastBroadcastTime > 100) { // Send every 100ms
    broadcastInputs();
    lastBroadcastTime = millis();
  }

  // 2. Check for incoming messages (like the vehicle state from SSU)
  parseCANMessage();
}

void broadcastInputs() {
  // Read the current state of all switches.
  bool startButtonState = (digitalRead(START_BUTTON_PIN) == LOW);
  bool autonomousSwitchState = (digitalRead(AUTONOMOUS_SWITCH_PIN) == LOW);
  
  // Pack the boolean states into a single byte using bitmasking
  byte dataPayload = 0;
  if (startButtonState)     { dataPayload |= (1 << 0); } // Set bit 0
  // Bit 1 is now unused
  if (autonomousSwitchState){ dataPayload |= (1 << 2); } // Set bit 2

  // Send the CAN message
  CanMsg txMsg(DASHBOARD_INPUTS_ID, 1, &dataPayload);
  CAN.write(txMsg);
}

void parseCANMessage() {
  if (CAN.available()) {
    CanMsg rxMsg = CAN.read();

    if (rxMsg.id == VEHICLE_STATE_ID) {
      // The SSU is telling us the vehicle's state.
      VehicleState currentState = (VehicleState)rxMsg.data[0];

      // Update indicator lights based on vehicle state
      digitalWrite(HV_ON_LIGHT_PIN, (currentState == READY_TO_DRIVE));
      digitalWrite(WARNING_LIGHT_PIN, (currentState == ERROR));
    }
  }
}