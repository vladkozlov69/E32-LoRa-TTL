#include <Arduino.h>

#include "E32LoRaTTL.h"


#define M0_PIN	3
#define M1_PIN	4
#define AUX_PIN	8

#define DEVICE_A_ADDR_H 0x1A
#define DEVICE_A_ADDR_L 0xAA

E32LoRaTTL * lora = new E32LoRaTTL(M0_PIN, M1_PIN, AUX_PIN, &Serial1, &Serial);


//The setup function is called once at startup of the sketch
void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial1.begin(9600);
    Serial.begin(9600);

    while (!Serial) 
    {
        ; // wait for serial port to connect. Needed for native USB
    }

    //self-check initialization.
    lora->waitReady();

    RET_STATUS status = lora->SetAddressAndChannel(DEVICE_A_ADDR_H, DEVICE_A_ADDR_L, AIR_CHAN_433M);
    
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
    uint8_t data_buf[100], data_len;

    if(lora->ReceiveMsg(data_buf, &data_len)==RET_SUCCESS)
    {
        Serial.print("ReceiveMsg: ");  Serial.print(data_len);  Serial.println(" bytes.");
        Serial.println((char*)data_buf);
        blinkLED();
    }
}