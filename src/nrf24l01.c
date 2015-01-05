
//
// nrf24l01.c
//
//  Interface (driver) to nrf24l01+ module. User must register spi_tx function.
//
#include "nrf24l01.h"
#include "systick.h"
#include <string.h> // mem* functions

typedef struct nrf24_s {
    void (*pUserSpiTx)(void *pTxData, void *pRxData, uint32_t len);
    void (*pUserSetCE)(int setIt);
    uint8_t txBuf[33];  // max payload of 32 bytes + 1 command byte
    uint8_t rxBuf[33];
} nrf24_t;
static nrf24_t nrf24;

static void delay_ms(uint32_t delay)
{
    uint32_t now = systick_getMs();

    while(systick_getMs() - now < delay);
}

static void delay_us(uint32_t delay)
{
    uint32_t now = systick_getUs();

    while(systick_getUs() - now < delay);
}

static uint8_t getStatus(void)
{
    nrf24.txBuf[0] = 0xFF;  // NOP
    nrf24.rxBuf[0] = 0;

    nrf24.pUserSpiTx(nrf24.txBuf, nrf24.rxBuf, 1);

    return nrf24.rxBuf[0];
}

static void writeReg(uint8_t reg, uint8_t *pData, uint32_t len)
{
    nrf24.txBuf[0] = 0x20 | reg;  // write
    memcpy(&nrf24.txBuf[1], pData, len);
    nrf24.pUserSpiTx(nrf24.txBuf, nrf24.rxBuf, len + 1);
}

static void readReg(uint8_t reg, uint8_t *pData, uint32_t len)
{
    nrf24.txBuf[0] = reg;  // read
    nrf24.pUserSpiTx(nrf24.txBuf, nrf24.rxBuf, len + 1);

    // copy if non-null, otherwise caller is using rxBuf directly
    if(pData != 0){
        memcpy(pData, &nrf24.rxBuf[1], len);
    }
}

void Nrf24l01_setCallbacks(void (*pUserSpiTx)(void *pTxData, void *pRxData, uint32_t len),
                           void (*pUserSetCE)(int setIt))
{
    nrf24.pUserSpiTx = pUserSpiTx;
    nrf24.pUserSetCE = pUserSetCE;
}

int Nrf24l01_init(uint8_t *pAddress)
{
    uint8_t temp8;

    // 100ms delay per datasheet
    delay_ms(100);

    // check if the module is responding- assume we just powered up the module
    if(getStatus() != 0x0E){
        return -1;
    }

    // set address
    writeReg(0x0B, pAddress, 5);    // RX_ADDR_P1

    // confirm address
    readReg(0x0B, 0, 5);    // RX_ADDR_P1
    if(memcmp(&nrf24.rxBuf[1], pAddress, 5) != 0){
        return -2;
    }

    // power up, set config bit
    temp8 = 0x02;   // PWR_UP bit
    writeReg(0x00, &temp8, 1);

    // 1.5ms delay per datasheet
    delay_ms(2);

    // read CONFIG
    readReg(0x00, &temp8, 1);

    // ready to roll
    return 0;
}

int Nrf24l01_transmit(uint8_t *pData, int len, uint8_t *pAddress)
{
    uint8_t temp8;
    uint32_t timeOut;

    // assume PRIM_RX is cleared.

    // wait for tx fifo full flag to clear
    while(getStatus() & (0x01 << 5));   // bit 5, TX_FULL

    // set TX address
    writeReg(0x10, pAddress, 5);
    // set RX_ADDR_P0 to match for auto-ack
    writeReg(0x0A, pAddress, 5);

    // load packet into tx fifo
    if(len > 32){
        len = 32;
    }
    writeReg(0xA0, pData, len); // W_TX_PAYLOAD command

    // set CE high
    nrf24.pUserSetCE(1);

    // wait 10 us
    delay_us(10);

    // set CE low
    nrf24.pUserSetCE(0);

    // wait for TX_DS, or 100 ms timeout
    timeOut = systick_getMs();
    while((!(getStatus() & (1 << 5))) &&
          (systick_getMs() - timeOut < 100));
    // clear TX_DS
    temp8 = 1 << 5;
    writeReg(0x07, &temp8, 1);

    return len;
}

void Nrf24l01_setReceiveMode(int active, uint8_t payloadWidth)
{
    uint8_t temp8;

    if(active){
        // set payload width
        temp8 = payloadWidth;
        writeReg(0x12, &temp8, 1);

        // set PRIM_RX
        readReg(0x00, &temp8, 1);   // read config
        temp8 |= 1;                 // set PRIM_RX bit
        writeReg(0x00, &temp8, 1);

        // set CE high
        nrf24.pUserSetCE(1);

        // wait 130us
        delay_us(130);
    }
    else{
        // set CE low, clear PRIM_RX
        nrf24.pUserSetCE(0);
        readReg(0x00, &temp8, 1);   // read config
        temp8 &= ~1;                // clear PRIM_RX bit
        writeReg(0x00, &temp8, 1);
    }
}

int Nrf24l01_receive(uint8_t *pData)
{
    uint8_t temp8;

    // check rx fifo status
    readReg(0x17, &temp8, 1);
    if(temp8 & (1 << 0)){
        // RX_EMPTY
        return 0;
    }

    // read pipe the payload is in- retrieve status register by checking the
    // first byte in the last rx
    temp8 = (nrf24.rxBuf[0] >> 1) & 7;

    // get payload width in top payload
    readReg(0x60, &temp8, 1);

    // read out the bytes
    readReg(0x61, pData, temp8);     // R_RX_PAYLOAD command

    return temp8;
}
