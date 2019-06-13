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
gpio_handle_t csPin;  //chip select, is GPIO03 for slot#1, SPI1_EN for slot#2

volatile int mems_event = 0;

//
//Initialization for Platform
//
void platform_init(void)
{
printf("DEBUG:call platform_init()\n");
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 960000);

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

    lsm6dsl_init(spi_write, spi_read, platform_init);
  
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

//If interrupts are used, this is the place to attach them to your callback.
//  attachInterrupt(INT1, INT1Event_cb, RISING);

    // Enable all HW events.
    lsm6dsl_Enable_X();
    lsm6dsl_Enable_Pedometer();
    lsm6dsl_Enable_Tilt_Detection();
    lsm6dsl_Enable_Free_Fall_Detection();
    lsm6dsl_Enable_Single_Tap_Detection();
    lsm6dsl_Enable_Double_Tap_Detection();
    lsm6dsl_Enable_6D_Orientation();
  
    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse example by Avnet\r\n");
    printf("   **    **    LSM6DSL Example\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    lsm6dsl_ReadID(&id);
    if( id == 0x6a )
        printf("LSM6DSL device found!");
    else {
        printf("NO LSM6DSL device found!");
        exit(EXIT_FAILURE);
        }

    gettimeofday(&time_start, NULL);
    time_now = time_start;

    while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
//
// read the INT pin status to determine if any HW events occured. If they did, 
// get the HW event status to determine what to do...
//
        if (mems_event) {
            mems_event = 0;
            void                   sendOrientation();
            LSM6DSL_Event_Status_t status;

            lsm6dsl_Get_Event_Status(&status);

            if (status.StepStatus) { // New step detected, so print the step counter
                lsm6dsl_Get_Step_Counter(&step_count);
                printf("Step counter: %d", step_count);
                }

            if (status.FreeFallStatus) 
                pintf("Free Fall Detected!");

            if (status.TapStatus) 
                pintf("Single Tap Detected!");

            if (status.DoubleTapStatus) 
                pintf("Double Tap Detected!");

            if (status.TiltStatus) 
                pintf("Tilt Detected!");

            if (status.D6DOrientationStatus) 
                sendOrientation();
            }
        gettimeofday(&time_now, NULL);
        }
    exit(EXIT_SUCCESS);
}


void sendOrientation()
{
  uint8_t xl = 0;
  uint8_t xh = 0;
  uint8_t yl = 0;
  uint8_t yh = 0;
  uint8_t zl = 0;
  uint8_t zh = 0;

  lsm6dsl_Get_6D_Orientation_XL(&xl);
  lsm6dsl_Get_6D_Orientation_XH(&xh);
  lsm6dsl_Get_6D_Orientation_YL(&yl);
  lsm6dsl_Get_6D_Orientation_YH(&yh);
  lsm6dsl_Get_6D_Orientation_ZL(&zl);
  lsm6dsl_Get_6D_Orientation_ZH(&zh);

  if ( xl == 0 && yl == 0 && zl == 0 && xh == 0 && yh == 1 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |  *             | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 1 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |             *  | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 1 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |  *             | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 1 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |             *  | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 1 ) {
      printf( "\r\n  __*_____________  " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 1 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |________________| " \
              "\r\n    *               \r\n" );
      }
  else
    printf( "None of the 6D orientation axes is set in LSM6DSL - accelerometer.\r\n" );
}

