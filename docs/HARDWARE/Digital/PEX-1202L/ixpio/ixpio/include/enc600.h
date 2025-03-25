//-----------------------------------------------------------
// enc600.cpp                                    Chao Chi-Mou
//
// compiled under large mode, Turbo C++
//
// v1.0   3/23/2001
// v2.0   1/6/2002
//-----------------------------------------------------------
//#define WIN
//#define WINUSER
#define DOS

#if 0
#ifdef WINUSERconio  getch 
  #define EXPORTS extern "C" __declspec (dllexport) //for WIN95 user
#endif
#endif
#ifdef DOS
  #define EXPORTS   extern    //DOS
  #define CALLBACK            //DOS
#endif

//-----------------------------------------------------------
#define YES    1
#define NO     0
#define ON     1
#define OFF    0

#define CW           0
#define CCW          1
#define X1_axis      1
#define X2_axis      2
#define X3_axis      3
#define X4_axis      4
#define X5_axis      5
#define X6_axis      6
  
#define ENC_QUADRANT    0x00
#define ENC_CW_CCW      0x01
#define ENC_PULSE_DIR   0x02
#define ENC_HR_RESET    0x40
#define ENC_INDEX_RESET 0x80
//-----------------------------------------------------------------------
//EXPORTS unsigned char CALLBACK ENC6_REGISTRATION(unsigned char cardNo, unsigned int address);  
#ifdef __cplusplus
extern "C" {
#endif

EXPORTS unsigned char CALLBACK ENC6_REGISTRATION1(unsigned char cardNo, unsigned int address);
EXPORTS void          CALLBACK ENC6_INIT_CARD(unsigned char cardNo,
					      unsigned char x1_mode,
					      unsigned char x2_mode,
					      unsigned char x3_mode,
					      unsigned char x4_mode,
					      unsigned char x5_mode,
					      unsigned char x6_mode);
EXPORTS void          CALLBACK ENC6_CONFIG(unsigned char cardNo,
			                   unsigned char x1_mode,
			                   unsigned char x2_mode,
                                           unsigned char x3_mode,
			                   unsigned char x4_mode,
			                   unsigned char x5_mode,
			                   unsigned char x6_mode);					      
EXPORTS unsigned long CALLBACK ENC6_GET_ENCODER(unsigned char cardNo,unsigned char axis);
EXPORTS void          CALLBACK ENC6_RESET_ENCODER(unsigned char cardNo,unsigned char axis);
EXPORTS unsigned char CALLBACK ENC6_GET_INDEX(unsigned char cardNo,unsigned char axis); 
EXPORTS void          CALLBACK ENC6_DO(unsigned char cardNo,unsigned char value);

#ifdef __cplusplus
}
#endif
