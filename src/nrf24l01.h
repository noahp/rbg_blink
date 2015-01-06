
//
// nrf24l01.h
//
//  API for nrf24l01.c, which is the interface to the nrf24l01+ modules.
//
#include <stdint.h>

// ->set user spi tx function. pTxData should be clocked out, and pRxData should
// be clocked in. pRxData may be set to null.
// ->set function to toggle CE pin- if setIt, CE should go active high, else low
void Nrf24l01_setCallbacks(void (*pUserSpiTx)(void *pTxData, void *pRxData, uint32_t len),
                           void (*pUserSetCE)(int setIt));

// initialize internal state, connect and initialize the module. pAddress is the
// 5-byte address for this module.
int Nrf24l01_init(uint8_t *pAddress);

// send a payload to the module for tranmission.
int Nrf24l01_transmit(uint8_t *pData, int len, uint8_t *pAddress);

// set receive mode to active if non-zero, deactivate it otherwise
void Nrf24l01_setReceiveMode(int active);

// emit data if available, only one packet.
int Nrf24l01_receive(uint8_t *pData);
