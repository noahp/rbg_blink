
#include "MKL26Z4.h"
#include "systick.h"
#include <string.h> //memset
#include "usb_main.h"
#include "nrf24l01.h"
#include <stdio.h>
#include "usb.h"

extern void  initialise_monitor_handles(void);

static void main_init_io(void)
{
    // init ports
    // disable COP
    SIM_COPC = 0;

    // enable clocks for PORTB
    SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

    // set B0 to GPIO
    PORTB_PCR0 = PORT_PCR_MUX(1);

    // set output B0 low (LED on initially)
    GPIOB_PCOR = (1 << 0);

    // set B0 DDR to output
    GPIOB_PDDR |= (1 << 0);

/*
    nrf24l01 pinout
    25  -   CS
    26  -   SCK
    27  -   MOSI
    28  -   MISO
    29  -   IRQ
    30  -   CE
*/
    // set CE to output
    // enable clocks for PORTD
    SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK;
    // set D5 to GPIO
    PORTD_PCR5 = PORT_PCR_MUX(1);
    // set output D5 low, disable
    GPIOD_PCOR = (1 << 5);
    // set D5 DDR to output
    GPIOD_PDDR |= (1 << 5);
    uint32_t msTime = systick_getMs();
    while(systick_getMs() - msTime < 5);
}

static void main_init_spi(void)
{
    // init spi0
    // enable clocks for PORTC
    SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // configure io pins for spi- alt 2
    // PTC4-5-6-7
    PORTC_PCR4 = PORT_PCR_MUX(1);   // gpio
    PORTC_PCR5 = PORT_PCR_MUX(2);
    PORTC_PCR6 = PORT_PCR_MUX(2);
    PORTC_PCR7 = PORT_PCR_MUX(2);

    // set pc4 to high, disable
    GPIOC_PSOR = (1 << 4);
    // set pc4 to output-
    GPIOC_PDDR |= (1 << 4);

    // enable SPI0 module
    SIM_SCGC4 |= SIM_SCGC4_SPI0_MASK;

    // configure as master, cs output driven manually, CPOL=0
    SPI0_C1 = //SPI_C1_LSBFE_MASK
              //SPI_C1_SSOE_MASK
              //SPI_C1_CPHA_MASK
              //SPI_C1_CPOL_MASK
              SPI_C1_MSTR_MASK;
              //SPI_C1_SPTIE_MASK
              //SPI_C1_SPE_MASK
              //SPI_C1_SPIE_MASK
    SPI0_C2 = 0;

    // select clock divider- SPPR = 0b010 (/3), SPR = 0b0010 (/8).
    // target baud rate is 2MHz
//    SPI0_BR = SPI_BR_SPPR(0x2) | SPI_BR_SPR(0x1);
    SPI0_BR = SPI_BR_SPPR(0x7) | SPI_BR_SPR(0x2);

    // turn on spi
    SPI0_C1 |= SPI_C1_SPE_MASK;

    // now set up dma
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX_MASK; // enable clocks
    SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;
    // dma mux
    // disable dmamux0 & clear channel select
    DMAMUX0_BASE_PTR->CHCFG[0] &= ~(DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE_MASK);
    // select channel 16 for spi rx & enable it
    DMAMUX0_BASE_PTR->CHCFG[0] |= DMAMUX_CHCFG_SOURCE(16) | DMAMUX_CHCFG_ENBL_MASK;
    // disable dmamux1 & clear channel select
    DMAMUX0_BASE_PTR->CHCFG[1] &= ~(DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE_MASK);
    // select channel 17 for spi tx & enable it
    DMAMUX0_BASE_PTR->CHCFG[1] |= DMAMUX_CHCFG_SOURCE(17) | DMAMUX_CHCFG_ENBL_MASK;

    // dma
    // set destination address register to the SPI rx register
    DMA_BASE_PTR->DMA[0].SAR = (uint32_t)&SPI0_DL;
    // configure DMA_0 for spi rx
    DMA_BASE_PTR->DMA[0].DCR = DMA_DCR_ERQ_MASK |       // enable periph. request
                               DMA_DCR_CS_MASK |
                               DMA_DCR_DINC_MASK |
                               DMA_DCR_SSIZE(0x01) |
                               DMA_DCR_DSIZE(0x01) |
                               DMA_DCR_D_REQ_MASK;
    // set destination address register to the SPI tx register
    DMA_BASE_PTR->DMA[1].DAR = (uint32_t)&SPI0_DL;
    // configure DMA_1 for spi tx
    DMA_BASE_PTR->DMA[1].DCR = DMA_DCR_ERQ_MASK |       // enable periph. request
                               DMA_DCR_CS_MASK |
                               DMA_DCR_SINC_MASK |
                               DMA_DCR_SSIZE(0x01) |
                               DMA_DCR_DSIZE(0x01) |
                               DMA_DCR_D_REQ_MASK;
}

