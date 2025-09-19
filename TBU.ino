// THROTTLE BY WIRE UNIT

#include <Arduino_CAN.h>

// CAN ID for Throttle Status
const int THROTTLE_STATUS_ID = 0x201;

// Global variables to hold the current state
byte throttleValue = 0;
byte gearState = 0; // 0 = Neutral, 1 = In-Gear

// Statistics tracking
unsigned long totalMessagesReceived = 0;
unsigned long lastStatsTime = 0;

// Function to handle received CAN messages
void processCanMessage(CanMsg& message) {
  totalMessagesReceived++;
  
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
  if (message.id == THROTTLE_STATUS_ID) {
    Serial.print("| THROTTLE: ");
    if (message.data_length >= 2) {
      int throttlePercent = map(message.data[0], 0, 255, 0, 100);
      Serial.print(throttlePercent);
      Serial.print("%, GEAR: ");
      Serial.print(message.data[1] == 0 ? "N" : "D");
    }
  }
  
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("=== Enhanced TBU Demo Node ===");
  Serial.println("Features:");
  Serial.println("- CAN Bus Monitor with timestamps");
  Serial.println("- Throttle/Gear status transmission");
  Serial.println("- Message statistics");
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  t<0-100>  - Set throttle percentage");
  Serial.println("  g<0/1>    - Set gear (0=Neutral, 1=In-Gear)");
  Serial.println("  s         - Show statistics");
  Serial.println("  r         - Reset statistics");
  Serial.println("  h         - Show this help");
  Serial.println();

  // Initialize CAN bus at 500 kbps with error checking
  if (!CAN.begin(500E3)) {
    Serial.println("ERROR: Starting CAN failed!");
    Serial.println("Check wiring and CAN transceiver");
    while (1) {
      delay(1000);
      Serial.print(".");
    }
  }
  
  Serial.println("CAN Bus initialized at 500 kbps");
  
  lastStatsTime = millis();
  Serial.println("Ready for commands...");
}

void loop() {
  // 1. Check for incoming CAN messages using available() and read()
  if (CAN.available()) {
    CanMsg rxMsg = CAN.read();
    processCanMessage(rxMsg);
  }

  // 2. Handle serial commands with improved parsing
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case 't': { // Throttle command
        int throttlePercent = Serial.parseInt();
        throttlePercent = constrain(throttlePercent, 0, 100);
        throttleValue = map(throttlePercent, 0, 100, 0, 255);
        Serial.print("✓ Throttle set to: ");
        Serial.print(throttlePercent);
        Serial.println("%");
        break;
      }
      
      case 'g': { // Gear command
        int gearInput = Serial.parseInt();
        gearState = (gearInput == 0) ? 0 : 1;
        Serial.print("✓ Gear set to: ");
        Serial.println(gearState == 0 ? "Neutral" : "In-Gear");
        break;
      }
      
      case 's': { // Show statistics
        Serial.println("=== CAN Bus Statistics ===");
        Serial.print("Total messages received: ");
        Serial.println(totalMessagesReceived);
        Serial.print("Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
        Serial.print("Current throttle: ");
        Serial.print(map(throttleValue, 0, 255, 0, 100));
        Serial.println("%");
        Serial.print("Current gear: ");
        Serial.println(gearState == 0 ? "Neutral" : "In-Gear");
        Serial.println();
        break;
      }
      
      case 'r': { // Reset statistics
        totalMessagesReceived = 0;
        lastStatsTime = millis();
        Serial.println("✓ Statistics reset");
        break;
      }
      
      case 'h': { // Help
        Serial.println("=== Command Help ===");
        Serial.println("t<0-100>  - Set throttle (e.g., t75 for 75%)");
        Serial.println("g<0/1>    - Set gear (g0=Neutral, g1=In-Gear)");
        Serial.println("s         - Show statistics");
        Serial.println("r         - Reset statistics");
        Serial.println("h         - Show this help");
        Serial.println();
        break;
      }
      
      default:
        Serial.println("? Unknown command. Type 'h' for help.");
        break;
    }
    
    // Clear the serial buffer of any remaining characters
    while (Serial.available()) {
      Serial.read();
    }
  }

  // 3. Broadcast the current state over CAN periodically
  static unsigned long lastBroadcastTime = 0;
  const unsigned long BROADCAST_INTERVAL = 100; // 100ms = 10Hz
  
  if (millis() - lastBroadcastTime >= BROADCAST_INTERVAL) {
    // Create CAN message with data payload
    byte dataPayload[2];
    dataPayload[0] = throttleValue;
    dataPayload[1] = gearState;
    
    // Create CanMsg object and send it
    CanMsg txMsg(THROTTLE_STATUS_ID, 2, dataPayload);
    
    if (CAN.write(txMsg)) {
      // Success - uncomment for TX confirmation
      // Serial.println("TX: Throttle status sent");
    } else {
      Serial.println("ERROR: Failed to send CAN message");
    }
    
    lastBroadcastTime = millis();
  }

  // 4. Periodic statistics display (every 30 seconds)
  if (millis() - lastStatsTime > 30000) {
    Serial.print("[INFO] Messages received in last 30s: ");
    Serial.println(totalMessagesReceived);
    lastStatsTime = millis();
  }
}