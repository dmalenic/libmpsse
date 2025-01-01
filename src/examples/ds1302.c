/* DS1302 DataSheet: https://www.analog.com/media/en/technical-documentation/data-sheets/DS1302.pdf */

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpsse.h>
#include <time.h>
#include <unistd.h>


/* Timer registers */
typedef struct {
	uint8_t cmd;		/* command byte */
	uint8_t	sec;		/* reg 0 */
	uint8_t	min;		/* reg 1 */
	uint8_t	hour;		/* reg 2 */
	uint8_t	date;		/* reg 4 */
	uint8_t	mon;		/* reg 5 */
	uint8_t	wday;		/* reg 3 */
	uint8_t	year;		/* reg 6 */
	uint8_t ctrl;		/* ret 7 */
} t_ds1302regs;


static const t_ds1302regs set_current_date(void);
volatile uint8_t run = 1;

/* Signal handler function */
static void handle_sigint(int sig) {
	printf("Caught signal %d (Ctrl+C).\n", sig);
	run = 0;
}


int main(void)
{
	/* Register the signal handler for SIGINT */
	signal(SIGINT, handle_sigint);

	struct mpsse_context *ds1302 = NULL;

	int retval = EXIT_FAILURE;

	/* Initialzie DS1302 in SPI3 mode, 1MHz speed and LSB bit order, with CS to low on idle */
	if((ds1302 = MPSSE(SPI3, ONE_MHZ, LSB)) != NULL && ds1302->open)
	{
		SetCSIdle(ds1302, 0);

		printf("%s initialized at %dHz (SPI mode 3), LSB and CS low on idle.\n", GetDescription(ds1302), GetClock(ds1302));
		printf("VID 0x%04x PID 0x%04x\n", GetVid(ds1302), GetPid(ds1302));

		printf("Reading control register.\n");

		char *control = NULL;
		char cmd = 0x8F;

		/* singler-byte read command */
		Start(ds1302);
		Write(ds1302, &cmd, 1);
		control = Read(ds1302, 1);
		Stop(ds1302);

		printf("Writing control register.\n");
		control[0] &= ~0x80;

		/*
		 * single-byte write command
		 * disabling write-protect bit is not strictly necessary in this example
		 * because it protects RAM registers only, and we are writing Clock registers
		 */
		cmd = 0x8E;
		Start(ds1302);
		Write(ds1302, &cmd, 1);
		Write(ds1302, control, 1);
		Stop(ds1302);

		free(control);

		printf("Initializing time:\n");

		t_ds1302regs ds1302regs = set_current_date();

		char *data = (char *)&ds1302regs;
		for (int i = 1; i < sizeof(ds1302regs); ++i)
		{
			printf("%02x ", (uint8_t)data[i]);
		}

		/* clock brust-mode write command */
		ds1302regs.cmd = 0xBE;
		Start(ds1302);
		Write(ds1302, (char*)&ds1302regs, sizeof(ds1302regs));
		Stop(ds1302);

		printf("\n\nReading time in one second intervals (stop with Ctrl+C):\n");

		while(run)
		{
			sleep(1);

			/* clock brust-mode read command */
			cmd = 0xBF;
			Start(ds1302);
			Write(ds1302, &cmd, 1);
			char *data = Read(ds1302, 8);
			Stop(ds1302);

			for (int i = 0; i < 8; ++i)
			{
				printf("%02x ", (uint8_t)data[i]);
			}
			printf("\n");

			free(data);
		}	
		printf("Exiting...\n");
	}
	else
	{
		printf("Failed to initialize MPSSE: %s\n", ErrorString(ds1302));
	}

	Close(ds1302);

	return retval;
}


static inline uint8_t int8_t_to_bcd(int8_t val)
{
	uint8_t v = ((uint8_t) val) % 100;
	return ((v / 10) << 4) + (v % 10); 
}


static inline uint8_t int16_t_to_bcd(int16_t val)
{
	uint8_t v = ((uint16_t) val) % 100;
	return ((v / 10) << 4) + (v % 10); 
}


static const t_ds1302regs set_current_date(void)
{
	time_t t;
	struct tm tm;

	t_ds1302regs regs;
	memset(&regs, 0, sizeof(regs));
	/* repeat if second has changed while converting - try to be accurate within a second */
	do {
		t = time(NULL);
		localtime_r(&t, &tm);

		regs.sec  = int8_t_to_bcd(tm.tm_sec);
		regs.min  = int8_t_to_bcd(tm.tm_min);
		regs.hour = int8_t_to_bcd(tm.tm_hour);
		regs.date = int8_t_to_bcd(tm.tm_mday);
		regs.mon  = int8_t_to_bcd(tm.tm_mon+1);
		regs.wday = tm.tm_wday + 1;
		regs.year = int16_t_to_bcd(tm.tm_year);
	} while (t != time(NULL));
	return regs;
}