static void main_spi_send(void *pTxData, void *pRxData, uint32_t len)
{
    // wait until dma busy flag is cleared
    while(DMA_BASE_PTR->DMA[0].DSR_BCR & DMA_DSR_BCR_BSY_MASK);
    while(DMA_BASE_PTR->DMA[1].DSR_BCR & DMA_DSR_BCR_BSY_MASK);

    // cs low
    GPIOC_PCOR = (1 << 4);

    // reset dma
    DMA_BASE_PTR->DMA[0].DSR_BCR = DMA_DSR_BCR_DONE_MASK;
    DMA_BASE_PTR->DMA[1].DSR_BCR = DMA_DSR_BCR_DONE_MASK;
    // write to the dma
    DMA_BASE_PTR->DMA[0].DAR = (uint32_t)pRxData;
    DMA_BASE_PTR->DMA[0].DSR_BCR = len & 0x00ffffff;
    DMA_BASE_PTR->DMA[1].SAR = (uint32_t)pTxData;
    DMA_BASE_PTR->DMA[1].DSR_BCR = len & 0x00ffffff;
    // enable dma on the spi
    DMA_BASE_PTR->DMA[0].DCR |= DMA_DCR_ERQ_MASK;
    DMA_BASE_PTR->DMA[1].DCR |= DMA_DCR_ERQ_MASK;
    SPI0_C2 |= SPI_C2_TXDMAE_MASK | ((pRxData != NULL)?(SPI_C2_RXDMAE_MASK):(0));
    // wait for operations to complete (blocking)
    while(!(DMA_BASE_PTR->DMA[0].DSR_BCR & DMA_DSR_BCR_DONE_MASK));
    while(!(DMA_BASE_PTR->DMA[1].DSR_BCR & DMA_DSR_BCR_DONE_MASK));

    // cs high
    GPIOC_PSOR = (1 << 4);
}

static void main_ce_set(int setIt)
{
    if(setIt){
        GPIOD_PSOR = (1 << 5);
    }
    else{
        GPIOD_PCOR = (1 << 5);
    }
}

static void main_led(void)
{
    static uint32_t blinkTime = 0;
    static uint32_t blinkCount = 0;

    // blink every 250ms
    if(systick_getMs() - blinkTime > 250){
        blinkTime = systick_getMs();
        // toggle
        GPIOB_PTOR = (1 << 0);

        printf("says who!!!! %d\n", (int)blinkCount++);
    }
}

#define MY_ADDRESS      "/x12/x34/x56/x78/x90"
#define REMOTE_ADDRESS  "/x12/x34/x56/x78/x91"
int main(void)
{
    uint8_t cdcChar, rxChar;
    uint32_t rxPollTime = 0;

    initialise_monitor_handles();

    // initialize the necessary
    main_init_io();
    main_init_spi();

    // initialize nrf component
    Nrf24l01_setCallbacks(main_spi_send, main_ce_set);
    Nrf24l01_init((uint8_t *)MY_ADDRESS);   // set address
    Nrf24l01_setReceiveMode(1);

    // usb init
    usb_main_init();

    while(1){
        // led task
        main_led();

        if(systick_getMs() - rxPollTime > 5){
            rxPollTime = systick_getMs();
            if(Nrf24l01_receive(&rxChar)){
                EP_IN_Transfer(EP2, &rxChar, 1);
            }
        }

        // usb task
        if(usb_main_mainfunction(&cdcChar) != -1){
            // send it
            Nrf24l01_setReceiveMode(0);
            if(Nrf24l01_transmit(&cdcChar, 1, (uint8_t *)REMOTE_ADDRESS)){
                // Send it back to the PC
                //EP_IN_Transfer(EP2, &cdcChar, 1);
            }
            Nrf24l01_setReceiveMode(1);
        }
    }

    return 0;
}
