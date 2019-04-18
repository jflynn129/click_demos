
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
#include <sys/time.h>


#include "lsm6dsl.h"
#include <hwlib/hwlib.h>

#define _delay(x) (usleep(x*1000))   //macro to provide ms pauses

#define INT_M1       GPIO_PIN_94  //slot #1
#define INT_M2       GPIO_PIN_7   //slot #2
#define GPIO_PIN_XYZ INT_M1

spi_handle_t  myspi = (spi_handle_t)0;
gpio_handle_t csPin; //spi chipselect pin

static void platform_init(void)
{
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 960000);

    gpio_init(GPIO_PIN_95, &csPin);
    gpio_dir(csPin, GPIO_DIR_OUTPUT);
    gpio_write(csPin,  GPIO_LEVEL_HIGH );         // RST is active low
}


static int32_t platform_write(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len)
{
    uint8_t *buff;

    buff = malloc(len+1);
    buff[0] = Reg;
    memcpy(&buff[1], Bufp, len);

    gpio_write( csPin,  GPIO_LEVEL_HIGH );
    int r=spi_transfer(myspi, buff, (uint32_t)len+1, NULL, (uint32_t)0);
    gpio_write( csPin,  GPIO_LEVEL_LOW );
    free(buff);
    return r;
}

static int32_t platform_read(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len)
{
    Reg |= 0x80;

    gpio_write( csPin,  GPIO_LEVEL_HIGH );
    spi_transfer(myspi, (uint8_t*)&Reg, (uint32_t)1, Bufp, (uint32_t)1000);
    gpio_write( csPin,  GPIO_LEVEL_LOW );
    return 0;
}


void usage (void)
{
    printf(" The 'uarti2cspi_demo' program can be started with several options:\n");
    printf(" -l   : run in loopback (Rx connected to TX line\n");
    printf(" -d X : run time in seconds (X) for loopback test\n");
    printf(" -t   : run in terminal mode (connected via uart to a different device)\n");
    printf(" -?   : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int           i, loopback_test=0, terminal_mode=0, run_time=30;
    struct        timeval time_start, time_now;

    while((i=getopt(argc,argv,"d:lt?")) != -1 )
        switch(i) {
           case 'd':
               sscanf(optarg,"%d",&run_time);
               printf(">> run loop back for %d seconds ",run_time);
               break;
           case 'l':
               loopback_test = 1;
               terminal_mode = 0;
               printf(">> run loop-back test");
               break;
           case 't':
               loopback_test = 0;
               terminal_mode = 1;
               printf(">> running in terminal mode");
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> nknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the UART I2C/SPI Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    gettimeofday(&time_start, NULL);
    time_now = time_start;

    if( loopback_test ) {
        uint8_t ch = 0x55;

        while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
            uartspi_putc(ch);
            _delay(10);
            if (uartspi_getc()!=ch) 
                fprintf( stderr, "!! ERROR, loopback failed.\n");
            ch = (ch==0x55)? 0xAA:0x55;
            _delay(500);
            gettimeofday(&time_now, NULL);
            }
        }
    else if( terminal_mode ) {
        while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
            if( uartspi_ready() ) {
                collect input up to CR
            if CR received
                echo back time and recived input
            }
        }
    else{
        fprintf (stderr, "ERROR: must specify a test type...\n");
        usage();
        exit(EXIT_FAILURE);
        }

    printf("DONE...\n");
    exit(EXIT_SUCCESS);
}

