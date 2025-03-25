/* Example of analog output for PIO-DA16/DA8/DA4.

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

   This example shows the analog output by basic register read/write.

   v 0.0.0 10 Dec 2002 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

#define  MAX_BOARD_NUMBER     30

typedef unsigned short WORD;
typedef char BYTE;
static float PIODA_fDeltaV[MAX_BOARD_NUMBER][16];
static float PIODA_fDeltaI[MAX_BOARD_NUMBER][16];
static WORD  PIODA_wN10V[MAX_BOARD_NUMBER][16];
static WORD  PIODA_wP10V[MAX_BOARD_NUMBER][16];
static WORD  PIODA_w00mA[MAX_BOARD_NUMBER][16];
static WORD  PIODA_w20mA[MAX_BOARD_NUMBER][16];
static WORD  PIODA_EEP;

WORD PIODA_ReadEEPROMAll(WORD);
WORD PIODA_EEP_READ(WORD, WORD, WORD *, WORD *);
WORD PIODA_COMM(WORD);
WORD PIODA_WR_BYTE(WORD, WORD);
WORD PIODA_RD_BYTE(WORD, WORD *);
WORD PIODA_OutputByte(WORD, ixpio_reg_t *);
WORD PIODA_InputByte(WORD, ixpio_reg_t *);

int main()
{
  int i;
  WORD bHi, bLo;
  WORD wTmp;
  WORD wBase;
  WORD wCh;
  
  int fd;
  char *dev_file;
  ixpio_reg_t cs, aol, aoh;
  ixpio_reg_t reg;
  unsigned int ao_value;

  dev_file = "/dev/ixpio1";

  /* open device file */
  fd = open(dev_file, O_RDWR);
  if (fd < 0)
  {
    printf("Failure of open device file \"%s.\"\n", dev_file);
    return FAILURE;
  }

  /* AUX 4/3/2 are DO, others DI */
  reg.value = 0x1c;                      
  reg.id = IXPIO_ACR;

  if (PIODA_OutputByte(fd, &reg)) return FAILURE;
 
  /* ALL DO are low */
  reg.value = 0x00;                      
  reg.id = IXPIO_ADR;

  if (PIODA_OutputByte(fd, &reg)) return FAILURE;
 
  //************************************************************
  // Read calibration data from EEPROM
  //************************************************************

  WORD wBoardNo = fd;
  for(i=0; i<64; i++)
  {
     PIODA_EEP_READ(fd,(WORD)(i),&bHi,&bLo);
     if( i<16 )
     {
        PIODA_wN10V[wBoardNo][i]=(WORD)(bHi*256 + bLo);
     }
     else if( (i>=16) && (i<32) )
     {
        PIODA_wP10V[wBoardNo][i-16]=(WORD)(bHi*256 + bLo);
     }
     else if( (i>=32) && (i<48) )
     {
        PIODA_w00mA[wBoardNo][i-32]=(WORD)(bHi*256 + bLo);
     }
     else if( i>=48 )
     {
        PIODA_w20mA[wBoardNo][i-48]=(WORD)(bHi*256 + bLo);
     }
  }

  for(i=0; i<16; i++)
  {
    printf("N10V %d : %d\n", i, PIODA_wN10V[wBoardNo][i]);
    printf("P10V %d : %d\n", i, PIODA_wP10V[wBoardNo][i]);
    printf("W00MA %d : %d\n", i, PIODA_w00mA[wBoardNo][i]);
    printf("W20MA %d : %d\n", i, PIODA_w20mA[wBoardNo][i]);
    printf("\n");
  }

  close(fd);
  puts("\nEnd of program.");
  return SUCCESS;
}


WORD PIODA_EEP_READ(WORD fd, WORD wOffset, WORD *bHi, WORD *bLo)
{
    WORD wBase, wHi, wLo;
    ixpio_reg_t reg;

    PIODA_COMM(fd);

    // read command & address
    wOffset=(WORD)(wOffset|0x80);
    PIODA_WR_BYTE(fd,wOffset);

    PIODA_EEP=(WORD)(PIODA_EEP&0x0c);
    reg.id = IXPIO_ADR;
    reg.value = PIODA_EEP;

    if (PIODA_OutputByte(fd, &reg)) return FAILURE;
   
    PIODA_EEP=(WORD)(PIODA_EEP&0x14);
    reg.value = PIODA_EEP;

    if (PIODA_OutputByte(fd, &reg)) return FAILURE;
   
    PIODA_EEP=(WORD)(PIODA_EEP|0x08);
    reg.value = PIODA_EEP;

    if (PIODA_OutputByte(fd, &reg)) return FAILURE;

    PIODA_EEP=(WORD)(PIODA_EEP&0x14);
    reg.value = PIODA_EEP;
    
    if (PIODA_OutputByte(fd, &reg)) return FAILURE;
   
    PIODA_RD_BYTE(fd, &wHi);
        *bHi = (BYTE) wHi;
    // read low  8-bit data
    PIODA_RD_BYTE(fd, &wLo);
        *bLo = (BYTE) wLo;

    PIODA_EEP=(WORD)(PIODA_EEP&0x14);
    reg.value = PIODA_EEP;

    if (PIODA_OutputByte(fd, &reg)) return FAILURE;
   
    PIODA_EEP=(WORD)(PIODA_EEP&0x18);
    reg.value = PIODA_EEP;

    if (PIODA_OutputByte(fd, &reg)) return FAILURE;
   
    return SUCCESS;
}

