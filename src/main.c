
#include "MKL26Z4.h"
#include "systick.h"
#include "delay.h"
#include <string.h> //memset

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
}

static void main_spi_send(uint8_t *pData, uint32_t len)
{
    // wait until dma busy flag is cleared
    while(DMA_BASE_PTR->DMA[0].DSR_BCR & DMA_DSR_BCR_BSY_MASK);
    //// pull spi mosi low (p27) for at least 50us
    //PORTC_PCR6 = PORT_PCR_MUX(1);
    //GPIOC_PCOR = (1 << 6);
    //delay_ms(1);
    // reset dma
    DMA_BASE_PTR->DMA[0].DSR_BCR = DMA_DSR_BCR_DONE_MASK;
    // write to the dma
    DMA_BASE_PTR->DMA[0].SAR = (uint32_t)pData;
    DMA_BASE_PTR->DMA[0].DSR_BCR = len & 0x00ffffff;
    // enable dma on the spi
    DMA_BASE_PTR->DMA[0].DCR |= DMA_DCR_ERQ_MASK;
    //PORTC_PCR6 = PORT_PCR_MUX(2);
    SPI0_C2 |= SPI_C2_TXDMAE_MASK;
}

static void main_init_spi(void)
{
    // init spi0
    // enable clocks for PORTC
    SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // configure io pins for spi- alt 2
    // PTC4-5-6
    PORTC_PCR4 = PORT_PCR_MUX(2);
    PORTC_PCR5 = PORT_PCR_MUX(2);
    PORTC_PCR6 = PORT_PCR_MUX(2);

    // set pc6 to output-
    GPIOC_PDDR |= (1 << 6);

    // enable SPI0 module
    SIM_SCGC4 |= SIM_SCGC4_SPI0_MASK;

    // configure as master, cs output driven automatically, CPOL=1
    SPI0_C1 |= (SPI_C1_MSTR_MASK | SPI_C1_SSOE_MASK | SPI_C1_CPOL_MASK);
    SPI0_C2 |= SPI_C2_MODFEN_MASK;

    // select clock divider- SPPR = 0b010 (/3), SPR = 0b0010 (/8).
    // target baud rate is 2MHz
    SPI0_BR = SPI_BR_SPPR(0x2) | SPI_BR_SPR(0x1);

    // turn on spi
    SPI0_C1 |= SPI_C1_SPE_MASK;

    // now set up dma
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX_MASK; // enable clocks
    SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;
    // dma mux
    // disable dmamux0 & clear channel select
    DMAMUX0_BASE_PTR->CHCFG[0] &= ~(DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE_MASK);
    // select channel 17 for spi tx & enable it
    DMAMUX0_BASE_PTR->CHCFG[0] |= DMAMUX_CHCFG_SOURCE(17) | DMAMUX_CHCFG_ENBL_MASK;

    // dma
    // set destination address register to the SPI tx register
    DMA_BASE_PTR->DMA[0].DAR = (uint32_t)&SPI0_DL;
    // configure DMA_0 for spi tx
    DMA_BASE_PTR->DMA[0].DCR = DMA_DCR_ERQ_MASK |       // enable periph. request
                               DMA_DCR_CS_MASK |
                               DMA_DCR_SINC_MASK |
                               DMA_DCR_SSIZE(0x01) |
                               DMA_DCR_DSIZE(0x01) |
                               DMA_DCR_D_REQ_MASK;

}

typedef union rgbData_u {
    uint32_t all;
    uint8_t byte[4];
    struct {
        uint8_t g;
        uint8_t r;
        uint8_t b;
        uint8_t pad;
    } color;
}rgbData_t;

//
//  encodeWS2812B - encode 8 bits into 24 bits. pOut is 3x pIn length. len is
//                  bytes.
//
static void encodeWS2812B(uint8_t *pIn, uint8_t *pOut, uint32_t len)
{
    uint8_t i,c;
    uint32_t j;
    rgbData_t output;

    for(j=0; j<len; j++){
        output.all = 0;
        c = pIn[j];
        // encode 1 byte into 3.
        for(i=0; i<8; i++){
            if(c & 0x80){
                output.all |= (0x6);
            }
            else{
                output.all |= (0x4);
            }
            c <<= 1;
            output.all <<= 3;
        }

        output.all >>= 3;

        pOut[3*j+0] = output.byte[2];
        pOut[3*j+1] = output.byte[1];
        pOut[3*j+2] = output.byte[0];
    }
}

static uint8_t rawData[169];
static rgbData_t rgbData = {.color = {.g=0xFF, .r=0x00, .b=0x00}};
static uint32_t dataOff = 0xFF;
void main_send_lights(void)
{
    // set the rgb values
    if(dataOff < 0xFF){
        rgbData.color.b--;
        rgbData.color.g++;
    }
    else if (dataOff < 0xFF * 2){
        // steady for 1 period
        asm("nop");
    }
    else if(dataOff < 0xFF * 3){
        rgbData.color.g--;
        rgbData.color.r++;
        if(rgbData.color.r > 0xFF /2){
            asm("nop");
        }
    }
    else if (dataOff < 0xFF * 4){
        // steady for 1 period
        asm("nop");
    }
    else if(dataOff < 0xFF * 5){
        rgbData.color.r--;
        rgbData.color.b++;
    }
    else if (dataOff < 0xFF * 6){
        // steady for 1 period
        asm("nop");
    }

    dataOff = (dataOff + 1) % (0xFF * 6);

    // encode the sequence
    encodeWS2812B(rgbData.byte, &rawData[160], 3);

    // send the data
    main_spi_send(rawData, sizeof(rawData));
}

static void main_led(void)
{
    static uint32_t blinkTime = 0;
    static uint8_t blinkCount = 0;

    // blink every 250ms
    if(blinkCount < 10){
        if(systick_getMs() - blinkTime > 250){
            blinkTime = systick_getMs();
            // toggle
            GPIOB_PTOR = (1 << 0);
            blinkCount++;
        }
    }
}

static void main_rgb(void)
{
    static uint32_t rgbTime = 0;

    // refresh every 100ms
    if(systick_getMs() - rgbTime > 5){
        rgbTime = systick_getMs();
        main_send_lights();
    }
}

int main(void) {
    // initialize the necessary
    main_init_io();
    main_init_spi();

    memset(rawData, 0, sizeof(rawData));

    while(1){
        // led task
        main_led();

        // rgb task
        main_rgb();
    }

    return 0;
}
