/************************************************************************
 *                                                                      *
 *  piod48.c  ---  real time daq kernel functions.                       *
 *                                                                      *
 *  This file is open source software; you can redistribute it and/or   *
 *  modify it under the terms of the version 2 of GNU General Public    *
 *  License as published by the Free Software Foundation.               *
 *                                                                      *
 *  This program is distributed in the hope that it will be useful,     *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 *  GNU General Public License for more details.                        *
 *                                                                      *
 *  You should have received a copy of the GNU General Public License   *
 *  along with this package; if not, write to the Free Software         *
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.           *
 *  or look at http://www.gnu.org/fsf/fsf.html                          *
 *                                                                      *
 *  Copyleft, Peter Wurmsdobler, peterw@thinkingnerds.com               *
 *                                                                      *
 ************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "piod48.h"

/***********************************************************************
 * some general things                                                 *
 ***********************************************************************/

/* Debug macros for DAQ piod48 PIOD48
 */
//#define DEBUG
#ifdef DEBUG
    #define DPRINTK(args...)    printk(## args)
#else
    #define DPRINTK(args...)
#endif

/************************************************************************
 *                                                                      * 
 *  Settings on the DAQ piod48 the following driver definitions are     *
 *  valid for:                                                          *
 *                                                                      *
 *    JP#    Pos       Meaning                                          *
 *                                                                      * 
 ************************************************************************/

#define PIOD48_NAME  "piod48: "

/************************************************************************
 *  piod48 initialisation                                                * 
 ************************************************************************/

/* PCI stuff
 */
#define PIOD48_VENDOR_ID    0x0000e159  /* as used in pci_find_device */
#define PIOD48_DEVICE_ID    0x00000002  /* as used in pci_find_device */
#define PIOD48_SUBAUX_ID    0x00800130  /* as used in driver */
#define PIOD48_SUBAUX_MASK  0xf0        /* look into code */
#define PIOD48_SUBAUX_ADDR  0x0007      /* look into code */

/* register range and offsets
 */
#define PIOD48_RANGE              0x0100 /* base address range */
#define PIOD48_RESET_ADDR         0x0000 /* reset register     */
#define PIOD48_CONTROL_ADDR       0x0000 /* control register */
#define PIOD48_AUX_CTRL_ADDR      0x0002 /* auxiliary control register */
#define PIOD48_AUX_DATA_ADDR      0x0003 /* auxiliary data register */
#define PIOD48_AUX_STAT_ADDR      0x0007 /* auxiliary pin status register */
#define PIOD48_INT_MASK_ADDR      0x0005 /* interrupt mask register */
#define PIOD48_INT_POLARITY_ADDR  0x002a /* interrput polarity register*/
#define PIOD48_INT_CONTROL_ADDR   0x00f0 /* interrput control register*/

#define PIOD48_INT_MASK_SHIFT     0x10


/************************************************************************
 *  control stuff                                                       * 
 ************************************************************************/

/* board enable
 */
#define PIOD48_ENABLE   0x01 /* after boot, enable board */
#define PIOD48_DISABLE  0x00 /* disable at shutdown      */


/************************************************************************
 *  io stuff                                                            * 
 ************************************************************************/

/* DIO register range and offsets (8255 dio chip)
 */
#define PIOD48_DIO_CHIPS     2       /* number of DIO nchips    */
#define PIOD48_DIO_PPERC     3       /* number of ports/nchip   */
#define PIOD48_DIO_PORTS     ( PIOD48_DIO_CHIPS*PIOD48_DIO_PPERC )
#define PIOD48_DIO_NBITS     8       /* number of bits/PORT     */
#define PIOD48_DIO_BASES     { 0x00c0, 0x00d0 } /* chip base addr */
#define PIOD48_DIO_DADDR     { 0x0000, 0x0004, 0x0008 }
#define PIOD48_DIO_CADDR     0x000c  /* chips control register  */
#define PIOD48_DIO_OCODE     0x80;   /* output code */
#define PIOD48_DIO_ICODE     { 0x90, 0x82, 0x89 } /* input code */


/************************************************************************
 *  timer/counter stuff                                                 * 
 ************************************************************************/

/* counter chips (8254 timer/counter chip) 
 */
