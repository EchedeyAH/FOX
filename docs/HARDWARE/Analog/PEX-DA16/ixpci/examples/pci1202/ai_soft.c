/* Read ADC result by software trigger.

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

   This program shows the AI by software trigger.

   v 0.0.0 21 Jan 2003 by Reed Lai
     Create */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpci.h"

int main()
{
	int fd, config;
	char *dev_file;
	float av;
	ixpci_reg_t reg, rad;

	dev_file = "/dev/ixpci1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	reg.id = IXPCI_SR;	//bar2 status register 0x00 winson
	if (ioctl(fd, IXPCI_READ_REG, &reg)) {
		puts("Failure of Status Register");
		close(fd);
		return FAILURE;
	}
	if ((reg.value & 0x04) == 0) {
		reg.id = IXPCI_CR;	//bar2 control register 0x00 winson
		reg.value = 0xffff;	/* send a recorery to PIC */
		if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
			puts("Failure of Control Register");
			close(fd);
			return FAILURE;
		}
	}

	reg.id = IXPCI_SR;	//wait for handshake. winson
	if (ioctl(fd, IXPCI_READ_REG, & reg)) {
		puts("Failure of Status Register");
		close(fd);
		return FAILURE;
	}
	while ((reg.value & 0x04) ==0) {
		ioctl(fd, IXPCI_READ_REG, &reg);
	}
	
	/* channel = 0, gain = 1 */
	reg.id = IXPCI_CR;
	config = 0x8403;
	/* set pic low */
	config &= 0xdfff;
	reg.value = config; 
	
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of set channel & gain");
		close(fd);
		return FAILURE;
	}
	
	reg.id = IXPCI_SR;
	ioctl(fd, IXPCI_READ_REG, &reg);
	while ((reg.value & 0x04) != 0 ) {
		ioctl(fd, IXPCI_READ_REG, &reg);
	}
	
	/* set pic high */
	reg.id = IXPCI_CR;
	reg.value = config | 0x2000;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Faiulre of set pic high");
		close(fd);
		return FAILURE;
	}

	reg.id = IXPCI_SR;
	ioctl(fd, IXPCI_READ_REG, &reg);
	while((reg.value & 0x04) == 0) {
		ioctl(fd, IXPCI_READ_REG, &reg);
	}
	
	usleep(3); /* delay 3 us setting time */
	
	/* CLEAR FIFO */
	reg.id = IXPCI_CR;
	/* B15=0=clear FIFO, B13=1=not MagicScan controller cmd */
	reg.value = 0x2000;			
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of clear FIFO.");
		close(fd);
		return FAILURE;
	}

	/* B15=1=no clear FIFO, B13=1=not MagicScan controller cmd */
	reg.value = 0xA000;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of Control Register.");
		close(fd);
		return FAILURE;
	}
			
	/* software trigger mode */
	reg.id = IXPCI_ADST;
	reg.value = 0xffff;	// generate a software trigger pulse. winson
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of software trigger mode.");
		close(fd);
		return FAILURE;
	}

	/* wait for ready signal */
	reg.id = IXPCI_SR;
	ioctl(fd, IXPCI_READ_REG, &reg);
	while ((reg.value & 0x20) == 0) {
		ioctl(fd, IXPCI_READ_REG, &reg);
	}
	
	rad.id = IXPCI_AD;

	if (ioctl(fd, IXPCI_READ_REG, &rad)) {
		puts("Failure of analog input.");
		close(fd);
		return FAILURE;
	}
	
	//av = 10 * (float)rad.value / 0xfff - 5;
	av = ((float)rad.value-2048)/2048.0*5;	//winson
	printf("ADC read = 0x%04x ==> %fV\n ", rad.value, av);

	close(fd);
	return SUCCESS;
}
