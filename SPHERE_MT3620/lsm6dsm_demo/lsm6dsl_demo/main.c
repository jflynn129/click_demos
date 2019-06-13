/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>


// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/spi.h>

#include "mt3620_rdb.h"

#include "lsm6dsl_reg.h"

#define MIKRO_INT       MT3620_GPIO34  //slot #1 =MT3620_GPIO34 ; slot #2 = MT3620_GPIO35
#define MIKRO_CS        MT3620_GPIO2   //MT3620_GPIO2 

static int spiFd   = -1;
//static int intPinFd= -1;

//static GPIO_Value_Type intPin;

static void platform_init(void)
{
    SPIMaster_Config spi_config;

//    intPinFd= GPIO_OpenAsInput( MIKRO_INT );
    int r = SPIMaster_InitConfig(&spi_config);
    if (r != 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig=%d, errno=%s (%d)\n",r, strerror(errno),errno);
        return;
        }

    spi_config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
    spiFd = SPIMaster_Open(MT3620_SPI_ISU1, MT3620_SPI_CHIP_SELECT_A, &spi_config);
    if (spiFd < 0) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }

    int result = SPIMaster_SetBusSpeed(spiFd, 400000);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }

    result = SPIMaster_SetMode(spiFd, SPI_Mode_3);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    reg |= 0x80;

    ssize_t i = SPIMaster_WriteThenRead(spiFd, &reg,  1, bufp, len );
    if( i == -1 ) {
        Log_Debug("ERROR: SPIMaster_WriteThenRead: errno=%d/%s\n",errno,strerror(errno));
        i=0;
        }
    return 0;

}

static int32_t platform_write(void *handle, uint8_t Reg, uint8_t *bufp, uint16_t len)
{
    SPIMaster_Transfer transfer;

    if( SPIMaster_InitTransfers(&transfer,1) != 0 )
        return -1;

    transfer.flags = SPI_TransferFlags_Write;
    transfer.writeData = bufp;
    transfer.length = len;

	if (SPIMaster_TransferSequential(spiFd, &transfer, 1) == -1)
		Log_Debug("ERROR: SPIMaster_TransferSequential: %d/%s\n", errno, strerror(errno));
    return 0;
}

void usage(void)
{
    Log_Debug(" The 'lcdmini_demo' program can be started with several options:\n");
    Log_Debug(" -r X        : Set run time in seconds for demo to run\n");
    Log_Debug(" -t \"testing\": Set the text to use during demo run\n");
    Log_Debug(" -?          : Display usage info\n");
}

