#ifndef __LORA_H__
#define __LORA_H__
#include <stm32f1xx.h>
#include <time.h>
#include <SPI.h>


#define LORA_DEFAULT_NSS_PORT    *GPIOA // A4
#define LORA_DEFAULT_RESET_PORT  *GPIOB // B0
#define LORA_DEFAULT_DIO0_PORT   *GPIOB // B1
#define LORA_DEFAULT_NSS_PIN     4 // A4
#define LORA_DEFAULT_RESET_PIN   0 // B0
#define LORA_DEFAULT_DIO0_PIN    1 // B1

#define PA_OUTPUT_PA_BOOST_PIN  1
#define PA_OUTPUT_RFO_PIN       0

/*!
 * RegPaConfig
 */
#define RF_PACONFIG_PASELECT_MASK                   0x7F
#define RF_PACONFIG_PASELECT_PABOOST                0x80
#define RF_PACONFIG_PASELECT_RFO                    0x00 // Default

#define RF_PACONFIG_MAX_POWER_MASK                  0x8F

#define RF_PACONFIG_OUTPUTPOWER_MASK                0xF0

/*!
 * RegPaDac
 */
#define RF_PADAC_20DBM_MASK                         0xF8
#define RF_PADAC_20DBM_ON                           0x07
#define RF_PADAC_20DBM_OFF                          0x04  // Default

struct LoRaClass {
//   SPISettings _spiSettings;
  GPIO_TypeDef _nss_port;
  GPIO_TypeDef _reset_port;
  GPIO_TypeDef _dio0_port;
  uint32_t _nss_pin;
  uint32_t _reset_pin;
  uint32_t _dio0_pin;

  int _frequency;
  int _packetIndex;
  int _implicitHeaderMode;
  void (*_onReceive)(int);
};

//    ----- -----  ----- -----
// ----- ----- Public ----- -----
//    ----- -----  ----- -----
void nss_delay(struct LoRaClass *LoRa, uint8_t ms);
void nss_delay_k(struct LoRaClass *LoRa, uint8_t mks);

void LoRaClass_init(struct LoRaClass *LoRa);

int begin(struct LoRaClass *LoRa, long frequency, uint8_t PABOOST);
void end(struct LoRaClass *LoRa);

// int beginPacket(struct LoRaClass *LoRa, int implicitHeader = false);
int beginPacket(struct LoRaClass *LoRa, int implicitHeader);
// int endPacket(struct LoRaClass *LoRa, bool async = false);
int endPacket(struct LoRaClass *LoRa, uint8_t async);

// int parsePacket(struct LoRaClass *LoRa, int size = 0);
int parsePacket(struct LoRaClass *LoRa, int size);
int packetRssi(struct LoRaClass *LoRa);
float packetSnr(struct LoRaClass *LoRa);

// from Print
uint32_t write(struct LoRaClass *LoRa, uint8_t byte);
uint32_t _write(struct LoRaClass *LoRa, const uint8_t *buffer, uint32_t size);

// from Stream
int available(struct LoRaClass *LoRa);
int read(struct LoRaClass *LoRa);
int peek(struct LoRaClass *LoRa);
void flush(struct LoRaClass *LoRa);


// void receive(struct LoRaClass *LoRa, int size = 0);
void receive(struct LoRaClass *LoRa, int size);
void idle(struct LoRaClass *LoRa);
void sleep(struct LoRaClass *LoRa);

void setTxPower(struct LoRaClass *LoRa, int8_t power, int8_t outputPin);
void setTxPowerMax(struct LoRaClass *LoRa, int level);
void setFrequency(struct LoRaClass *LoRa, long frequency);
void setSpreadingFactor(struct LoRaClass *LoRa, int sf);
void setSignalBandwidth(struct LoRaClass *LoRa, long sbw);
void setCodingRate4(struct LoRaClass *LoRa, int denominator);
void setPreambleLength(struct LoRaClass *LoRa, long length);
void setSyncWord(struct LoRaClass *LoRa, int sw);
void enable_crc(struct LoRaClass *LoRa);
void disable_crc(struct LoRaClass *LoRa);

// deprecated
void crc(struct LoRaClass *LoRa);
void no_crc(struct LoRaClass *LoRa);

uint8_t random(struct LoRaClass *LoRa);

// void setPins(struct LoRaClass *LoRa, int ss = LORA_DEFAULT_SS_PIN, int reset = LORA_DEFAULT_RESET_PIN, int dio0 = LORA_DEFAULT_DIO0_PIN);
void setPins(struct LoRaClass *LoRa, GPIO_TypeDef nss_port, GPIO_TypeDef reset_port, GPIO_TypeDef dio0_port, 
             int nss_pin, int reset_pin, int dio0_pin);
void setSPIFrequency(struct LoRaClass *LoRa, uint32_t frequency);

// void dumpRegisters(struct LoRaClass *LoRa, Stream& out);

//    ----- ----- - ----- -----
// ----- ----- Private ----- -----
//    ----- ----- - ----- -----

void __explicitHeaderMode(struct LoRaClass *LoRa);
void __implicitHeaderMode(struct LoRaClass *LoRa);

uint8_t __readRegister(struct LoRaClass *LoRa, uint8_t address);
void __writeRegister(struct LoRaClass *LoRa, uint8_t address, uint8_t value);
uint8_t __singleTransfer(struct LoRaClass *LoRa, uint8_t address, uint8_t value);


#endif