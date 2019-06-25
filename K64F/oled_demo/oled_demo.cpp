/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/un.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include "Avnet_GFX.h"
#include "oledb_ssd1306.h"

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

spi_handle_t  myspi = (spi_handle_t)0;
gpio_handle_t rstPin; //spi reset pin
gpio_handle_t dcPin;  //device/command pin

//
//SPI handler for OLEDB display
//
void spi_init(void)
{
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 960000);

    gpio_init(GPIO_PIN_95, &rstPin);
    gpio_write(rstPin,  GPIO_LEVEL_HIGH );         // RST is active low
    gpio_dir(rstPin, GPIO_DIR_OUTPUT);

    gpio_init(GPIO_PIN_96, &dcPin);
    gpio_write(dcPin,  GPIO_LEVEL_HIGH );          // D/C, HIGH=Data, LOW=Command
    gpio_dir(dcPin, GPIO_DIR_OUTPUT);
}
 
int spi_write( uint16_t cmd, uint8_t *b, int siz )
{
    if( cmd == SSD1306_COMMAND) //if sending a Command
        gpio_write( dcPin,  GPIO_LEVEL_LOW );
    int r=spi_transfer(myspi, b, (uint32_t)siz,NULL,(uint32_t)0);
    if( cmd == SSD1306_COMMAND)
        gpio_write( dcPin,  GPIO_LEVEL_HIGH );
    return r;
}

int oled_reset(void)
{
    gpio_write( rstPin,  GPIO_LEVEL_LOW );
    delay(10);   //10 msec
    gpio_write( rstPin,  GPIO_LEVEL_HIGH );
    delay(10);   //10 msec
    return 0;
}


int main(int argc, char *argv[]) 
{
    void           oledb_test(OLEDB_SSD1306*);
    OLEDB_SSD1306* oled_display = open_oled( spi_init, oled_reset, spi_write );

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    Mikroe OLED-B Demo\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    printf("This program runs through a series of graphics tests.\r\n");
    printf("\r\n");
    oledb_test(oled_display);
    close_oled(oled_display);
    exit(EXIT_SUCCESS);
}
