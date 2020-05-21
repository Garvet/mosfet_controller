#include "LoRa.h"

// registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_LR_OCP				 0X0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_PKT_SNR_VALUE        0x1b
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PaDac				 0x4d //add REG_PaDac

// modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06

// PA config
//#define PA_BOOST                 0x80
//#define RFO                      0x70
// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40


#define MAX_PKT_LENGTH             255

void LoRaClass_init(struct LoRaClass *LoRa) {
    // _spiSettings(8E6, MSBFIRST, SPI_MODE0);
    LoRa->_nss_port = LORA_DEFAULT_NSS_PORT;
    LoRa->_reset_port = LORA_DEFAULT_RESET_PORT;
    LoRa->_dio0_port = LORA_DEFAULT_DIO0_PORT;
    LoRa->_nss_pin = LORA_DEFAULT_NSS_PIN;
    LoRa->_reset_pin = LORA_DEFAULT_RESET_PIN;
    LoRa->_dio0_pin = LORA_DEFAULT_DIO0_PIN;
    LoRa->_frequency = 0;
    LoRa->_packetIndex = 0;
    LoRa->_implicitHeaderMode = 0;
    LoRa->_onReceive = 0; // NULL


    // overide Stream timeout value
    // setTimeout(0);
}

void nss_delay(struct LoRaClass *LoRa, uint8_t ms) {
    ms_delay(ms);
    GPIOA->ODR |= GPIO_ODR_ODR4; // off
    // SPI1->CR1 |= SPI_CR1_SSI;
    ms_delay(ms);
    GPIOA->ODR &= ~GPIO_ODR_ODR4; // on
    // SPI1->CR1 &= ~SPI_CR1_SSI;
    ms_delay(ms);
}

void nss_delay_k(struct LoRaClass *LoRa, uint8_t mks) {
    mks_delay(mks);
    GPIOA->ODR |= GPIO_ODR_ODR4; // off
    // SPI1->CR1 |= SPI_CR1_SSI;
    mks_delay(mks);
    GPIOA->ODR &= ~GPIO_ODR_ODR4; // on
    // SPI1->CR1 &= ~SPI_CR1_SSI;
    mks_delay(mks);
}

int begin(struct LoRaClass *LoRa, long frequency, uint8_t PABOOST) {
    // setup pins
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    //Для начала сбрасываем все конфигурационные биты в нули
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    GPIOB->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1);
    //Настраиваем 

    //NSS:  CNF4 = 0x00 (00b) и MODE4 = 0x03 (11b) - режим Output push-pull
    GPIOA->CRL |= GPIO_CRL_MODE4_1 | GPIO_CRL_MODE4_0;
    GPIOA->ODR &= ~GPIO_ODR_ODR4;
    //Reset:  CNF0 = 0x00 (00b) и MODE0 = 0x03 (11b) - режим Output push-pull
    GPIOB->CRL |= GPIO_CRL_MODE0_1 | GPIO_CRL_MODE0_0;
    //DIO0:  CNF0 = 0x02 (10b) и MODE0 = 0x00 (00b) - режим Input pull-down
    GPIOB->CRL |= GPIO_CRL_CNF1_1;
    GPIOB->ODR &= ~GPIO_ODR_ODR1;

    // pinMode(_ss, OUTPUT);    // -----
    // pinMode(_reset, OUTPUT); // -----
    // pinMode(_dio0, INPUT);   // -----

    // perform reset
    GPIOB->ODR &= ~GPIO_ODR_ODR0;
    ms_delay(50);
    GPIOB->ODR |= GPIO_ODR_ODR0;
    ms_delay(100);
    // digitalWrite(_reset, LOW);  // -----
    // delay(20); // -----
    // digitalWrite(_reset, HIGH); // -----
    // delay(50); // -----
    
    // set NSS high (off)
    GPIOA->ODR |= GPIO_ODR_ODR4;
    // digitalWrite(_ss, HIGH); // -----
    // start SPI
    SPI1_init(0);
    // SPI.begin(); // -----
    // set NSS low (on)
    nss_delay(LoRa, 1);

    // check version
    uint8_t version = __readRegister(LoRa, REG_VERSION);
    if (version != 0x12) { 
        nss_delay(LoRa, 10);
        return 0; 
    }

    nss_delay(LoRa, 1);

    // put in sleep mode
    sleep(LoRa);

    nss_delay(LoRa, 1);

    // set frequency
    setFrequency(LoRa, frequency);

    nss_delay(LoRa, 1);
    // set base addresses
    __writeRegister(LoRa, REG_FIFO_TX_BASE_ADDR, 0);
    __writeRegister(LoRa, REG_FIFO_RX_BASE_ADDR, 0);
    // set LNA boost
    __writeRegister(LoRa, REG_LNA, __readRegister(LoRa, REG_LNA) | 0x03);
    // set auto AGC
    __writeRegister(LoRa, REG_MODEM_CONFIG_3, 0x04);
    // set output power to 14 dBm
    if(PABOOST != 0)
        setTxPower(LoRa, 14, RF_PACONFIG_PASELECT_PABOOST);
    else
        setTxPower(LoRa, 14, RF_PACONFIG_PASELECT_RFO);
    setSpreadingFactor(LoRa, 11);
    // put in standby mode
    setSignalBandwidth(LoRa, 125E3);
    //setCodingRate4(LoRa, 5);
    setSyncWord(LoRa, 0x34);
    disable_crc(LoRa);
    crc(LoRa);
    idle(LoRa);
    return 1;
}

