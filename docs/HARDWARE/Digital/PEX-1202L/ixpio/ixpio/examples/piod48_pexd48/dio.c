/* Example of I/O port for PIO-D48.

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

   v 0.0.0 30 Nov 2019 by Winson Chen
     create, set dio port and test input & output */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <unistd.h>

#include "ixpio.h"

#define PA 0
#define PB 1
#define PC 2

char* port_direction(int, int, int);
int getch();
void usage();

int port_control[2][3];

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t reg;
	unsigned int old = -1;
	char cmd;
	int cn, tmp, val, do_val, i, j;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* Init as input port */
	for(i = 0; i <= 1; i++)
	{
		for(j = 0; j<=2; j++)
			port_control[i][j] = 1;
	}

	usage();

	while(1)
	{
		cmd = getch();

		switch(cmd)
		{
			case 'a':
			{
				printf("1 means Input port, 0 means Output port.\n");
				printf("-------------------------------------------------\n");
				printf("|Bit 7|Bit 6|Bit 5|Bit 4|Bit 3|Bit 2|Bit 1|Bit 0|\n");
				printf("-------------------------------------------------\n");
				printf("|1    |0    |0    |PA   |PC-H |0    |PB   |PC-L |\n");
				printf("-------------------------------------------------\n\n");
				printf("Select CN1 or CN2 then enter value to config ports:\n");
				printf("1. CN1\n2. CN2\n");
				tmp = scanf("%d",&cn);
				getchar();

				/* Select CN1 or CN2 */
				if(cn == 1)
					reg.id = IXPIO_82551CW;
				else if(cn == 2)
					reg.id = IXPIO_82552CW;
				else
				{
					printf("Wrong value, break to menu.\n");
					break;
				}

				/* Configure PA, PB, PC as input/output port */
				printf("Enter Value to config port: ");
				tmp = scanf("%d",&val);
				getchar();
				while(!(val & 0x80))
				{
					printf("Bit 7 Must be 1\n");
					printf("Enter Value: ");
					tmp = scanf("%d",&val);
					getchar();
				}

				reg.value = val;
				if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
					printf("Failure of ioctl command IXPIO_WRITE_REG IXPIO_82551/2CW.\n");
					close(fd);
					return FAILURE;
				}

				printf("CN%d PA set %s port\n", cn, port_direction(cn, PA, val & 0x10));
				printf("CN%d PB set %s port\n", cn, port_direction(cn, PB, val & 0x02));
				if((val & 0x08) && (val & 0x01))
					printf("CN%d PC set %s port\n", cn, port_direction(cn, PC, 1));
				else
					printf("CN%d PC set %s port\n", cn, port_direction(cn, PC, 0));
			}
			break;
			case 'b':
			{
				printf("1. CN1 PA\n2. CN1 PB\n3. CN1 PC\n4. CN2 PA\n5. CN2 PB\n6. CN2 PC\n");
				printf("Select port to test Digital Output: ");
				tmp = scanf("%d",&i);
				getchar();
				switch(i)
				{
					case 1:
					{
						if((port_control[0][0]))
						{
							printf("CN1 PA is input port, break\n");
							continue;
						}
						reg.id = IXPIO_82551PA;
					}
					break;
					case 2:
					{
						if((port_control[0][1]))
						{
							printf("CN1 PB is input port, break\n");
							continue;
						}
						reg.id = IXPIO_82551PB;
					}
					break;
					case 3:
					{
						if((port_control[0][2]))
						{
							printf("CN1 PC is input port, break\n");
							continue;
						}
						reg.id = IXPIO_82551PC;
						if(val & 0x08)
							printf("Notice CN1 PC high nibble is DI port\n");
						if(val & 0x01)
							printf("Notice CN1 PC low nibble is DI port\n");
					}
					break;
					case 4:
					{
						if((port_control[1][0]))
						{
							printf("CN2 PA is input port, break\n");
							continue;
						}
						reg.id = IXPIO_82552PA;
					}
					break;
					case 5:
					{
						if((port_control[1][1]))
						{
							printf("CN2 PB is input port, break\n");
							continue;
						}
						reg.id = IXPIO_82552PB;
					}
					break;
					case 6:
					{
						if((port_control[1][2]))
						{
							printf("CN2 PC is input port, break\n");
							continue;
						}
						reg.id = IXPIO_82552PC;
						if(val & 0x08)
							printf("Notice CN2 PC high nibble is DI port\n");
						if(val & 0x01)
							printf("Notice CN2 PC low nibble  is DI port\n");
					}
					break;
				}

				printf("Set DO value(0~255): ");
				tmp = scanf("%d",&do_val);
				getchar();
				reg.value = do_val;
				
				if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
					printf("Failure of ioctl command IXPIO_WRITE_REG IXPIO_82551CW.\n");
					close(fd);
					return FAILURE;
				}
				printf("Set DO value done.\n");
			}
			break;
			case 'c':
			{
				printf("1. CN1 PA\n2. CN1 PB\n3. CN1 PC\n4. CN2 PA\n5. CN2 PB\n6. CN2 PC\n");
                                printf("Select port to test Digital Input: ");
                                tmp = scanf("%d",&i);
                                getchar();
                                switch(i)
                                {
                                        case 1:
                                        {
                                                if(!(port_control[0][0]))
                                                {
                                                        printf("CN1 PA is output port, break\n");
                                                        continue;
                                                }
                                                reg.id = IXPIO_82551PA;
                                        }
                                        break;
                                        case 2:
                                        {
                                                if(!(port_control[0][1]))
                                                {
                                                        printf("CN1 PB is output port, break\n");
                                                        continue;
                                                }
                                                reg.id = IXPIO_82551PB;
                                        }
                                        break;
                                        case 3:
                                        {
                                                if(!(port_control[0][2]))
                                                {
                                                        printf("CN1 PC is output port, break\n");
                                                        continue;
                                                }
                                                reg.id = IXPIO_82551PC;
						if(!(val & 0x08))
                                                        printf("Notice CN1 PC high nibble is DO port\n");
                                                if(!(val & 0x01))
                                                        printf("Notice CN1 PC low nibble is DO port\n");
                                        }
                                        break;
                                        case 4:
                                        {
                                                if(!(port_control[1][0]))
                                                {
                                                        printf("CN2 PA is output port, break\n");
                                                        continue;
                                                }
                                                reg.id = IXPIO_82552PA;
                                        }
                                        break;
                                        case 5:
                                        {
                                                if(!(port_control[1][1]))
                                                {
                                                        printf("CN2 PB is output port, break\n");
                                                        continue;
                                                }
                                                reg.id = IXPIO_82552PB;
                                        }
                                        break;
                                        case 6:
                                        {
                                                if(!(port_control[1][2]))
                                                {
                                                        printf("CN2 PC is output port, break\n");
                                                        continue;
                                                }
                                                reg.id = IXPIO_82552PC;
						if(!(val & 0x08))
                                                        printf("Notice CN2 PC high nibble is DO port\n");
                                                if(!(val & 0x01))
                                                        printf("Notice CN2 PC low nible is DO port\n");
                                        }
                                        break;
				}

					printf("End loop while Bit 7 is enable\n");
					while (!ioctl(fd, IXPIO_READ_REG, &reg) && !(reg.value & 0x80)) {
						if (old != reg.value) {
							old = reg.value;
							printf("port input value = 0x%x\n", reg.value);
						}
					}
					printf("Exit while loop\n");
			}
			break;
			case 'p':
				usage();
			break;
			case 'q':
				printf("Exit.\n");
			break;
			default:
				printf("Invalid option.(p to list, q to quit)\n");
			break;
		}

		if(cmd == 'q')
			break;
	}

	close(fd);
	return SUCCESS;
}

char* port_direction(int cn, int port, int x)
{
	char* out = "output";
	char* in = "input";

	if(x)
	{
		port_control[cn-1][port] = 1;
		return	in;
	}
	else
	{
		port_control[cn-1][port] = 0;
		return	out;
	}
}

int getch(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

void usage()
{
	printf("a. Program port0 to port5 as DI or DO port. (Default setting is DI port.)\n");
	printf("b. Digital Output\n");
	printf("c. Digital Input\n");
	printf("p. Show function menu.\n");
	printf("q. Quit this demo.\n\n");
}
