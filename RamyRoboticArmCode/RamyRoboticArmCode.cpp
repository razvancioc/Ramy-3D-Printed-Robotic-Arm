/**
 * @file main.cpp
 * @brief Control software for a 6-DOF Robotic Arm using ESP32 and PCA9685.
 * * This program reads analog inputs from 3 dual-axis joysticks and translates 
 * them into smooth, incremental movements for 6 servo motors. It utilizes the 
 * PCA9685 PWM driver module via I2C to offload the PWM generation from the ESP32.
 * * @author [Numele Tau / ARTTU Formula Student]
 * @date 4 April 2026
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

/** * @brief Initialize the PCA9685 driver with the default I2C address (0x40).
 */
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

/** @brief PWM pulse length for 0 degrees (varies by servo brand, usually 100-150 out of 4096) */
#define SERVO_MIN 125 

/** @brief PWM pulse length for 180 degrees (varies by servo brand, usually 500-600 out of 4096) */
#define SERVO_MAX 575 

/** @brief Total number of servos in the robotic arm */
#define SERVO_COUNT 6

/** @brief PCA9685 channels connected to the servos */
const int servoPorts[SERVO_COUNT] = {0, 1, 2, 3, 4, 5};

/** * @brief ESP32 Analog Input Pins for the 3 joysticks (X and Y axes).
 * @note ESP32 ADC is 12-bit, meaning joystick values will range from 0 to 4095.
 */
const int joyPins[SERVO_COUNT] = {32, 33, 34, 35, 36, 39};

/** @brief Stores the current angle for each servo to allow incremental movement */
float currentAngles[SERVO_COUNT] = {90.0, 90.0, 90.0, 90.0, 90.0, 90.0};

/** @brief Degrees to move per loop iteration when the joystick is pushed */
float armSpeed = 1.5; 

/* * Joystick Deadzone Settings
 * Since physical joysticks rarely center perfectly at 2048, we create a "deadzone" 
 * in the middle where no movement occurs. This prevents the servos from jittering 
 * when nobody is touching the joysticks.
 */
/** @brief Lower threshold of the joystick deadzone */
const int deadzoneLow = 1750;
/** @brief Upper threshold of the joystick deadzone */
const int deadzoneHigh = 2050;

/**
 * @brief Initial configuration of serial communication, I2C, and motor home positions.
 */
void setup() {
  Serial.begin(115200);
  
  // Initialize Joystick Pins as inputs
  for(int i = 0; i < SERVO_COUNT; i++) {
    pinMode(joyPins[i], INPUT);
  }

  // Initialize I2C communication on specific ESP32 pins (SDA = 21, SCL = 22)
  Wire.begin(21, 22);
  
  // Start the PCA9685 PWM driver
  pwm.begin();
  
  // Analog servos typically run at ~50 Hz updates
  pwm.setPWMFreq(50);
  
  Serial.println("Initializing Servos to 90 degrees...");
  
  // Move all servos to their 90-degree home position safely on startup
  for(int i = 0; i < SERVO_COUNT; i++) {
    // Map the 90-degree angle to the corresponding PWM pulse length
    int startPulse = map(90, 0, 180, SERVO_MIN, SERVO_MAX);
    
    // Send the pulse length to the specific port on the PCA9685
    pwm.setPWM(servoPorts[i], 0, startPulse);
  }
  
  delay(1000);
  Serial.println("Robot Arm Ready!");
}

/**
 * @brief Main execution loop. Reads inputs, calculates new angles, and updates servos.
 */
void loop() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    // Read the 12-bit value from the joystick (0 to 4095)
    int joyValue = analogRead(joyPins[i]);

    // Incremental Logic: Move the arm only if the joystick is pushed past the deadzone
    if (joyValue > deadzoneHigh) {
      currentAngles[i] += armSpeed;
    } 
    else if (joyValue < deadzoneLow) {
      currentAngles[i] -= armSpeed;
    }

    // Constraint: Prevent the math from telling the servo to move past its physical limits
    if (currentAngles[i] < 0)   currentAngles[i] = 0;
    if (currentAngles[i] > 180) currentAngles[i] = 180;

    // Convert the angle (0-180) into a pulse length (125-575) for the PCA9685 chip
    int pulseLength = map(currentAngles[i], 0, 180, SERVO_MIN, SERVO_MAX);
    
    // Command the PCA9685 to adjust the specific servo
    pwm.setPWM(servoPorts[i], 0, pulseLength);
  }

  // Debugging snippet: Uncomment to monitor the first joystick's behavior
  /*
  Serial.print("J1_X: "); Serial.print(analogRead(joyPins[0]));
  Serial.print(" | A1: "); Serial.println(currentAngles[0]);
  */

  // Small delay to make the movement smooth and allow servos to catch up physically
  delay(15); 
}