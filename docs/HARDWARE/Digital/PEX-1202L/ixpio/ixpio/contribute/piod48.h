/************************************************************************
 *                                                                      *
 *  piod48.h  ---  daq inputs header file for piod48.c.                   *
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

/************************************************************************
 *  control stuff                                                       * 
 ************************************************************************/

/* interrupt mask
 */
#define PIOD48_INTX_DISABLE  0x00    /* disable INT generation */
#define PIOD48_INT0_ENABLE   0x01    /* enable INT0 generation */
#define PIOD48_INT1_ENABLE   0x02    /* enable INT1 generation */
#define PIOD48_INT2_ENABLE   0x04    /* enable INT2 generation */
#define PIOD48_INT3_ENABLE   0x08    /* enable INT3 generation */

/* interrupt source
 */
#define PIOD48_INTX_OFF      0x14    /* disable all interrupts */
#define PIOD48_INT0_OFF      0x04    /* disable interrupt 0 */
#define PIOD48_INT0_3OF2     0x08    /* channel 3 of port 2 */
#define PIOD48_INT0_3N7OF2   0x00    /* channel 3 and !channel 7 of port 2 */
#define PIOD48_INT1_OFF      0x10    /* disable interrupt 1 */
#define PIOD48_INT1_3OF5     0x20    /* channel 3 of port 5 */
#define PIOD48_INT1_3N7OF5   0x00    /* channel 3 and !channel 7 of port 5 */
#define PIOD48_INT2_PC0INV   0x02    /* invert pc0 of port 2 */
#define PIOD48_INT2_PC0NINV  0x00    /* not invert pc0 of port 2 */

#define PIOD48_COUNTER_2MHZ  0x00    /* counter at 2MHz */

/* interrupt polarity
 */
#define PIOD48_INTX_INVERSE  0x00    /* inverse polarity for all  */
#define PIOD48_INT0_NONINV   0x01    /* INT0 non-inverse polarity */
#define PIOD48_INT1_NONINV   0x02    /* INT1 non-inverse polarity */
#define PIOD48_INT2_NONINV   0x04    /* INT2 non-inverse polarity */
#define PIOD48_INT3_NONINV   0x08    /* INT3 non-inverse polarity */
#define PIOD48_INTX_NONINV   0x0f    /* all non-inverse polarity  */

/* interrupt dis/enable
 */
#define PIOD48_IRQ_DISABLE   0x00    /* disable all interrupts */
#define PIOD48_IRQ_ENABLE    0x01    /* enable interrupt       */

/* counter modes
 */
#define PIOD48_COUNTER_MODE0 0x00    /* interrupt on terminal count */
#define PIOD48_COUNTER_MODE1 0x02    /* programmable oneshot        */
#define PIOD48_COUNTER_MODE2 0x04    /* rate generator              */
#define PIOD48_COUNTER_MODE3 0x06    /* square wave generator       */
#define PIOD48_COUNTER_MODE4 0x08    /* software triggered pulse    */
#define PIOD48_COUNTER_MODE5 0x0a    /* hardware triggered pulse    */


/************************************************************************
 *  io stuff                                                            * 
 ************************************************************************/

/* DIO port type
 */
#define PIOD48_DIO_INPUT     0x00    /* dio port is an output */
#define PIOD48_DIO_OUTPUT    0x01    /* dio port is an input  */


/***********************************************************************
 * Control functions ...                                               *
 ***********************************************************************/

/* program interrupt with mask and source
 */
extern void piod48_int_select( unsigned int mask, unsigned int source );

/* invert polarity
 */
extern unsigned int piod48_inv_polarity( void );

/* get number of interrupt line, irq handler can be defined outside
 */
extern int piod48_irq_get( void );

/* start counter "counter" with divisor and mode
 */
extern void piod48_counter_set( unsigned int counter, 
    unsigned int divisor, unsigned char mode );

/* start timer using two coutners, with "time" in nanosecs and mode!
 */
extern unsigned int piod48_timer32_start( unsigned int timer,
    unsigned int time, unsigned char mode );

/* stop timer using coutners
 */
extern void piod48_timer32_stop( unsigned int timer );


/***********************************************************************
 * Digital input/output basic functions ...                            *
 ***********************************************************************/

/* piod48_dio_set_byte  ---  set byte at port 0-5 by "byte".
 */ 
extern void piod48_dio_set_byte( unsigned char byte,
    unsigned short int nport );

/* set bit at port, channel 0-7, by "bit" 0 or 1.
 */ 
extern void piod48_dio_set_bit( unsigned short int bit,
    unsigned short int nport, unsigned short int nbit );
    
/* piod48_dio_get_byte  ---  get byte at port 0-5 into "byte"
 */ 
extern unsigned char piod48_dio_get_byte( unsigned short int nport );

/* piod48_dio_get_bit  ---  get bit at port 0-5, channel 0-7,
 * into "bit".
 */ 
extern unsigned short int piod48_dio_get_bit( unsigned short int nport,
    unsigned short int nbit );

/* config ports as INPUT or OUTPUT, if output, set to zero.
 */ 
extern void piod48_dio_conf_port( unsigned short int nport,
    unsigned char type );
