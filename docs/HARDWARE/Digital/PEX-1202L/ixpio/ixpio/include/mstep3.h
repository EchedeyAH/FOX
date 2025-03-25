//*********************************************************
//   MSTEP31.h                                      2/2/2001
//
//*********************************************************
//#define WIN
//#define WINUSER
//#define DOS

#ifdef WIN
  #define EXPORTS           //for WIN95
#endif

#ifdef WINUSER
  #define EXPORTS extern "C" __declspec (dllexport) //for WIN95 user
#endif

#ifdef DOS
  #define EXPORTS   extern    //DOS
  #define CALLBACK            //DOS
#endif

//---------------------------------------------------------
#define YES    1
#define NO     0
#define ON     1
#define OFF    0

#define CW_CCW    0
#define PULSE_DIR 1

#define NORMAL_DIR   0
#define REVERSE_DIR  1

#define FW      0
#define BW      1
#define CW      0
#define CCW     1

#define X_axis   1
#define Y_axis   2
#define Z_axis   3

#define XY_plane 1
#define XZ_plane 2
#define YZ_plane 3

#define DDA_CW_CCW     0x00
#define DDA_DIR_PULSE  0x01
#define SERVO_ON       0x02
#define DDA_EN         0x04
#define DDA_OE         0x08

#define ENC_MARK       0x30
#define ENC_AB_PHASE   0x00
#define ENC_CW_CCW     0x10
#define ENC_DIR_PULSE  0x20
#define ENC_EXTERNAL   0x40
#define ENC_INTERNAL   0x00
                  
#define READY 0
#define BUSY  1
                  
#define pi 3.141592654

