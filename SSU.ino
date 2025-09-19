#include <Arduino_CAN.h>

// --- Pin Definitions ---
const int TSAL_PIN = 10;        // Pin for Tractive System Active Light
const int ASAL_PIN = 11;        // Pin for Autonomous System Active Light (Unused in this example)
const int RTDS_PIN = 12;        // Pin for Ready To Drive Sound
const int AIR_RELAY_PIN = 13;   // Pin to control the Accumulator Isolation Relays

// --- CAN IDs (as defined in our protocol) ---
const int BRAKE_STATUS_ID = 0x200;
const int THROTTLE_STATUS_ID = 0x201;
const int DASHBOARD_INPUTS_ID = 0x210;
const int VEHICLE_STATE_ID = 0x300;
const int FNR_STATE_ID = 0x301;  // Forward/Neutral/Reverse State from DBU

// --- Threshold Constants (replaces "magic numbers") ---
const int BRAKE_PRESSED_THRESHOLD = 25;   // Value above which brake is considered pressed (>10%)
const int ACCEL_PRESSED_THRESHOLD = 12;   // Value above which throttle is considered pressed (>5%)

// --- Vehicle State Machine ---
enum VehicleState {
  IDLE,
  READY_TO_DRIVE,
  ERROR
};
VehicleState currentState = IDLE;

// --- Input State Variables (to store latest data from CAN bus) ---
bool isBrakePressed = false;
bool isGearNeutral = false;
bool isAcceleratorPressed = false;

// --- Edge Detection for Start Button ---
bool isStartButtonPressed = false;
bool previousStartButtonState = false; // Store the last known state of the button

// --- Non-blocking Timer for RTDS ---
unsigned long rtdsStartTime = 0;
const long rtdsDuration = 1500; // Duration to play the sound in milliseconds

// --- ADDED: Monitoring and Debug Features ---
// Statistics tracking
unsigned long totalMessagesReceived = 0;
unsigned long lastStatsTime = 0;
bool debugModeEnabled = false;

// Message type counters for detailed stats
unsigned long brakeMessages = 0;
unsigned long throttleMessages = 0;
unsigned long dashboardMessages = 0;
unsigned long fnrMessages = 0;  // Added FNR message counter
unsigned long unknownMessages = 0;

