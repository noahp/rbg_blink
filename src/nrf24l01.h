
//
// nrf24l01.h
//
//  API for nrf24l01.c, which is the interface to the nrf24l01+ modules.
//
#include <stdint.h>

// set user spi tx function. pTxData should be clocked out, and pRxData should
// be clocked in.
void Nrf24l01_setSpiTx(void (*pUserSpiTx)(void *pTxData, void *pRxData, int len));

// initialize internal state.
void Nrf24l01_init(void);

// connect to the module
void Nrf24l01_connect(void);

// send a payload to the module for tranmission.
int Nrf24l01_transmit(void *pData, int len);

// emit data if available, only one packet.
int Nrf24l01_receive(void *pData);
