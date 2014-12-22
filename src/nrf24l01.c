
//
// nrf24l01.c
//
//  Interface (driver) to nrf24l01+ module. User must register spi_tx function.
//
#include "nrf24l01.h"

typedef struct nrf24_s {
    void (*pUserSpiTx)(void *pTxData, void *pRxData, int len);
    uint8_t txBuf[16];
    uint8_t rxBuf[16];
} nrf24_t;
static nrf24_t nrf24;

void Nrf24l01_setSpiTx(void (*pUserSpiTx)(void *pTxData, void *pRxData, int len))
{
    nrf24.pUserSpiTx = pUserSpiTx;
}

void Nrf24l01_init(void)
{
    // check if the module is responding

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
