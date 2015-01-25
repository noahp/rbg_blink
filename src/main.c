
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

    module pinout
    | IRQ | SO  |
    | MO  | SCK |
    | CS  | CE  |
    | V+  | GND | <- pin 1
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

static void main_init_i2c(void)
{
    // set up i2c and power/gnd for si7021 module
    // pins
    // 17 - gnd, 18 - vcc, 20 - scl, 21 - sda
    // enable clocks for port a & b, & i2c0
    SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK;
    SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;

    // set PTA18 & PTA19 to gpio, PTB0 & PTB1 to I2C0
    PORTA_PCR18 = PORT_PCR_MUX(1);   // gpio
    PORTA_PCR19 = PORT_PCR_MUX(1);   // gpio
    PORTB_PCR0 = PORT_PCR_MUX(2);   // i2c0
    PORTB_PCR1 = PORT_PCR_MUX(1);   // i2c0

    // configure i2c0
    // set mult to 4, icr to 27 for i2c baud rate = bus rate / (4 x 480) = 25kHz
    I2C0_F = I2C_F_MULT(4) | I2C_F_ICR(27);
    // enable i2c
    I2C0_C1 = I2C_C1_IICEN_MASK;

    // set pa18 & pa19 to low
    GPIOC_PCOR = (1 << 18) | (1 << 19);
    // set pa18 & pa19 to output
    GPIOC_PDDR |= (1 << 18) | (1 << 19);
    // set pa18 to high to power the module
    GPIOC_PSOR = (1 << 18);

    // startup delay
    delay_ms(10);

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

    // blink every 250ms
    if(systick_getMs() - blinkTime > 250){
        blinkTime = systick_getMs();
        // toggle
        GPIOB_PTOR = (1 << 0);
    }
}
static void delay_ms(uint32_t delay)
{
    uint32_t now = systick_getMs();

    while(systick_getMs() - now < delay);
}


#define MY_ADDRESS      "/x12/x34/x56/x78/x90"
#define REMOTE_ADDRESS  "/x12/x34/x56/x78/x91"
int main(void)
{
    uint8_t cdcChar, i;
    uint8_t rxBuf[32];
    uint32_t rxPollTime = 0;
    int rxBytes;

    // enable printf if debugger is connected
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
            rxBytes = Nrf24l01_receive(rxBuf);
            if(rxBytes){
                EP_IN_Transfer(EP2, rxBuf, rxBytes);

                if(rxBuf[0] == 't'){
                    // send temperature
                    #define WORDUP "temperature\n\r"
                    memcpy(rxBuf, WORDUP, sizeof(WORDUP) - 1);
                    Nrf24l01_setReceiveMode(0);
                    for(i=0; i<sizeof(WORDUP) - 1; i++){
                        Nrf24l01_transmit(&rxBuf[i], 1, (uint8_t *)REMOTE_ADDRESS);
    //                    for(i=0; i<sizeof(WORDUP); i++){
    //                        Nrf24l01_transmit(rxBuf[i], 1, (uint8_t *)REMOTE_ADDRESS);
    //                    }
                    }
                    Nrf24l01_setReceiveMode(1);
                }
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
