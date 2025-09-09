// aBAJA Steer-by-Wire System Test Sketch

// --- Pin Definitions ---

// 1. Relay for Clutch
const int RELAY_PIN = A4; // Pin to control the relay transistor

// 2. Limit Switches
const int LEFT_LIMIT_PIN = 8;  // D8
const int RIGHT_LIMIT_PIN = 9; // D9

// 3. IBT-2 Motor Driver
const int MOTOR_RPWM_PIN = 10;
const int MOTOR_LPWM_PIN = 11;
const int MOTOR_REN_PIN = 13;
const int MOTOR_LEN_PIN = 12;

// 4. Absolute Encoder (ACE-128)
const int ENCODER_P1_PIN = 2;  // D2
const int ENCODER_P2_PIN = 3;  // D3
const int ENCODER_P3_PIN = 6;  // D6
const int ENCODER_P4_PIN = 7;  // D7
const int ENCODER_P5_PIN = A0;
const int ENCODER_P6_PIN = A1;
const int ENCODER_P7_PIN = A2;
const int ENCODER_P8_PIN = A3;

// Array to hold encoder pins for easy reading
const int encoderPins[8] = {
  ENCODER_P1_PIN, ENCODER_P2_PIN, ENCODER_P3_PIN, ENCODER_P4_PIN,
  ENCODER_P5_PIN, ENCODER_P6_PIN, ENCODER_P7_PIN, ENCODER_P8_PIN
};


void setup() {
  // Start serial communication for feedback
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor to open
  Serial.println("--- aBAJA Steer-by-Wire System Test ---");

  // --- Initialize Components ---

  // 1. Relay Setup
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with relay OFF
  Serial.println("Relay pin initialized as OUTPUT.");

  // 2. Limit Switch Setup
  pinMode(LEFT_LIMIT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_LIMIT_PIN, INPUT_PULLUP);
  Serial.println("Limit switch pins initialized as INPUT_PULLUP.");
  Serial.println("NOTE: Switches should read HIGH when not pressed, LOW when pressed.");

  // 3. Motor Driver Setup
  pinMode(MOTOR_RPWM_PIN, OUTPUT);
  pinMode(MOTOR_LPWM_PIN, OUTPUT);
  pinMode(MOTOR_REN_PIN, OUTPUT);
  pinMode(MOTOR_LEN_PIN, OUTPUT);
  // Disable motor initially for safety
  digitalWrite(MOTOR_REN_PIN, LOW);
  digitalWrite(MOTOR_LEN_PIN, LOW);
  Serial.println("Motor driver pins initialized as OUTPUT.");

  // 4. Encoder Setup
  for (int i = 0; i < 8; i++) {
    pinMode(encoderPins[i], INPUT); // Encoder uses external pull-ups
  }
  Serial.println("Encoder pins initialized as INPUT.");
  Serial.println("NOTE: Encoder requires external 4.7k pull-up resistors to 5V.");
  Serial.println("\n--- Starting Tests in 5 seconds ---");
  delay(5000);
}


void loop() {
  // --- Test 1: Relay ---
  Serial.println("\n--- Testing Relay ---");
  Serial.println("Turning Relay ON for 3 seconds...");
  digitalWrite(RELAY_PIN, HIGH); // Activate transistor, turn relay ON
  delay(3000);
  Serial.println("Turning Relay OFF for 3 seconds...");
  digitalWrite(RELAY_PIN, LOW);  // Deactivate transistor, turn relay OFF
  delay(3000);

  // --- Test 2: Limit Switches ---
  Serial.println("\n--- Testing Limit Switches (continuous for 10 seconds) ---");
  Serial.println("Press each switch to see its state change from 1 to 0.");
  for (int i = 0; i < 100; i++) {
    int leftState = digitalRead(LEFT_LIMIT_PIN);
    int rightState = digitalRead(RIGHT_LIMIT_PIN);
    Serial.print("Left Switch: ");
    Serial.print(leftState);
    Serial.print(" | Right Switch: ");
    Serial.println(rightState);
    delay(100);
  }

  // --- Test 3: Encoder ---
  Serial.println("\n--- Testing Encoder (continuous for 10 seconds) ---");
  Serial.println("Turn the steering mechanism to see the binary value change.");
  for (int i = 0; i < 100; i++) {
    Serial.print("Encoder Reading (P8..P1): ");
    String binaryValue = "";
    for (int j = 7; j >= 0; j--) { // Read from P8 down to P1
      binaryValue += digitalRead(encoderPins[j]);
    }
    Serial.println(binaryValue);
    delay(100);
  }

  // --- Test 4: Motor Driver ---
  Serial.println("\n--- Testing Motor Driver ---");
  Serial.println("WARNING: Motor will now move!");

  // Enable the driver
  digitalWrite(MOTOR_REN_PIN, HIGH);
  digitalWrite(MOTOR_LEN_PIN, HIGH);

  Serial.println("Moving motor FORWARD (half speed) for 2 seconds...");
  analogWrite(MOTOR_RPWM_PIN, 128); // 50% duty cycle
  analogWrite(MOTOR_LPWM_PIN, 0);
  delay(2000);

  Serial.println("Braking motor for 1 second...");
  analogWrite(MOTOR_RPWM_PIN, 0);
  analogWrite(MOTOR_LPWM_PIN, 0);
  delay(1000);

  Serial.println("Moving motor REVERSE (half speed) for 2 seconds...");
  analogWrite(MOTOR_RPWM_PIN, 0);
  analogWrite(MOTOR_LPWM_PIN, 128); // 50% duty cycle
  delay(2000);

  Serial.println("Braking motor for 1 second...");
  analogWrite(MOTOR_RPWM_PIN, 0);
  analogWrite(MOTOR_LPWM_PIN, 0);
  delay(1000);

  // Disable the driver for safety
  digitalWrite(MOTOR_REN_PIN, LOW);
  digitalWrite(MOTOR_LEN_PIN, LOW);

  Serial.println("\n--- Test Cycle Complete. Restarting in 10 seconds. ---");
  delay(10000);
}