#define PIOD48_COUNTER_CHIPS 1       /* number of counter chips     */
#define PIOD48_COUNTER_CPERC 3       /* number of counter per chip  */
#define PIOD48_COUNTER_TOTAL ( PIOD48_COUNTER_CPERC*PIOD48_COUNTER_CHIPS )
#define PIOD48_COUNTER_BASES { 0x00e0 } /* chip address             */
#define PIOD48_COUNTER_DADDR { 0x0000, 0x0004, 0x0008 }
#define PIOD48_COUNTER_CADDR  0x000c  /* COUNTER control             */

/* counter control register words (8254 timer/counter) 
 */
#define PIOD48_COUNTER_BIN   0x00    /* binary count mode             */
#define PIOD48_COUNTER_BCD   0x01    /* BCD count mode                */
#define PIOD48_COUNTER_LATCH 0x00    /* counter latch instruction     */
#define PIOD48_COUNTER_LO    0x10    /* read/write low counter byte   */
#define PIOD48_COUNTER_HI    0x20    /* read/write high counter byte  */
#define PIOD48_COUNTER_LOHI  0x30    /* read/write low, then high     */
#define PIOD48_COUNTER_SEL   { 0x00, 0x40, 0x80 } /* select counter i */
#define PIOD48_COUNTER_RBACK 0xc0    /* counter read back             */

/* in general, there are some 8254 timer/counter with several counters 
 * each. Some counters are used as such, others are used as timers, single
 * or by combination of two counters. For AI, a timer is usually used
 * to serve as pacer. For an application one has:
 */
#define PIOD48_TIMER_BASE    500     /* Clock time in nanosecs        */
#define PIOD48_TIMER_32      1       /* available 32bit timers        */
#define PIOD48_TIMER_32C     {1,2}   /* counter used for 32 bit timer */


/************************************************************************
 * global values                                                        * 
 ************************************************************************/

struct
    {
    unsigned char dio_port[PIOD48_DIO_PORTS];
    unsigned char dio_type[PIOD48_DIO_PORTS];
    unsigned char dio_code[PIOD48_DIO_PORTS];
    unsigned long  int base_phys;
    unsigned short int base_io;
    unsigned short int irq;
    unsigned int  int_mask;
    unsigned char int_source;
    unsigned int  int_polarity;
    } 
    piod48;


/***********************************************************************
 * Control functions ...                                               *
 ***********************************************************************/

/* program interrupt with mask and source
 */
void piod48_int_select( unsigned int mask, unsigned int source )
    {
    piod48.int_mask = mask;
    piod48.int_source = source;
    piod48.int_polarity = PIOD48_INTX_NONINV;

    outb( piod48.int_source||PIOD48_INT2_PC0NINV||PIOD48_COUNTER_2MHZ,
        piod48.base_io+PIOD48_INT_CONTROL_ADDR );
    outb( piod48.int_polarity, 
        piod48.base_io+PIOD48_INT_POLARITY_ADDR );
    outb( piod48.int_mask, 
        piod48.base_io+PIOD48_INT_MASK_ADDR );
    }

/* invert polarity
 */
unsigned int piod48_inv_polarity( void )
    {
    unsigned int mask;      /* mask for test */
    unsigned int status;    /* AUX Pin Status Register */
    unsigned int source;    /* interrupt source  */
    unsigned int edge;      /* interrupt source edge */

    edge   = 0;
    source = 0;
    status = inb( piod48.base_io+PIOD48_AUX_STAT_ADDR ) & 0x0f;

    for ( mask = 1; mask < PIOD48_INT_MASK_SHIFT; mask <<= 1 )
        {
        if ( piod48.int_polarity & mask ) /* non-inverse mode */
            {
            if ( status & mask )
                {
                source |= mask;    /* interrupt source */
                edge   |= mask;    /* positive edge    */
                }
            }
        else /* inverse mode */
            {
            if (!(status & mask))
                {
                source |= mask;    /* interrupt source */
                edge   &= !mask;   /* negative edge    */
                }
            }
        }

    piod48.int_polarity ^= source; /* Inverts the polarity of interrupted */
    outb( piod48.int_polarity, 
        piod48.base_io + PIOD48_INT_POLARITY_ADDR ); /* source for next edge. */

    return edge;
    }

/* get number of interrupt line, irq handler can be defined outside
 */
int piod48_irq_get( void )
    {
    return piod48.irq;
    }

/* irq handler for piod48 
 */
unsigned int piod48_irq_handler( unsigned int irq_number, 
    struct pt_regs *p )
    {
    /* nothing done her, use this template in your real time application !
     */
    
    return 0;
    }

