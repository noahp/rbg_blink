
//
// nrf24l01.c
//
//  Interface (driver) to nrf24l01+ module. User must register spi_tx function.
//
#include "nrf24l01.h"

typedef struct nrf24_s {
    void (*pUserSpiTx)(void *pTxData, void *pRxData, uint32_t len);
    void (*pUserSetCE)(int setIt);
    uint8_t txBuf[33];  // max payload of 32 bytes + 1 command byte
    uint8_t rxBuf[33];
} nrf24_t;
static nrf24_t nrf24;

static uint8_t getStatus(void)
{
    nrf24.txBuf[0] = 0x07;
    nrf24.rxBuf[0] = 0;

    nrf24.pUserSpiTx(nrf24.txBuf, nrf24.rxBuf, 1);

    return nrf24.rxBuf[0];
}

void Nrf24l01_setCallbacks(void (*pUserSpiTx)(void *pTxData, void *pRxData, uint32_t len),
                           void (*pUserSetCE)(int setIt))
{
    nrf24.pUserSpiTx = pUserSpiTx;
    nrf24.pUserSetCE = pUserSetCE;
}

void Nrf24l01_init(void)
{
    // check if the module is responding- assume we just powered up the module
    if(getStatus() != 0x0E){
        return;
    }

    // set
}

void Nrf24l01_connect(void)
{

}

int Nrf24l01_transmit(void *pData, int len)
{
    return -1;
}

int Nrf24l01_receive(void *pData)
{
    return 0;
}
