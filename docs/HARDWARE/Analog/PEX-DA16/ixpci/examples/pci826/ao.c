/* Example of Digital I/O for PCI-826 PCI-822

   Author: WInson Chen

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

     This program shows digital io by ioctl register read/write command.

   v 0.0.0 8 Oct 2013 by Winson chen
     create */

#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<string.h>

#include"ixpci.h"

int main()
{
	int fd, m,tmp;
	char *dev_file, *ch;
	float n;
	ixpci_reg_t sda, rda, wao, rao, set_da;

	dev_file = "/dev/ixpci1";

	fd = open(dev_file, O_RDWR);
	
	/* enable/disable AO channel */
	set_da.id = IXPCI_ED_DA_CH;
	set_da.value = 3;

	printf("Choose channel:(0 or 1):");
	tmp = scanf("%d",&m);
	if(m != 0 && m != 1){
		printf("Error input\n");
		close(fd);
		return FAILURE;
	}

	/* AO channel select */
	sda.id = IXPCI_RS_DA_CS; 
	rda.id = IXPCI_RS_DA_CS;
	if(m == 0)
		sda.value = 0;
	if(m == 1)
		sda.value = 3;

	wao.id = IXPCI_RW_DA;
	rao.id = IXPCI_RW_DA;

	printf("Enter Voltage:");
	tmp = scanf("%f",&n);

	if(n == 10)
		wao.value = 3276.8 * (n + 10) - 1;
	else
		wao.value = 3276.8 * (n + 10);

	if(ioctl(fd, IXPCI_WRITE_REG, &set_da)){
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_ED_DA_CH.");
		close(fd);
		return FAILURE;
	}
	
	if(ioctl(fd, IXPCI_WRITE_REG, &sda)){
		puts("Failure of ioctl command IXPCI_WRITE_REG IXPCI_RS_DA_CS.");
		close(fd);
		return FAILURE;
	}

	if(ioctl(fd, IXPCI_READ_REG, &rda)){		
		puts("Failure of ioctl command IXPCI_READ_REG IXPCI_RS_DA_CS.");
		close(fd);
		return FAILURE;
	}

	if(rda.value == 3)
		ch = "ch1";
	else if(rda.value == 0)
		ch = "ch0";
	printf("The channel you choose is %s\n",ch);

	/* write DATA to AO */
	if(ioctl(fd, IXPCI_WRITE_REG, &wao)){
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RW_DA.");
		close(fd);
		return FAILURE;
	}

	if(ioctl(fd, IXPCI_READ_REG, &rao)){
		puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_RW_DA.");
		close(fd);
		return FAILURE;
	}
	//printf("value is %d\n",rao.value);
	printf("READ D/A DATA is %.2f\n",rao.value / 3276.8 - 10);

	close(fd);
	return SUCCESS;
}
