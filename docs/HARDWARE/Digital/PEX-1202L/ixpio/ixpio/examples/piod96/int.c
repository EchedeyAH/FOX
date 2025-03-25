/* Example of interrupt handling for PIO-D96. This program configures
   driver to send signals in same signal id for the four interrupt
   channels.

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

   v 0.0.0  5 Nov 2002 by Reed Lai
     create, blah blah... */
#include <stdio.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"

/* custom signal id, 33-63 */
#define MY_SIG 40

void sig_handler(int sig)
{
        static unsigned sig_counter;
        printf("\rGot single %d for %u times \n", sig, ++sig_counter);
}

int main()
{
        int fd,index;
        char *dev_file;
        ixpio_reg_t reg,reg1;
        ixpio_signal_t sig;

        static struct sigaction act, act_old;

        dev_file = "/dev/ixpio1";

        /* open device file */
        fd = open(dev_file, O_RDWR);
        if (fd < 0) {
                printf("Failure of open device file \"%s.\"\n", dev_file);
                return FAILURE;
        }

        /* set action for signal */
        act.sa_handler = sig_handler;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, MY_SIG);
        if (sigaction(MY_SIG, &act, &act_old)) {
                close(fd);
                puts("Failure of signal action.");
                return FAILURE;
        }

	/* port configuration */
        reg.id = IXPIO_PCA;
        reg.value = 2;                  /* Port 1 as DO, Port 2 as DI */
        if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of configuring port.");
                return FAILURE;
        }

        /* configure board interrupt */
        reg.id = IXPIO_IMCR;

	/* enable INT_CHAN_0 for interrupt */
        reg.value = 0x01;

        if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of configuring interrupt.");
                return FAILURE;
        }

	/* signal condiction */
        sig.sid = MY_SIG;
        sig.pid = getpid();
        sig.is = 0x01;   /* signal for the P2C0 channels */
        sig.edge = 0x01;  /* high level trigger */
        if (ioctl(fd, IXPIO_SET_SIG, &sig)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of signal condiction.");
                return FAILURE;
        }

        /* wait for exit */
        reg1.id = IXPIO_P1;
        reg1.value = 1;

        puts("press <Esc> to exit\n");

        index = 8;

	while (index)
        {
          printf("DI value : 0x%x\n",reg1.value);
          if( ioctl(fd, IXPIO_WRITE_REG, &reg1) )
          {
            puts("Failure of IXPIO_READ/WRITE_REG");
            close(fd);
            return FAILURE;
          }

          if (reg1.value == 8)
          {
	    if ( index == 5 )
            {
              printf("\n======== run again ========\n");
            }
            else if ( index == 1 )
            {
              printf("\n========    end    ========\n");
            }

            reg1.value = 1;
          }
          else
          {
            reg1.value <<= 1;
          }

          index--;

          sleep(1);
        }

        close(fd);
        sigaction(MY_SIG, &act_old, NULL);
        puts("\nEnd of program.");
        return SUCCESS;
}