// Function to get state name as string
const char* getStateName(VehicleState state) {
  switch (state) {
    case IDLE: return "IDLE";
    case READY_TO_DRIVE: return "READY_TO_DRIVE";
    case ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

// Function to handle CAN message monitoring display
void displayCanMessage(CanMsg& message) {
  if (!debugModeEnabled) return; // Only show if debug mode is on
  
  // Timestamp for better debugging
  Serial.print("[");
  Serial.print(millis());
  Serial.print("ms] ");
  
  Serial.print("CAN RX | ID: 0x");
  if (message.id < 0x10) Serial.print("0");
  if (message.id < 0x100) Serial.print("0");
  Serial.print(message.id, HEX);
  
  Serial.print(" | DLC: ");
  Serial.print(message.data_length);
  
  Serial.print(" | Data: ");
  for (int i = 0; i < message.data_length; i++) {
    if (message.data[i] < 0x10) Serial.print("0");
    Serial.print(message.data[i], HEX);
    Serial.print(" ");
  }
  
  // Decode known message types
  switch (message.id) {
    case BRAKE_STATUS_ID:
      Serial.print("| BRAKE: ");
      Serial.print(message.data[0]);
      Serial.print(" (");
      Serial.print(message.data[0] > BRAKE_PRESSED_THRESHOLD ? "PRESSED" : "RELEASED");
      Serial.print(")");
      break;
      
    case THROTTLE_STATUS_ID:
      if (message.data_length >= 2) {
        int throttlePercent = map(message.data[0], 0, 255, 0, 100);
        Serial.print("| THROTTLE: ");
        Serial.print(throttlePercent);
        Serial.print("%");
      }
      break;
      
    case FNR_STATE_ID:
      Serial.print("| FNR: ");
      Serial.print(message.data[0] == 0x00 ? "NEUTRAL" : "FORWARD");
      break;
      
    case DASHBOARD_INPUTS_ID:
      Serial.print("| START_BTN: ");
      Serial.print((message.data[0] & 0b00000001) ? "ON" : "OFF");
      Serial.print(", AUTO_SW: ");
      Serial.print((message.data[0] & 0b00000100) ? "ON" : "OFF");
      break;
      
    case VEHICLE_STATE_ID:
      Serial.print("| STATE: ");
      Serial.print(getStateName((VehicleState)message.data[0]));
      break;
  }
  
  Serial.println();
}

// Function to handle serial commands for debugging
void handleSerialCommands() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case 'd': { // Toggle debug mode
        debugModeEnabled = !debugModeEnabled;
        Serial.print("✓ Debug mode ");
        Serial.println(debugModeEnabled ? "ENABLED" : "DISABLED");
        break;
      }
      
      case 's': { // Show statistics
        Serial.println("=== SSU Node Statistics ===");
        Serial.print("Current State: ");
        Serial.println(getStateName(currentState));
        Serial.print("Total CAN messages received: ");
        Serial.println(totalMessagesReceived);
        Serial.print("- Brake messages: ");
        Serial.println(brakeMessages);
        Serial.print("- Throttle messages: ");
        Serial.println(throttleMessages);
        Serial.print("- Dashboard messages: ");
        Serial.println(dashboardMessages);
        Serial.print("- FNR messages: ");
        Serial.println(fnrMessages);
        Serial.print("- Unknown messages: ");
        Serial.println(unknownMessages);
        Serial.println();
        Serial.println("--- Current Input States ---");
        Serial.print("Brake Pressed: ");
        Serial.println(isBrakePressed ? "YES" : "NO");
        Serial.print("Gear in Neutral: ");
        Serial.println(isGearNeutral ? "YES" : "NO");
        Serial.print("Accelerator Pressed: ");
        Serial.println(isAcceleratorPressed ? "YES" : "NO");
        Serial.print("Start Button Pressed: ");
        Serial.println(isStartButtonPressed ? "YES" : "NO");
        Serial.println();
        Serial.println("--- Output States ---");
        Serial.print("AIR Relays: ");
        Serial.println(digitalRead(AIR_RELAY_PIN) ? "CLOSED" : "OPEN");
        Serial.print("TSAL: ");
        Serial.println(digitalRead(TSAL_PIN) ? "ON" : "OFF");
        Serial.print("RTDS: ");
        Serial.println(digitalRead(RTDS_PIN) ? "ON" : "OFF");
        Serial.print("ASAL: ");
        Serial.println(digitalRead(ASAL_PIN) ? "ON" : "OFF");
        Serial.print("Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
        Serial.println();
        break;
      }
      
      case 'r': { // Reset statistics
        totalMessagesReceived = 0;
        brakeMessages = 0;
        throttleMessages = 0;
        dashboardMessages = 0;
        fnrMessages = 0;
        unknownMessages = 0;
        lastStatsTime = millis();
        Serial.println("✓ Statistics reset");
        break;
      }
      
      case 'h': { // Help
        Serial.println("=== SSU Debug Commands ===");
        Serial.println("d         - Toggle debug mode (CAN message display)");
        Serial.println("s         - Show statistics and current states");
        Serial.println("r         - Reset statistics");
        Serial.println("h         - Show this help");
        Serial.println();
        break;
      }
      
      default:
        Serial.println("? Unknown command. Type 'h' for help.");
        break;
    }
    
    // Clear the serial buffer
    while (Serial.available()) {
      Serial.read();
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Configure output pins
  pinMode(AIR_RELAY_PIN, OUTPUT);
  pinMode(TSAL_PIN, OUTPUT);
  pinMode(RTDS_PIN, OUTPUT);
  pinMode(ASAL_PIN, OUTPUT); // If you use it

  // Set initial output states to OFF
  digitalWrite(AIR_RELAY_PIN, LOW);
  digitalWrite(TSAL_PIN, LOW);
  digitalWrite(RTDS_PIN, LOW);

  // Initialize CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("ERROR: Starting CAN failed!");
    Serial.println("Check wiring and CAN transceiver");
    currentState = ERROR;
    // The loop will handle being in the ERROR state
  } else {
    Serial.println("=== SSU CAN Node Initialized ===");
    Serial.println("Features:");
    Serial.println("- Vehicle state management");
    Serial.println("- CAN bus monitoring (when debug enabled)");
    Serial.println("- Statistics tracking");
    Serial.println();
    Serial.println("Debug Commands: Type 'h' for help");
    Serial.println("Current State: IDLE");
    Serial.println();
  }
  
  lastStatsTime = millis();
}

void loop() {
  // 1. Handle debug commands from serial
  handleSerialCommands();

  // 2. Check for and parse incoming CAN messages
  parseCANMessage();

  // 3. Update the vehicle's state machine based on inputs
  updateVehicleState();

  // 4. Control outputs based on the current state
  controlOutputs();
  
  // 5. Broadcast the vehicle's state periodically (non-blocking)
  static unsigned long lastStateBroadcast = 0;
  if (millis() - lastStateBroadcast > 200) { // Send state every 200ms
    broadcastVehicleState();
    lastStateBroadcast = millis();
  }
  
  // 6. Periodic statistics summary (every 60 seconds)
  if (millis() - lastStatsTime > 60000) {
    Serial.print("[INFO] Messages received in last 60s: ");
    Serial.print(totalMessagesReceived);
    Serial.print(" | State: ");
    Serial.println(getStateName(currentState));
    lastStatsTime = millis();
  }
}

// ENHANCED FUNCTION with monitoring
void parseCANMessage() {
  // Check if a message is available
  if (CAN.available()) {
    // Get the message
    CanMsg rxMsg = CAN.read();
    
    // Update statistics
    totalMessagesReceived++;
    
    // Display message if debug mode is enabled
    displayCanMessage(rxMsg);

    // Process the message (original logic unchanged)
    switch (rxMsg.id) {
      case BRAKE_STATUS_ID:
        brakeMessages++; // Update counter
        isBrakePressed = (rxMsg.data[0] > BRAKE_PRESSED_THRESHOLD); 
        break;

      case THROTTLE_STATUS_ID:
        throttleMessages++; // Update counter
        isAcceleratorPressed = (rxMsg.data[0] > ACCEL_PRESSED_THRESHOLD);
        // Removed gear reading from throttle message - now using FNR_STATE
        break;

      case FNR_STATE_ID:
        fnrMessages++; // Update counter
        isGearNeutral = (rxMsg.data[0] == 0x00); // 0x00 = Neutral, 0x01 = Forward
        if (debugModeEnabled) {
          Serial.print("FNR State received: ");
          Serial.println(rxMsg.data[0] == 0x00 ? "NEUTRAL" : "FORWARD");
        }
        break;

      case DASHBOARD_INPUTS_ID: {
        dashboardMessages++; // Update counter
        previousStartButtonState = isStartButtonPressed;       // Store previous state
        isStartButtonPressed = (rxMsg.data[0] & 0b00000001);   // Update current state for start button

        // Read the autonomous switch state from Bit 2 of the same message
        bool isAutonomousSwitchOn = (rxMsg.data[0] & 0b00000100);
        digitalWrite(ASAL_PIN, isAutonomousSwitchOn); // Control the ASAL pin directly
        break;
      }
        
      default:
        unknownMessages++; // Track unknown message types
        break;
    }
  }
}

void updateVehicleState() {
  // Check for a rising edge on the start button (it was just pressed)
  bool startButtonJustPressed = isStartButtonPressed && !previousStartButtonState;
  
  // Store previous state for change detection
  static VehicleState previousState = IDLE;

  switch (currentState) {
    case IDLE:
      // Log condition check when start button is pressed
      if (startButtonJustPressed) {
        Serial.println("=== START BUTTON PRESSED - Checking Conditions ===");
        Serial.print("Brake Pressed: ");
        Serial.println(isBrakePressed ? "✓ YES" : "✗ NO - BRAKE PEDAL REQUIRED");
        Serial.print("Gear in Neutral: ");
        Serial.println(isGearNeutral ? "✓ YES" : "✗ NO - NEUTRAL GEAR REQUIRED");
        Serial.print("Accelerator Released: ");
        Serial.println(!isAcceleratorPressed ? "✓ YES" : "✗ NO - RELEASE ACCELERATOR");
        Serial.print("Start Button Rising Edge: ");
        Serial.println(startButtonJustPressed ? "✓ YES" : "✗ NO");
        Serial.println();
      }
      
      // Check all conditions from the flowchart to enter Ready to Drive mode
      if (isBrakePressed && isGearNeutral && !isAcceleratorPressed && startButtonJustPressed) {
        Serial.println("✓ ALL CONDITIONS MET. Entering READY_TO_DRIVE mode.");
        currentState = READY_TO_DRIVE;
        
        // Start the non-blocking Ready-to-Drive sound
        rtdsStartTime = millis();
        digitalWrite(RTDS_PIN, HIGH);
      } else if (startButtonJustPressed) {
        Serial.println("✗ CONDITIONS NOT MET. Remaining in IDLE mode.");
        Serial.println();
      }
      break;

    case READY_TO_DRIVE:
      // Example logic to exit Ready to Drive mode
      if (startButtonJustPressed) {
          Serial.println("✓ Shutdown condition met. Returning to IDLE mode.");
          currentState = IDLE;
      }
      break;
      
    case ERROR:
      // The system will stay in this state until reset.
      Serial.println("! System in ERROR state. Reset required.");
      break;
  }
  
  // Log state changes
  if (currentState != previousState) {
    Serial.print("STATE CHANGE: ");
    Serial.print(getStateName(previousState));
    Serial.print(" -> ");
    Serial.println(getStateName(currentState));
    previousState = currentState;
  }
}

void controlOutputs() {
  // Handle the non-blocking timer for the RTDS sound
  if (rtdsStartTime > 0 && millis() - rtdsStartTime >= rtdsDuration) {
    digitalWrite(RTDS_PIN, LOW); // Turn off sound
    rtdsStartTime = 0;           // Reset the timer
    Serial.println("RTDS sound completed");
  }

  switch (currentState) {
    case IDLE:
      digitalWrite(AIR_RELAY_PIN, LOW); // Open AIRs
      digitalWrite(TSAL_PIN, LOW);      // Turn off TSAL
      break;

    case READY_TO_DRIVE:
      digitalWrite(AIR_RELAY_PIN, HIGH); // Close AIRs
      digitalWrite(TSAL_PIN, HIGH);      // Turn on TSAL
      break;

    case ERROR:
      // Ensure all critical systems are off in an error state
      digitalWrite(AIR_RELAY_PIN, LOW);
      digitalWrite(TSAL_PIN, LOW);
      digitalWrite(RTDS_PIN, LOW);
      break;
  }
}

void broadcastVehicleState() {
  // Create a 1-byte buffer for our data
  uint8_t msgData[1];
  msgData[0] = currentState;

  // Use the CanMsg constructor to build the message.
  CanMsg txMsg(VEHICLE_STATE_ID, 1, msgData);

  if (CAN.write(txMsg)) {
    // Optionally log successful transmission in debug mode
    if (debugModeEnabled) {
      Serial.print("[");
      Serial.print(millis());
      Serial.print("ms] CAN TX | ID: 0x");
      Serial.print(VEHICLE_STATE_ID, HEX);
      Serial.print(" | State: ");
      Serial.println(getStateName(currentState));
    }
  } else {
    Serial.println("ERROR: Failed to send vehicle state");
  }
}