int main(int argc, char *argv[])
{
    int           i, run_time = 30;  //default to 3 seconds
    uint8_t       rst, m;
    struct        timeval time_start, time_now;
    axis3bit16_t  data_raw_acceleration;
    axis3bit16_t  data_raw_angular_rate;
    axis1bit16_t  data_raw_temperature;
    float         acceleration_mg[3];
    float         angular_rate_mdps[3];
    float         temperature_degC;

    lsm6dsl_ctx_t dev_ctx;

    while ((i = getopt(argc, argv, "r:?")) != -1)
        switch (i) {
        case 'r':
            sscanf(optarg, "%x", &run_time);
            Log_Debug(">> run-time set to %d seconds ", run_time);
            break;
        case '?':
            usage();
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, ">> nknown option character `\\x%x'.\n", optopt);
            exit(EXIT_FAILURE);
        }

    Log_Debug("\n\n");
    Log_Debug("     ****\r\n");
    Log_Debug("    **  **     SW reuse using C example\r\n");
    Log_Debug("   **    **    for the ST LSM6DSL Click\r\n");
    Log_Debug("  ** ==== **\r\n");
    Log_Debug("\r\n");

    platform_init();
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.handle = NULL;

    m = 0;
    lsm6dsl_device_id_get(&dev_ctx, &m);
    Log_Debug("LSM6DSL Sensor Found (0x%02X)!\n", m);
    if (m != LSM6DSL_ID) exit(EXIT_FAILURE);

	lsm6dsl_reset_set(&dev_ctx, PROPERTY_ENABLE);
	do {
		lsm6dsl_reset_get(&dev_ctx, &rst);
	} while (rst);
	/*
	 *  Enable Block Data Update
	 */
	lsm6dsl_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
	/*
	 * Set Output Data Rate
	 */
	lsm6dsl_xl_data_rate_set(&dev_ctx, LSM6DSL_XL_ODR_12Hz5);
	lsm6dsl_gy_data_rate_set(&dev_ctx, LSM6DSL_GY_ODR_12Hz5);
	/*
	 * Set full scale
	 */
	lsm6dsl_xl_full_scale_set(&dev_ctx, LSM6DSL_2g);
	lsm6dsl_gy_full_scale_set(&dev_ctx, LSM6DSL_2000dps);

	/*
	 * Configure filtering chain(No aux interface)
	 */
	 /* Accelerometer - analog filter */
	lsm6dsl_xl_filter_analog_set(&dev_ctx, LSM6DSL_XL_ANA_BW_400Hz);

	/* Accelerometer - LPF1 path ( LPF2 not used )*/
	lsm6dsl_xl_lp1_bandwidth_set(&dev_ctx, LSM6DSL_XL_LP1_ODR_DIV_4);

	/* Accelerometer - LPF1 + LPF2 path */
	//lsm6dsl_xl_lp2_bandwidth_set(&dev_ctx, LSM6DSL_XL_LOW_NOISE_LP_ODR_DIV_100);

	/* Accelerometer - High Pass / Slope path */
	lsm6dsl_xl_reference_mode_set(&dev_ctx, PROPERTY_DISABLE);
	lsm6dsl_xl_hp_bandwidth_set(&dev_ctx, LSM6DSL_XL_HP_ODR_DIV_100);

	/* Gyroscope - filtering chain */
	lsm6dsl_gy_band_pass_set(&dev_ctx, LSM6DSL_HP_260mHz_LP1_STRONG);

    gettimeofday(&time_start, NULL);
    time_now = time_start;
	m = 0;
    while (difftime(time_now.tv_sec, time_start.tv_sec) < run_time) {
        lsm6dsl_reg_t reg;

		sleep(1);
		Log_Debug("\r(%2d)", ++m);
		gettimeofday(&time_now, NULL);
		lsm6dsl_status_reg_get(&dev_ctx, &reg.status_reg);
//		if (!reg.byte )
//			continue;

		Log_Debug(" status reg is 0x%02X",reg.byte);
//        if (reg.status_reg.xlda) { /* Read magnetic field data */
            memset(data_raw_acceleration.u8bit, 0x00, 3 * sizeof(int16_t));
            lsm6dsl_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
            acceleration_mg[0] = lsm6dsl_from_fs2g_to_mg(data_raw_acceleration.i16bit[0]);
            acceleration_mg[1] = lsm6dsl_from_fs2g_to_mg(data_raw_acceleration.i16bit[1]);
            acceleration_mg[2] = lsm6dsl_from_fs2g_to_mg(data_raw_acceleration.i16bit[2]);
            Log_Debug("Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n", acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
//            }

//        if (reg.status_reg.gda) { /* Read magnetic field data */
            memset(data_raw_angular_rate.u8bit, 0x00, 3 * sizeof(int16_t));
            lsm6dsl_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate.u8bit);
            angular_rate_mdps[0] = lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[0]);
            angular_rate_mdps[1] = lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[1]);
            angular_rate_mdps[2] = lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[2]);
            Log_Debug("Angular rate [mdps]:%4.2f\t%4.2f\t%4.2f\r\n", angular_rate_mdps[0], angular_rate_mdps[1], angular_rate_mdps[2]);
//            }

//        if (reg.status_reg.tda) {   /* Read temperature data */
            memset(data_raw_temperature.u8bit, 0x00, sizeof(int16_t));
            lsm6dsl_temperature_raw_get(&dev_ctx, data_raw_temperature.u8bit);
            temperature_degC = lsm6dsl_from_lsb_to_celsius(data_raw_temperature.i16bit);
            Log_Debug("Temperature [degC]:%6.2f\r\n", temperature_degC);
//            }
        }

    Log_Debug("\nDONE...");
    exit(EXIT_SUCCESS);
}

