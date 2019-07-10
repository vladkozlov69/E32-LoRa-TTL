#include <Arduino.h>
#include <SoftwareSerial.h>
#include "E32LoRaTTL.h"


#define M0_PIN	2
#define M1_PIN	3
#define AUX_PIN	8

#define SOFT_RX 10
#define SOFT_TX 11

#define DEVICE_A_ADDR_H 0x1A
#define DEVICE_A_ADDR_L 0xAA
#define DEVICE_B_ADDR_H 0x1A
#define DEVICE_B_ADDR_L 0xAB

SoftwareSerial softSerial(SOFT_RX, SOFT_TX);

E32LoRaTTL * lora = new E32LoRaTTL(M0_PIN, M1_PIN, AUX_PIN, &softSerial, &Serial);

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    softSerial.begin(9600);
    Serial.begin(9600);

    while (!Serial) 
    {
        ; // wait for serial port to connect. Needed for native USB
    }

    //self-check initialization.
    lora->waitReady();

    RET_STATUS status = lora->SetAddressAndChannel(DEVICE_B_ADDR_H, DEVICE_B_ADDR_L, AIR_CHAN_433M);
    
    if(status == RET_SUCCESS)
        Serial.println("Setup init OK!!");
}

void blinkLED()
{
    static bool LedStatus = LOW;

    digitalWrite(LED_BUILTIN, LedStatus);
    LedStatus = !LedStatus;
}

// The loop function is called in an endless loop
void loop()
{
    uint8_t data_buf[50];

    sprintf((char *)data_buf, "Hello [%lu]", millis());

    if(lora->SendMsg(DEVICE_A_ADDR_H, DEVICE_A_ADDR_L, AIR_CHAN_433M,
        (unsigned char *)data_buf, strlen((char *)data_buf))==RET_SUCCESS)
    {
        blinkLED();
    }

    delay(random(400, 600));
}