/* start counter "counter" with divisor and mode
 */
void piod48_counter_set( unsigned int counter, 
    unsigned int divisor, unsigned char mode )
    {
    int nchan, nchip;
    unsigned short int counter_caddr[PIOD48_COUNTER_CHIPS] = PIOD48_COUNTER_BASES;
    unsigned short int counter_daddr[PIOD48_COUNTER_CPERC] = PIOD48_COUNTER_DADDR;
    unsigned char counter_sel[PIOD48_COUNTER_CPERC] = PIOD48_COUNTER_SEL;
    
    counter %= PIOD48_COUNTER_TOTAL;
    nchip   = counter / PIOD48_COUNTER_CPERC;
    nchan   = counter % PIOD48_COUNTER_CPERC;
    
    DPRINTK("starting counter[%d] with %d \n",counter,divisor);

    outb( (mode|counter_sel[nchan]|PIOD48_COUNTER_LOHI|PIOD48_COUNTER_BIN), 
        piod48.base_io+counter_caddr[nchip]+PIOD48_COUNTER_CADDR );
    outb( (unsigned char)(divisor),	   
        piod48.base_io+counter_caddr[nchip]+counter_daddr[nchan] );
    outb( (unsigned char)(divisor>>8),
        piod48.base_io+counter_caddr[nchip]+counter_daddr[nchan] );
    }

/* start timer using two coutners, with "time" in nanosecs and mode!
 */
unsigned int piod48_timer32_start( unsigned int timer,
    unsigned int time, unsigned char mode )
    {
    unsigned int divisor,d1,d2;
    int timer_32c[PIOD48_TIMER_32*2] = PIOD48_TIMER_32C;
    
    timer  %= PIOD48_TIMER_32;
    divisor = time/PIOD48_TIMER_BASE;
    DPRINTK("divisor[%d] = %d\n",timer,divisor);
    
    d1 = divisor/0x0ffff+1;
    if ( d1 < 0x00002 ) d1 = 2;
    if ( d1 > 0x0ffff ) d1 = 0x0ffff;
    d2 = divisor/d1;
    if ( d2 < 0x00002 ) d2 = 2;
    if ( d2 > 0x0ffff ) d2 = 0x0ffff;

    time    = d1*d2*PIOD48_TIMER_BASE;   
    DPRINTK("starting timer[%d] at %d ns\n",timer,time);
    
    piod48_counter_set( timer_32c[timer*2], 
        (unsigned short int)(d1), mode );
    piod48_counter_set( timer_32c[timer*2+1], 
        (unsigned short int)(d2), mode );

    return divisor*PIOD48_TIMER_BASE;
    }

/* stop timer using coutners
 */
void piod48_timer32_stop( unsigned int timer )
    {
    int timer_32c[PIOD48_TIMER_32*2] = PIOD48_TIMER_32C;
    
    timer %= PIOD48_TIMER_32;
    piod48_counter_set( timer_32c[timer*2],   0x0000, 0x00 );
    piod48_counter_set( timer_32c[timer*2+1], 0x0000, 0x00 );
    }


/***********************************************************************
 * Digital input/output basic functions ...                            *
 ***********************************************************************/

/* piod48_dio_set_byte  ---  set byte at port 0-5 by "byte".
 */ 
void piod48_dio_set_byte( unsigned char byte,
    unsigned short int nport )
    {
    int nchan, nchip;
    unsigned short int dio_caddr[PIOD48_DIO_CHIPS] = PIOD48_DIO_BASES;
    unsigned short int dio_daddr[PIOD48_DIO_PPERC] = PIOD48_DIO_DADDR;

    nport %= PIOD48_DIO_PORTS;        /* just in case      */
    nchip =  nport/PIOD48_DIO_PPERC;  /* number of chip    */
    nchan =  nport%PIOD48_DIO_PPERC;  /* number of channel */

    if ( piod48.dio_type[nport] == PIOD48_DIO_OUTPUT )
        {
        piod48.dio_port[nport] = byte;
        
        outb( piod48.dio_port[nport], 
            piod48.base_io+dio_caddr[nchip]+dio_daddr[nchan] );
        }
    }

/* set bit at port, channel 0-7, by "bit" 0 or 1.
 */ 
