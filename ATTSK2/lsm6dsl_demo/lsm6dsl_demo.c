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
#include <sys/time.h>

#include <hwlib/hwlib.h>

#include <LSM6DSLSensor.h>

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

spi_handle_t  myspi = (spi_handle_t)0;
//gpio_handle_t rstPin; //spi reset pin
gpio_handle_t csPin;  //chip select, is GPIO03 for slot#1, SPI1_EN for slot#2

volatile int mems_event = 0;

//
//SPI handler for OLEDB display
//
void spi_init(void)
{
printf("DEBUG:call spi_init()\n");
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 960000);

//    gpio_init(GPIO_PIN_95, &rstPin);
//    gpio_write(rstPin,  GPIO_LEVEL_HIGH );         // RST is active low
//    gpio_dir(rstPin, GPIO_DIR_OUTPUT);

    //Interrupts.
//    attachInterrupt(INT1, INT1Event_cb, RISING);
// setup the interrupt pin

//    gpio_init(GPIO_PIN_96, &dcPin);
//    gpio_write(dcPin,  GPIO_LEVEL_HIGH );          // D/C, HIGH=Data, LOW=Command
//    gpio_dir(dcPin, GPIO_DIR_OUTPUT);
}
 
uint8_t spi_write( uint8_t *b, uint8_t reg, uint16_t siz )
{
    uint8_t *tmp = malloc(siz+1);
    tmp[0] = reg;
    memcpy(&tmp[1], b, siz);

    int r=spi_transfer(myspi,tmp,(uint32_t)siz+1,NULL,(uint32_t)0);

printf("DEBUG:called spi_write(), send %d bytes, first byte is 0x%02X, call returned %d\n", siz+1, tmp[0], r);
    free(tmp);

    return r;
}

uint8_t spi_read( uint8_t *b,uint8_t reg,uint16_t siz)
{
    int r=spi_transfer(myspi,&reg,(uint32_t)1,b,(uint32_t)siz);
printf("DEBUG:called spi_read(), reg=0x%02X, data=0x%02X (%d), returned %d\n", reg, *b, siz, r);

    return r;
}

void INT1_mems_event_cb()
{
  mems_event = 1;
}

void usage (void)
{
    printf(" The 'c_demo' program can be started with several options:\n");
    printf(" -r X: Set the reporting period in 'X' (seconds)\n");
    printf(" -t  : Test the OLED-B Click Board\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int            i, run_time = 30;  
    struct timeval time_start, time_now;
    uint8_t        id;

    lsm6dsl_init(spi_write, spi_read, spi_init);

    lsm6dsl_ReadID(&id);
    printf("DEVICE ID REPORTED AS: 0x%02X\n",id);
exit(EXIT_SUCCESS);

    // Initlialize Components.
    lsm6dsl_Enable_X();

    // Enable Tilt Detection.
    lsm6dsl_Enable_Tilt_Detection();

    while((i=getopt(argc,argv,"r:?")) != -1 )

        switch(i) {
           case 'r':
               sscanf(optarg,"%x",&run_time);
               printf(">> run for %d seconds ",run_time);
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
    printf("    **  **     SW reuse example by Avnet\r\n");
    printf("   **    **    LSM6DSL Example\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    gettimeofday(&time_start, NULL);
    time_now = time_start;

    while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
        if (mems_event) {
            mems_event = 0;
            LSM6DSL_Event_Status_t status;
            lsm6dsl_Get_Event_Status(&status);
            if (status.TiltStatus) {
                //start led blinking...
                // and Output data.
                printf("Tilt Detected!\n");
                }
              }
        gettimeofday(&time_now, NULL);
        }
    exit(EXIT_SUCCESS);
}