void end(struct LoRaClass *LoRa) {
    // put in sleep mode
    sleep(LoRa);
    // stop SPI
    // SPI.end(); // ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- -----
}

int beginPacket(struct LoRaClass *LoRa, int implicitHeader) {
    // put in standby mode
    idle(LoRa);
    if (implicitHeader) {
        __implicitHeaderMode(LoRa);
    } else {
        __explicitHeaderMode(LoRa);
    }
    // reset FIFO address and paload length
    __writeRegister(LoRa, REG_FIFO_ADDR_PTR, 0);
    __writeRegister(LoRa, REG_PAYLOAD_LENGTH, 0);
    
    return 1;
}

int endPacket(struct LoRaClass *LoRa, uint8_t async) {
    // put in TX mode
    __writeRegister(LoRa, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

    if (async != 0) {
        // grace time is required for the radio
        mks_delay(150);
        // delayMicroseconds(150); // -----
    } else {
        // wait for TX done
        while ((__readRegister(LoRa, REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) {
            // yield(); // ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- -----
        }
        // clear IRQ's
        __writeRegister(LoRa, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
    }

    return 1;
}



int parsePacket(struct LoRaClass *LoRa, int size) {
    int packetLength = 0;
    int irqFlags = __readRegister(LoRa, REG_IRQ_FLAGS);

    if (size > 0) {
        __implicitHeaderMode(LoRa);
        __writeRegister(LoRa, REG_PAYLOAD_LENGTH, size & 0xff);
    } else {
        __explicitHeaderMode(LoRa);
    }

    // clear IRQ's
    __writeRegister(LoRa, REG_IRQ_FLAGS, irqFlags);

    if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
        // received a packet
        LoRa->_packetIndex = 0;
        // read packet length
        if (LoRa->_implicitHeaderMode) {
        packetLength = __readRegister(LoRa, REG_PAYLOAD_LENGTH);
        } else {
        packetLength = __readRegister(LoRa, REG_RX_NB_BYTES);
        }
        // set FIFO address to current RX address
        __writeRegister(LoRa, REG_FIFO_ADDR_PTR, __readRegister(LoRa, REG_FIFO_RX_CURRENT_ADDR));
        // put in standby mode
        idle(LoRa);
    }
    else if (__readRegister(LoRa, REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
        // not currently in RX mode
        // reset FIFO address
        __writeRegister(LoRa, REG_FIFO_ADDR_PTR, 0);
        // put in single RX mode
        __writeRegister(LoRa, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
    }
    return packetLength;
}

int packetRssi(struct LoRaClass *LoRa) {
    return (__readRegister(LoRa, REG_PKT_RSSI_VALUE) - (LoRa->_frequency < 868E6 ? 164 : 157));
}

float packetSnr(struct LoRaClass *LoRa) {
    return ((int8_t)__readRegister(LoRa, REG_PKT_SNR_VALUE)) * 0.25;
}

uint32_t write(struct LoRaClass *LoRa, uint8_t byte) {
    return _write(LoRa, &byte, sizeof(byte));
}

uint32_t _write(struct LoRaClass *LoRa, const uint8_t *buffer, uint32_t size) {
    uint32_t currentLength = __readRegister(LoRa, REG_PAYLOAD_LENGTH);

    // check size
    if ((currentLength + size) > MAX_PKT_LENGTH) {
        size = MAX_PKT_LENGTH - currentLength;
    }
    // write data
    for (uint32_t i = 0; i < size; i++) {
        __writeRegister(LoRa, REG_FIFO, buffer[i]);
    }
    // update length
    __writeRegister(LoRa, REG_PAYLOAD_LENGTH, currentLength + size);
    return size;
}

int available(struct LoRaClass *LoRa) {
    return (__readRegister(LoRa, REG_RX_NB_BYTES) - LoRa->_packetIndex);
}

int read(struct LoRaClass *LoRa) {
    if (!available(LoRa)) {
        return -1; 
        }
    LoRa->_packetIndex++;
    return __readRegister(LoRa, REG_FIFO);
}

int peek(struct LoRaClass *LoRa) {
    if (!available(LoRa)) {
        return -1; 
    }
    // store current FIFO address
    int currentAddress = __readRegister(LoRa, REG_FIFO_ADDR_PTR);
    // read
    uint8_t b = __readRegister(LoRa, REG_FIFO);
    // restore FIFO address
    __writeRegister(LoRa, REG_FIFO_ADDR_PTR, currentAddress);
    return b;
}

void receive(struct LoRaClass *LoRa, int size) {
    if (size > 0) {
        __implicitHeaderMode(LoRa);
        __writeRegister(LoRa, REG_PAYLOAD_LENGTH, size & 0xff);
    } else {
        __explicitHeaderMode(LoRa);
    }

    __writeRegister(LoRa, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

void idle(struct LoRaClass *LoRa) {
    __writeRegister(LoRa, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void sleep(struct LoRaClass *LoRa) {
    const uint16_t mode_sleep = 0x80; // MODE_LONG_RANGE_MODE | MODE_SLEEP;
    __writeRegister(LoRa, REG_OP_MODE, mode_sleep);
}

void setTxPower(struct LoRaClass *LoRa, int8_t power, int8_t outputPin) {
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = __readRegister(LoRa,  REG_PA_CONFIG );
    paDac = __readRegister(LoRa,  REG_PaDac );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | outputPin;
    paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK ) | 0x70;

    if ((paConfig & RF_PACONFIG_PASELECT_PABOOST) == RF_PACONFIG_PASELECT_PABOOST) {
	    if( power > 17 ) {
	        paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
	    }
	    else {
	        paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
	    }
	    if ((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON) {
            if( power < 5 ) {
                power = 5;
            }
            if( power > 20 ) {
                power = 20;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
	    }
	    else {
            if( power < 2 ) {
                power = 2;
            }
            if( power > 17 ) {
                power = 17;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
	    }
    }
    else
    {
        if( power < -1 ) {
            power = -1;
        }
        if( power > 14 ) {
            power = 14;
        }
        paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
	  }
	  __writeRegister(LoRa, REG_PA_CONFIG, paConfig );
	  __writeRegister(LoRa, REG_PaDac, paDac );
}

void setTxPowerMax(struct LoRaClass *LoRa, int level) {
	if (level < 5) {
		level = 5;
	}
	else if(level > 20)	{
		level = 20;
	}
	__writeRegister(LoRa, REG_LR_OCP,0x3f);
	__writeRegister(LoRa, REG_PaDac,0x87); // Open PA_BOOST
	__writeRegister(LoRa, REG_PA_CONFIG, RF_PACONFIG_PASELECT_PABOOST | (level - 5));
}

void setFrequency(struct LoRaClass *LoRa, long frequency) {
    LoRa->_frequency = frequency;
    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
    __writeRegister(LoRa, REG_FRF_MSB, (uint8_t)(frf >> 16));
    __writeRegister(LoRa, REG_FRF_MID, (uint8_t)(frf >> 8));
    __writeRegister(LoRa, REG_FRF_LSB, (uint8_t)(frf >> 0));
}

void setSpreadingFactor(struct LoRaClass *LoRa, int sf) {
    if (sf < 6) {
        sf = 6; 
    }
    else if (sf > 12) {
        sf = 12; 
    }
    if (sf == 6) {
        __writeRegister(LoRa, REG_DETECTION_OPTIMIZE, 0xc5);
        __writeRegister(LoRa, REG_DETECTION_THRESHOLD, 0x0c);
    } 
    else {
        __writeRegister(LoRa, REG_DETECTION_OPTIMIZE, 0xc3);
        __writeRegister(LoRa, REG_DETECTION_THRESHOLD, 0x0a);
    }
    __writeRegister(LoRa, REG_MODEM_CONFIG_2, (__readRegister(LoRa, REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
}

void setSignalBandwidth(struct LoRaClass *LoRa, long sbw) {
    int bw;

    if (sbw <= 7.8E3) { bw = 0; }
    else if (sbw <= 10.4E3) { bw = 1; }
    else if (sbw <= 15.6E3) { bw = 2; }
    else if (sbw <= 20.8E3) { bw = 3; }
    else if (sbw <= 31.25E3) { bw = 4; }
    else if (sbw <= 41.7E3) { bw = 5; }
    else if (sbw <= 62.5E3) { bw = 6; }
    else if (sbw <= 125E3) { bw = 7; }
    else if (sbw <= 250E3) { bw = 8; }
    else /*if (sbw <= 250E3)*/ { bw = 9; }
    __writeRegister(LoRa, REG_MODEM_CONFIG_1,(__readRegister(LoRa, REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
}

void setCodingRate4(struct LoRaClass *LoRa, int denominator) {
    if (denominator < 5) {
        denominator = 5;
    } else if (denominator > 8) {
        denominator = 8;
    }
    int cr = denominator - 4;
    __writeRegister(LoRa, REG_MODEM_CONFIG_1, (__readRegister(LoRa, REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

void setPreambleLength(struct LoRaClass *LoRa, long length) {
    __writeRegister(LoRa, REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
    __writeRegister(LoRa, REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
}

void setSyncWord(struct LoRaClass *LoRa, int sw) {
    __writeRegister(LoRa, REG_SYNC_WORD, sw);
}

void enable_crc(struct LoRaClass *LoRa) {
    __writeRegister(LoRa, REG_MODEM_CONFIG_2, __readRegister(LoRa, REG_MODEM_CONFIG_2) | 0x04);
}

void disable_crc(struct LoRaClass *LoRa) {
    __writeRegister(LoRa, REG_MODEM_CONFIG_2, __readRegister(LoRa, REG_MODEM_CONFIG_2) & 0xfb);
}

void crc(struct LoRaClass *LoRa) { 
    enable_crc(LoRa); 
}

void no_crc(struct LoRaClass *LoRa) { 
    disable_crc(LoRa); 
}

uint8_t random(struct LoRaClass *LoRa) {
    return __readRegister(LoRa, REG_RSSI_WIDEBAND);
}

void setPins(struct LoRaClass *LoRa, GPIO_TypeDef nss_port, GPIO_TypeDef reset_port, GPIO_TypeDef dio0_port, 
             int nss_pin, int reset_pin, int dio0_pin) {
    LoRa->_nss_port = nss_port;
    LoRa->_reset_port = reset_port;
    LoRa->_dio0_port = dio0_port;
    LoRa->_nss_pin = nss_pin;
    LoRa->_reset_pin = reset_pin;
    LoRa->_dio0_pin = dio0_pin;
}

void setSPIFrequency(struct LoRaClass *LoRa, uint32_t frequency) {
    // LoRa->_spiSettings = SPISettings(frequency, MSBFIRST, SPI_MODE0);
}

void __explicitHeaderMode(struct LoRaClass *LoRa) {
    LoRa->_implicitHeaderMode = 0;
    __writeRegister(LoRa, REG_MODEM_CONFIG_1, __readRegister(LoRa, REG_MODEM_CONFIG_1) & 0xfe);
}

void __implicitHeaderMode(struct LoRaClass *LoRa) {
    LoRa->_implicitHeaderMode = 1;
    __writeRegister(LoRa, REG_MODEM_CONFIG_1, __readRegister(LoRa, REG_MODEM_CONFIG_1) | 0x01);
}

uint8_t __readRegister(struct LoRaClass *LoRa, uint8_t address) {
    return __singleTransfer(LoRa, address & 0x7f, 0x00);
}

void __writeRegister(struct LoRaClass *LoRa, uint8_t address, uint8_t value) {
    __singleTransfer(LoRa, address | 0x80, value);
}

uint8_t __singleTransfer(struct LoRaClass *LoRa, uint8_t address, uint8_t value) {
    uint8_t response;
    GPIOA->ODR &= ~GPIO_ODR_ODR4;
    
    SPI1_transfer(address);
    response = SPI1_transfer(value);

    nss_delay_k(LoRa, 50);
    return response;
}