void piod48_dio_set_bit( unsigned short int bit,
    unsigned short int nport, unsigned short int nbit )
    {
    nport %= PIOD48_DIO_PORTS; /* just in case */
    nbit  %= PIOD48_DIO_NBITS; /* just in case */
    bit   &= 0x01;            /* just in case */

    piod48.dio_port[nport] &= ~(1 << nbit);
    piod48.dio_port[nport] |=  (bit << nbit); 

    if ( piod48.dio_type[nport] == PIOD48_DIO_OUTPUT ) /* just in case */
        piod48_dio_set_byte( piod48.dio_port[nport], nport );
    }
    
/* piod48_dio_get_byte  ---  get byte at port 0-5 into "byte"
 */ 
unsigned char piod48_dio_get_byte( unsigned short int nport )
    {
    int nchan, nchip;
    unsigned short int dio_caddr[PIOD48_DIO_CHIPS] = PIOD48_DIO_BASES;
    unsigned short int dio_daddr[PIOD48_DIO_PPERC] = PIOD48_DIO_DADDR;

    nport %= PIOD48_DIO_PORTS;        /* just in case      */
    nchip =  nport/PIOD48_DIO_PPERC;  /* number of chip    */
    nchan =  nport%PIOD48_DIO_PPERC;  /* number of channel */
    
    if ( piod48.dio_type[nport] == PIOD48_DIO_INPUT ) /* just in case */
        piod48.dio_port[nport] = 
            inb( piod48.base_io+dio_caddr[nchip]+dio_daddr[nchan] );
            
    return piod48.dio_port[nport];
    }

/* piod48_dio_get_bit  ---  get bit at port 0-5, channel 0-7,
 * into "bit".
 */ 
unsigned short int piod48_dio_get_bit( unsigned short int nport,
    unsigned short int nbit )
    {
    nport %= PIOD48_DIO_PORTS; /* just in case */
    nbit  %= PIOD48_DIO_NBITS; /* just in case */

    if ( piod48.dio_type[nport] == PIOD48_DIO_INPUT ) /* just in case */
        piod48.dio_port[nport] = piod48_dio_get_byte( nport );

    return (unsigned short int) 
        ( ( ( piod48.dio_port[nport] ) & ( 1<<nbit ) ) >> nbit );
    }

/* config ports as INPUT or OUTPUT, if output, set to zero.
 */ 
void piod48_dio_conf_port( unsigned short int nport,
    unsigned char type )
    {
    unsigned short int dio_caddr[PIOD48_DIO_CHIPS] = PIOD48_DIO_BASES;
    unsigned char dio_icode[PIOD48_DIO_PPERC] = PIOD48_DIO_ICODE;
    unsigned char ctrl;
    int nchip, nchan, i;

    nport %= PIOD48_DIO_PORTS;        /* just in case */
    nchip =  nport/PIOD48_DIO_PPERC;  /* number of chip */
    nchan =  nport%PIOD48_DIO_PPERC;  /* number of channel */
    
    /* configure port
     */
    if ( type == PIOD48_DIO_INPUT )
        {
        piod48.dio_type[nport] = PIOD48_DIO_INPUT;  /* type for input */
        piod48.dio_code[nport] = dio_icode[nchan];  /* code for input */
        }
    else if ( type == PIOD48_DIO_OUTPUT )
        {
        piod48.dio_type[nport] = PIOD48_DIO_OUTPUT; /* type for output */
        piod48.dio_code[nport] = PIOD48_DIO_OCODE;  /* code for output */
        }
    piod48.dio_port[nport] = 0x00; /* set byte to 0  */

    /* program nchip by composing port types
     */
    ctrl = 0x00;
    for ( i=0; i<PIOD48_DIO_PPERC; i++ )
        ctrl |= piod48.dio_code[i+nchip*PIOD48_DIO_PPERC];
    outb( ctrl, piod48.base_io+dio_caddr[nchip]+PIOD48_DIO_CADDR );
    
    /* set initial values
     */
    if ( type == PIOD48_DIO_OUTPUT )
        piod48_dio_set_byte( piod48.dio_port[nport], nport );

    if ( type == PIOD48_DIO_INPUT )
        piod48.dio_port[nport] = piod48_dio_get_byte( nport );
    }


/***********************************************************************
 *  piod48 defaults                                                     *
 ***********************************************************************/

static void piod48_defaults( void )
    {
    int i;
        
    /* configure digital io ports
     */
    for ( i=0; i<PIOD48_DIO_PORTS; i++ )
        piod48_dio_conf_port( i, PIOD48_DIO_INPUT );   

    piod48_int_select( PIOD48_INTX_DISABLE, PIOD48_INTX_OFF );
    }


