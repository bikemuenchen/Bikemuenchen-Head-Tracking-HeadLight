#include <Wire.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include <ESP8266WiFi.h>
#include <espnow.h>

#define DEBUG 0 // Debugging flag 1= ON, 0 = OFF

//uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x75, 0x40, 0x72}; // Replace with receiver MAC//
uint8_t broadcastAddress[] = {0x3C, 0x61, 0x05, 0xEE, 0xC5, 0x2A}; // run ESP_Get_MAC.ino program and open Serial Monitor on receiver ESP8266 to get MAC address


typedef struct Data_Package  {
  float yaw;
  float pitch;
} Data_Package ;

Data_Package  mpuData;


/* --- MPU6050 Setup --- */
MPU6050 mpu;
#define INTERRUPT_PIN 13 // D7 on NodeMCU
bool blinkState;
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint8_t fifoBuffer[64];
Quaternion q;
VectorFloat gravity;
float ypr[3];
volatile bool mpuInterrupt = false;
void ICACHE_RAM_ATTR dmpDataReady() {
  mpuInterrupt = true;
}

void resetData()
{
  mpuData.yaw = 0;
  mpuData.pitch = 0;
}

void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Packet sent to: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.print(" Status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // MPU6050 Initialization
  Wire.begin();
  Wire.setClock(400000);
  Serial.println(F("Initializing MPU6050..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);

  if (!mpu.testConnection()) {
    Serial.println(F("MPU6050 connection failed"));
    while (true);
  }
  Serial.println(F("MPU6050 connection successful"));

  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // Supply your gyro offsets here
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);

  if (devStatus == 0) {
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();
    Serial.println(F("DMP ready!"));
    dmpReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW initialized");

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  //esp_now_register_send_cb(onDataSent);

  if (esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0) != 0) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("Peer added successfully");
  resetData();
}

void loop() {
  if (!dmpReady) return;

  if (mpuInterrupt) {
    mpuInterrupt = false;

    if (mpu.getFIFOCount() >= 1024) {
      Serial.println("FIFO Overflow! Resetting...");
      mpu.resetFIFO();
      return;
    }
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

      mpuData.yaw = ypr[0] * 180 / M_PI;
      mpuData.pitch = ypr[2] * 180 / M_PI;

      esp_now_send(broadcastAddress, (uint8_t *) &mpuData, sizeof(mpuData));
    }

    if (DEBUG) {
      uint8_t result = esp_now_send(broadcastAddress, (uint8_t *) &mpuData, sizeof(mpuData));
      Serial.println(result == 0 ? "Data sent successfully" : "Error sending data");
      Serial.print("Yaw: "); Serial.print(mpuData.yaw);
      Serial.print(" Pitch: "); Serial.println(mpuData.pitch);
    }
  }
}