#define EXPORTS
#define CALLBACK
//####################################################################
#ifndef DOS
EXPORTS void CALLBACK MSTEP3_INITIAL();
EXPORTS void CALLBACK MSTEP3_END();
#endif
//------------------------------------------------------------------------
// EXPORTS unsigned char CALLBACK MSTEP3_REGISTRATION(unsigned char,
//					  unsigned int);
#ifdef __cplusplus
extern "C" {
#endif

EXPORTS unsigned char CALLBACK MSTEP3_REGISTRATION1(unsigned char cardNo,
					  unsigned int address);
EXPORTS unsigned char CALLBACK MSTEP3_DI(unsigned char cardNo);
EXPORTS void          CALLBACK MSTEP3_DO(unsigned char cardNo, unsigned char value);
EXPORTS void          CALLBACK MSTEP3_DO1(unsigned char cardNo, unsigned char value);
EXPORTS unsigned char CALLBACK MSTEP3_MSC(unsigned char cardNo);
EXPORTS void          CALLBACK MSTEP3_WAIT_X(unsigned char cardNo);
EXPORTS unsigned char CALLBACK MSTEP3_IS_X_STOP(unsigned char cardNo);
EXPORTS void          CALLBACK MSTEP3_WAIT_Y(unsigned char cardNo);
EXPORTS unsigned char CALLBACK MSTEP3_IS_Y_STOP(unsigned char cardNo);
EXPORTS void          CALLBACK MSTEP3_WAIT_Z(unsigned char cardNo);
EXPORTS unsigned char CALLBACK MSTEP3_IS_Z_STOP(unsigned char cardNo);  
//------------------------------------------------------------------------
EXPORTS void CALLBACK MSTEP3_SET_CONTROL_MODE(unsigned char cardNo,
				     unsigned char x_mode,
				     unsigned char y_mode,
				     unsigned char z_mode);
EXPORTS void CALLBACK MSTEP3_SET_VAR(unsigned char cardNo,
			    unsigned char set_DDA_cycle,
			    unsigned char set_Acc_Dec,
			    unsigned int  set_Low_Speed,
			    unsigned int  set_High_Speed,
			    unsigned int  set_arc_speed);
EXPORTS void CALLBACK MSTEP3_SET_DEFDIR(unsigned char cardNo,
			       unsigned char defdirX,
			       unsigned char defdirY,
			       unsigned char defdirZ);
EXPORTS void CALLBACK MSTEP3_SET_SERVO_ON(unsigned char cardNo,
				 unsigned char sonX,
				 unsigned char sonY,
				 unsigned char sonZ);
EXPORTS void CALLBACK MSTEP3_SET_ZERO(unsigned char cardNo,
					 unsigned char axis);
EXPORTS void CALLBACK MSTEP3_PRESET_POSITION(unsigned char cardNo, unsigned char axis,
				    long preset_position);
EXPORTS void CALLBACK MSTEP3_SET_NC(unsigned char cardNo, unsigned char sw); 
//----------------------------------------------------------------------
EXPORTS void CALLBACK MSTEP3_STOP(unsigned char cardNo, unsigned char axis);
EXPORTS void CALLBACK MSTEP3_DEC_STOP(unsigned char cardNo, unsigned char axis);
EXPORTS void CALLBACK MSTEP3_RESET_SYSTEM(unsigned char cardNo);
EXPORTS void CALLBACK MSTEP3_STOP_ALL(unsigned char cardNo);
EXPORTS void CALLBACK MSTEP3_EMG_STOP(unsigned char cardNo);
//----------------------------------------------------------------------
EXPORTS void CALLBACK MSTEP3_BACK_HOME(unsigned char cardNo,
				unsigned char axis,
				unsigned char set_home_speed,
				unsigned char set_search_speed);
EXPORTS void CALLBACK MSTEP3_BACK_HOME01(unsigned char cardNo,
				unsigned char axis,
				unsigned char set_home_speed);
EXPORTS void CALLBACK MSTEP3_PULSE_MOVE(unsigned char cardNo,
			       unsigned char axis,
			       long pulseN,
			       unsigned int move_speed);
EXPORTS void CALLBACK MSTEP3_INTP_PULSE(unsigned char cardNo,
				int Xpulse,
				int Ypulse,
				int Zpulse);
EXPORTS void CALLBACK MSTEP3_CONSTANT_SPEED(unsigned char cardNo,
				   unsigned char axis,
				   unsigned char dir,
				   unsigned int move_speed);

//-----------------------------------------------------------------------                                            
EXPORTS void CALLBACK MSTEP3_INTP_XYZ(unsigned char cardNo,
			              long x,long y,long z,
			              unsigned int speed);
//-----------------------------------------------------------------------                                                                         
EXPORTS void CALLBACK MSTEP3_INTP_LINE01(unsigned char cardNo,
                                unsigned char plane,
		                long x,
		                long y,
 		                unsigned int speed);
EXPORTS void CALLBACK MSTEP3_INTP_LINE(unsigned char cardNo,
			      long x,long y,unsigned int speed);
//-----------------------------------------------------------------------
EXPORTS void CALLBACK MSTEP3_INTP_CIRCLE01(unsigned char cardNo,
                                  unsigned char plane,
			          long x,
			          long y,
				  unsigned char dir,
				  unsigned int speed);
EXPORTS void CALLBACK MSTEP3_INTP_CIRCLE(unsigned char cardNo,
                                long x,
                                long y,
				unsigned char dir,
				unsigned int speed);                     
//-----------------------------------------------------------------------
EXPORTS void CALLBACK MSTEP3_INTP_ARC01(unsigned char cardNo,
			       unsigned char plane,
			       long x,
			       long y,
			       long R,
			       unsigned char dir,
			       unsigned int speed);
EXPORTS void CALLBACK MSTEP3_INTP_ARC(unsigned char cardNo,
			     long x,
			     long y,
			     long R,
			     unsigned char dir,
			     unsigned int speed); 
//-----------------------------------------------------------------------                                                  
EXPORTS void CALLBACK MSTEP3_INTP_XYZ02(unsigned char cardNo,
                       long x,
                       long y,
                       long z,
                       unsigned int speed,
                       unsigned char acc_mode);    
EXPORTS void CALLBACK MSTEP3_INTP_LINE02(unsigned char cardNo,
                                 unsigned char plane,
                                 long x,long y,
                                 unsigned int speed,
                                 unsigned char acc_mode);                       
EXPORTS void CALLBACK MSTEP3_INTP_CIRCLE02(unsigned char cardNo,
                           unsigned char plane,
                           long x,
                           long y,
                           unsigned char dir,
                           unsigned int speed,
                           unsigned char acc_mode);
EXPORTS void CALLBACK MSTEP3_INTP_ARC02(unsigned char cardNo,
                        unsigned char plane,
                        long x,
                        long y,
                        long R,
                        unsigned char dir,
                        unsigned int speed,
                        unsigned char acc_mode); 
EXPORTS unsigned char CALLBACK MSTEP3_INTP_STOP();
//####################################################################### 
EXPORTS void CALLBACK MSTEP3_GET_CARD(unsigned char cardNo);
EXPORTS unsigned char CALLBACK MSTEP3_GET_SYS(unsigned char cardNo);
EXPORTS unsigned char CALLBACK MSTEP3_GET_LIMIT(unsigned char cardNo);
EXPORTS unsigned char CALLBACK MSTEP3_GET_P1(unsigned char cardNo);
EXPORTS long          CALLBACK MSTEP3_GET_XC(unsigned char cardNo);
EXPORTS long          CALLBACK MSTEP3_GET_XP(unsigned char cardNo);
EXPORTS long          CALLBACK MSTEP3_GET_YC(unsigned char cardNo);
EXPORTS long          CALLBACK MSTEP3_GET_YP(unsigned char cardNo);
EXPORTS long          CALLBACK MSTEP3_GET_ZC(unsigned char cardNo);
EXPORTS long          CALLBACK MSTEP3_GET_ZP(unsigned char cardNo);

#ifdef __cplusplus
}
#endif