/***********************************************************************
 * Linux kernel functions                                              *
 ***********************************************************************/

/* the mandatory init_module.
 */
int init_module( void )
    {
    struct pci_dev *dev = NULL;
    unsigned char  sub_ven, sub_dev, sub_aux;
    unsigned int   csid;  /* composed sub-ids */

    /* Check presence of PCI bus, cela se discute.
     */
    if( !pci_present() ) 
        {
        printk("%s PCI bus not present!\n", PIOD48_NAME );
        return -ENODEV;
        }
    
    /* find the first device of this kind
     */
    while(1)
        {
        dev =  pci_find_device( PIOD48_VENDOR_ID, PIOD48_DEVICE_ID, dev );
        if ( !dev )
            {
            printk("%s no PCI device with vendor ID %x and device ID %x found\n",
                PIOD48_NAME, PIOD48_VENDOR_ID, PIOD48_DEVICE_ID );
            return -ENODEV;
            }
        printk("%s PCI device with vendor ID %x and device ID %x found\n",
            PIOD48_NAME, PIOD48_VENDOR_ID, PIOD48_DEVICE_ID );

        /* get sub-vendor-id and sub-device-id, sub-aux-id,
         * a little bit annoying make composed sub-ids and 
         * identify this device
         */
        if ( pci_read_config_byte( dev, PCI_SUBSYSTEM_VENDOR_ID, &sub_ven ) ||
		     pci_read_config_byte( dev, PCI_SUBSYSTEM_ID,        &sub_dev ) )
            {
		    printk( "%s getting subsystem ID failed!\n", PIOD48_NAME );
            return -ENODEV; /* something went wrong */
            }
 
        /* get physical address, request it and make io address
         */
        piod48.base_phys =  dev->base_address[0] & PCI_BASE_ADDRESS_IO_MASK;
        if ( check_region( piod48.base_phys, PIOD48_RANGE ) < 0 )
            {
         	printk( "%s could not request base address\n", PIOD48_NAME );
            continue; /* look for another board */
            }
        request_region( piod48.base_phys, PIOD48_RANGE, PIOD48_NAME );
        piod48.base_io = (unsigned short int)(piod48.base_phys);
        printk( "%s requested base io address 0x%x\n", 
            PIOD48_NAME, piod48.base_io );

        sub_aux = ( inb( piod48.base_io + PIOD48_SUBAUX_ADDR ) & 
            PIOD48_SUBAUX_MASK );
        csid = (sub_ven<<16) + (sub_dev<<8) + (sub_aux);
        if ( csid == PIOD48_SUBAUX_ID )
            {
            printk("%s PCI device with composed sub ID %x is a %s\n", 
                PIOD48_NAME, PIOD48_SUBAUX_ID, PIOD48_NAME );
            
            piod48.irq = dev->irq;
            printk("%s IRQ is selected to be %d (0 = no interrupt)\n",
                PIOD48_NAME, dev->irq );
 
            /* Board has to be enabled before, not explicitely in manual!!
             */
            outb( PIOD48_ENABLE, piod48.base_io + PIOD48_RESET_ADDR );
            printk( "%s DAQ board enabled\n", PIOD48_NAME );

            /* Set board defaults
             */
            piod48_defaults();
            
            break;
            }
        else
            {
            printk("%s PCI device with composed sub ID %x is no %s\n",
                PIOD48_NAME, PIOD48_SUBAUX_ID, PIOD48_NAME );
            release_region( piod48.base_phys, PIOD48_RANGE );
            printk( "%s released base io address 0x%x\n", 
                PIOD48_NAME, piod48.base_io );
            }
        }
        
    return 0;
    } 
     
/* the mandatory cleanup_module.
 */
void cleanup_module( void )
    {
    /* Set board defaults
     */
    piod48_defaults();

    /* disable board
     */
    outb( PIOD48_DISABLE, piod48.base_io + PIOD48_RESET_ADDR );
    printk( "%s DAQ board disabled\n", PIOD48_NAME );
       
    release_region( piod48.base_phys, PIOD48_RANGE );
    printk( "%s released base io address 0x%x\n", 
        PIOD48_NAME, piod48.base_io );
 	
    printk( "%s DAQ board released\n", PIOD48_NAME );
    } 
