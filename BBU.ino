#include <Arduino_CAN.h>

// CAN ID for Brake Status
const int BRAKE_STATUS_ID = 0x200;

void setup() {
  Serial.begin(115200);
  Serial.println("BBU Demo Node Initialized.");
  Serial.println("Enter a brake percentage (0-100)...");

  // Initialize CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
}

void loop() {
  // Check if the user has entered data in the Serial Monitor
  if (Serial.available() > 0) {
    int brakePercent = Serial.parseInt();

    // Ensure the input is within the 0-100 range
    brakePercent = constrain(brakePercent, 0, 100);

    // Map the 0-100 percentage to the 0-255 byte value for the CAN message
    byte brakeValue = map(brakePercent, 0, 100, 0, 255);

    // Send the CAN message
    CanMsg txMsg(BRAKE_STATUS_ID, 1, &brakeValue);
    CAN.write(txMsg);

    Serial.print("Sent Brake Status: ");
    Serial.print(brakePercent);
    Serial.print("% (Value: ");
    Serial.print(brakeValue);
    Serial.println(")");

    // Clear the serial buffer
    while (Serial.available()) {
      Serial.read();
    }
  }
}