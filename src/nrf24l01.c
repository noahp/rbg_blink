
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

static uint8_t getStatus(void)
{
    nrf24.txBuf[0] = 0x07;
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
    // 100ms delay per datasheet
    delay_ms(100);

    // check if the module is responding- assume we just powered up the module
    if(getStatus() != 0x0E){
        return -1;
    }

    // enable address P1
    // set address
    writeReg(0x0B, pAddress, 5);    // RX_ADDR_P1

    // confirm address
    readReg(0x0B, 0, 5);    // RX_ADDR_P1
    if(memcmp(nrf24.rxBuf, pAddress, 5) != 0){
        return -2;
    }

    // power up
    nrf24.txBuf[0] = 0x20 | 0x00;  // write | config register
    nrf24.txBuf[1] = 0x02;  // PWR_UP bit

    nrf24.pUserSpiTx(nrf24.txBuf, 0, 2);

    // 1.5ms delay per datasheet
    delay_ms(2);

    // ready to roll
    return 0;
}

int Nrf24l01_transmit(void *pData, int len)
{
    // wait for tx fifo full flag to clear
    while(!(getStatus() & 0x01));

    // load packet into tx fifo
    if(len > 32){
        len = 32;
    }


    // set CE high

    // wait 1 ms

    // set CE low

    return len;
}

int Nrf24l01_receive(void *pData)
{
    return 0;
}
