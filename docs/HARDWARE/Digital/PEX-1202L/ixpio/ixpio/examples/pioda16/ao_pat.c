/* Example of pattern analog output for PIO-DA16/DA8/DA4.

   Author: Reed Lai

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* File level history (record changes for this file here.)

   This program outputs analog patterns to channels

   	0 - a sine wave pattern,
	1 - a random pattern,
	2 - a FM that powered by 3 pattern,
	3 - a 50% AM pattern,
	4 - a 100% AM pattern,
	5 - a AM that 30% over modulated pattern.

   The analog output may be interfered by system interrupt latency since
   this is not a real-time implement.

   v 0.0.0 11 Dec 2002 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"
#include "_flags_inline.h"

#include "pattern.h"

#define INT 1  /* INT0 */
#define INT_EDGE 1  /* negative edge */

int main()
{
	int fd, sw;
	char *dev_file;
	ixpio_reg_t reg;
	ixpio_analog_t ao;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* install analog pattern */
	ao.channel = 0;
	ao.sig.is = INT;
	ao.sig.edge = INT_EDGE;
	ao.data.ptr = data1;
	ao.data_size = DATA1_SIZE;
	on_flag(&ao.flags, IXPIO_FLAG_ANALOG_PAT_START);
	if (ioctl(fd, IXPIO_ANALOG_OUT_PAT, &ao)) {
		close(fd);
		puts("Failure of analog pattern.");
		return FAILURE;
	}
	ao.channel = 1;
	//ao.sig.is = INT;
	//ao.sig.edge = INT_EDGE;
	ao.data.ptr = data2;
	ao.data_size = DATA2_SIZE;
	on_flag(&ao.flags, IXPIO_FLAG_ANALOG_PAT_START);
	if (ioctl(fd, IXPIO_ANALOG_OUT_PAT_ADD, &ao)) {
		close(fd);
		puts("Failure of analog pattern.");
		return FAILURE;
	}
	ao.channel = 2;
	//ao.sig.is = INT;
	//ao.sig.edge = INT_EDGE;
	ao.data.ptr = data3;
	ao.data_size = DATA3_SIZE;
	on_flag(&ao.flags, IXPIO_FLAG_ANALOG_PAT_START);
	if (ioctl(fd, IXPIO_ANALOG_OUT_PAT_ADD, &ao)) {
		close(fd);
		puts("Failure of analog pattern.");
		return FAILURE;
	}
	ao.channel = 3;
	//ao.sig.is = INT;
	//ao.sig.edge = INT_EDGE;
	ao.data.ptr = data4;
	ao.data_size = DATA4_SIZE;
	on_flag(&ao.flags, IXPIO_FLAG_ANALOG_PAT_START);
	if (ioctl(fd, IXPIO_ANALOG_OUT_PAT_ADD, &ao)) {
		close(fd);
		puts("Failure of analog pattern.");
		return FAILURE;
	}
	ao.channel = 4;
	//ao.sig.is = INT;
	//ao.sig.edge = INT_EDGE;
	ao.data.ptr = data5;
	ao.data_size = DATA5_SIZE;
	on_flag(&ao.flags, IXPIO_FLAG_ANALOG_PAT_START);
	if (ioctl(fd, IXPIO_ANALOG_OUT_PAT_ADD, &ao)) {
		close(fd);
		puts("Failure of analog pattern.");
		return FAILURE;
	}
	ao.channel = 5;
	//ao.sig.is = INT;
	//ao.sig.edge = INT_EDGE;
	ao.data.ptr = data6;
	ao.data_size = DATA6_SIZE;
	on_flag(&ao.flags, IXPIO_FLAG_ANALOG_PAT_START);
	if (ioctl(fd, IXPIO_ANALOG_OUT_PAT_ADD, &ao)) {
		close(fd);
		puts("Failure of analog pattern.");
		return FAILURE;
	}

	/***************************************
	 * setup 8254 counter-0 for INT_CHAN_0 *
	 *                                     *
	 * 4MHz / 200 = 20KHz                  *
	 ***************************************/
	reg.id = IXPIO_8254CW;
	reg.value = 0x36;			/* counter-0, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		puts("Failure of 8254 counter 0 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C0;
	reg.value = 0xc8;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		puts("Failure of 8254 counter 0 low byte.");
		return FAILURE;
	}
	reg.value = 0;  /* 0x00c8 == 100 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		puts("Failure of 8254 counter 0 high byte.");
		return FAILURE;
	}

	/* config board interrupt */
	reg.id = IXPIO_IMCR;
	reg.value = INT;

	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* start analog pattern */
	sw = IXPIO_DATA_START;
	if (ioctl(fd, sw)) {
		close(fd);
		puts("Failure of start analog pattern.");
		return FAILURE;
	}
	puts("Analog output: ON         <enter> to switch, ESC to exit.");

	/* press <enter> to switch, ESC to get out */
	while (getchar() != 27) {
		if (sw == IXPIO_DATA_START) {
			sw = IXPIO_DATA_STOP;
			puts("Analog output: OFF        <enter> to switch, ESC to exit.");
		} else {
			sw = IXPIO_DATA_START;
			puts("Analog output: ON         <enter> to switch, ESC to exit.");
		}
		if (ioctl(fd, sw)) {
			close(fd);
			puts("Failure of analog switch.");
			return FAILURE;
		}
	}

	if (ioctl(fd, IXPIO_KEEP_ALIVE)) {
		close(fd);
		puts("Failure of keep-alive.");
		return FAILURE;
	}

	close(fd);
	puts("End of program.");
	return SUCCESS;
}
