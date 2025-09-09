#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

Servo panServo, tiltServo;
#define SERVO_PIN_X 4  // panServo
#define SERVO_PIN_Y 5  // tiltServo
#define DEBUG 1

volatile bool newDataAvailable = false;
// Structure to receive servo angles
struct Data_Packet {
  float pitch;
  float yaw;
};

Data_Packet myData;

void servoReset()
{
  panServo.write(20);
  tiltServo.write(90);
}

// Callback function for received data
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  //servoX.write(receivedData.angleX);
  //servoY.write(receivedData.angleY);
  newDataAvailable = true;
  if (DEBUG) {
    //Serial.print("Received X Angle: "); Serial.print(receivedData.angleX);
    //Serial.print(" | Y Angle: "); Serial.println(receivedData.angleY);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Ensure STA mode for ESPNOW

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  Serial.println("ESP-NOW Initialized");

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  // Attach servos to pins
  panServo.attach(SERVO_PIN_X, 600, 2400); //panServo
  tiltServo.attach(SERVO_PIN_Y, 600, 2400); //tiltServo
  servoReset();
  delay(3000);
}

void loop() {
  // Nothing needed in loop

  if (newDataAvailable) {
    // Process received data and move servos
    // Map and move servos based on pitch and yaw

    float angleX = constrain(myData.yaw, 1, 55);
    float angleY = constrain(myData.pitch, -90, 90);

    int panAngle = map(angleX, 1, 55, 20, 75);   // Adjust range as needed
    int tiltAngle = map(angleY, -90, 90, 180, 0); // Adjust range as needed

    panServo.write(panAngle);
    tiltServo.write(tiltAngle);

    newDataAvailable = false;
    if (DEBUG) {
      Serial.print("raw_X: ");
      Serial.print(myData.yaw);
      Serial.print(" | angle_X: ");
      Serial.print(angleX);
      Serial.print(" | Pan: ");
      Serial.print(panAngle);
      Serial.print(" | Tilt: ");
      Serial.println(tiltAngle);
    }
  }
}