// send start 2-bit (0,1)
//-----------------------------------------------
// AUX2=D/O=CS of EEP          AUX3=D/O=SK of EEP
// AUX4=D/O=DI of EEP          AUX5=D/I=DO of EEP
//-----------------------------------------------
WORD PIODA_COMM(WORD fd)
{
  ixpio_reg_t reg;

  PIODA_EEP=0x04;
  reg.value = PIODA_EEP;
  reg.id = IXPIO_ADR;
  if (PIODA_OutputByte(fd, &reg)) return FAILURE;

  PIODA_EEP=0x0c;
  reg.value = PIODA_EEP;

  if (PIODA_OutputByte(fd, &reg)) return FAILURE;
 
  PIODA_EEP=0x04;
  reg.value = PIODA_EEP;
  if (PIODA_OutputByte(fd, &reg)) return FAILURE;
 
  PIODA_EEP=0x14;
  reg.value = PIODA_EEP;
  if (PIODA_OutputByte(fd, &reg)) return FAILURE;

  PIODA_EEP=0x1c;
  reg.value = PIODA_EEP;

  if (PIODA_OutputByte(fd, &reg)) return FAILURE;

  return SUCCESS;
}

WORD PIODA_WR_BYTE(WORD fd, WORD bData)
{
    WORD   i,j;
    ixpio_reg_t reg;

    for(i=0; i<8; i++)
    {
        PIODA_EEP=(WORD)(PIODA_EEP&0x14);
        reg.value = PIODA_EEP;
        reg.id = IXPIO_ADR;

        if (PIODA_OutputByte(fd, &reg)) return FAILURE;
       
        j=(WORD)(bData&0x80);
        bData=(WORD)(bData<<1);

        if( j==0 )         // DI=0
        {
           PIODA_EEP=(WORD)(PIODA_EEP&0x0c);
           reg.value = PIODA_EEP;
           if (PIODA_OutputByte(fd, &reg)) return FAILURE;
        }
        else               // DI=1
        {
           PIODA_EEP=(WORD)(PIODA_EEP|0x10);
           reg.value = PIODA_EEP;

           if (PIODA_OutputByte(fd, &reg)) return FAILURE;
        }

        PIODA_EEP=(WORD)(PIODA_EEP|0x08);
        reg.value = PIODA_EEP;

        if (PIODA_OutputByte(fd, &reg)) return FAILURE;
    }

    return SUCCESS;
}

// read the next 8-bit
WORD PIODA_RD_BYTE(WORD fd, WORD *bData)
{
    BYTE i,j,k,Val;
    ixpio_reg_t reg, reg2;
    Val=0x80;
    k=0;

    for(i=0; i<8; i++)
    {
        PIODA_EEP=(WORD)(PIODA_EEP&0x14);
        reg.value = PIODA_EEP;
        reg.id = IXPIO_ADR;
  
        if (PIODA_OutputByte(fd, &reg)) return FAILURE;
       
        reg2.id = IXPIO_APSR;
        if (PIODA_InputByte(fd, &reg2)) return FAILURE;
       
        j=(BYTE)((reg2.value)&0x20);
        if( j!=0 ) k+=Val;     // DO=1

        PIODA_EEP=(WORD)(PIODA_EEP|0x08);
        reg.value = PIODA_EEP;
        if (PIODA_OutputByte(fd, &reg)) return FAILURE;
       
        Val=(BYTE)(Val>>1);
    }

    *bData = k;

    return SUCCESS;
}

WORD PIODA_OutputByte(WORD fd, ixpio_reg_t *p_reg)
{
   if (ioctl(fd, IXPIO_WRITE_REG, p_reg))
   {
     close(fd);
     return FAILURE;
   }

   return SUCCESS;
}

WORD PIODA_InputByte(WORD fd, ixpio_reg_t *p_reg)
{
  if (ioctl(fd, IXPIO_READ_REG, p_reg))
  {
    close(fd);
    return FAILURE;
  }

  return SUCCESS;
}

