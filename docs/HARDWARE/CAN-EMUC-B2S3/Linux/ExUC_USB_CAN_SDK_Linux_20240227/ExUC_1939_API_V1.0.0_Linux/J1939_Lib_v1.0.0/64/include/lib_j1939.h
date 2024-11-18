#ifndef __LIB_J1939_H__
#define __LIB_J1939_H__

#include <stdint.h>

#define  CAN_PORT_NUM                 2
#define  J1939_PGN_REQUEST            59904
#define  J1939_PGN_ADDRESS_CLAIMED    60928
#define  J1939_PGN_COMMANDED_ADDRESS  65240


/* J1939 callback function */
typedef void (*J1939_CB_FUNC) (void *ptr);


/* callback msg_type */
enum
{
  NORMAL_PGN = 0,
  REQUEST_PGN,
  CHANGE_SA,
  CMD_SA
};


/* ack_type */
enum
{
  ACK_P = 0,
  ACK_N,
  ACK_AD,
  ACK_CR
};

/* CAN port */
enum
{
  CAN_1 = 0,
  CAN_2,
};


/*----------------------------------------------------*/
typedef struct 
{
  uint8_t  aac;          /* 1-bit Arbitrary Address Capable */
  uint8_t  ind_grp;      /* 3-bit Industry Group            */
  uint8_t  veh_sys_inst; /* 4-bit Vehicle System Instance   */
  uint8_t  veh_sys;      /* 7-bit Vehicle System            */ 
  uint8_t  func;         /* 8-bit Function                  */
  uint8_t  func_inst;    /* 5-bit Function Instance         */
  uint8_t  ecu_inst;     /* 3-bit ECU Instance              */
  uint16_t mfg_code;     /* 11-bit Manufacturer Code        */
  uint32_t identy_num;   /* 21-bit Identity Number          */

} J1939_NAME_INFO;


/*----------------------------------------------------*/
typedef struct
{
  int              com_port;
  uint8_t          sa   [CAN_PORT_NUM];  /* [0]: CAN_1, [1]: CAN_2 */
  J1939_NAME_INFO  name [CAN_PORT_NUM];

} J1939_INIT_INFO;


/*----------------------------------------------------*/
typedef struct
{
  uint32_t  pgn;      /* parameter group number */
  uint8_t  *buf;      /* pointer to data        */
  uint16_t  buf_len;  /* size of data           */
  uint8_t   dst;      /* destination of message */
  uint8_t   src;      /* source of message      */
  uint8_t   pri;      /* priority of message    */
  uint8_t   port;     /* CAN port of message    */

} J1939_FRAME_INFO;



/*----------------------------------------------------*/
typedef struct
{
  int               msg_type;     /* Out */
  int               ack_type;     /* In  */

  uint8_t           sa;           /* In (msg_type = CHANGE_SA) / Out (msg_type = CMD_SA)  */
  uint8_t           sa_req_port;  /* Out */
  uint32_t          req_pgn;      /* Out */

  J1939_FRAME_INFO  frame;        /* Out */
  J1939_CB_FUNC     cb_func;

} J1939_CB_INFO;


/*-------------------------------------------------------------------------------------------------------*/
#ifdef DEVELOPING_J1939
  uint8_t EMUCJ1939Init      (J1939_INIT_INFO   init);
  uint8_t EMUCJ1939Send      (J1939_FRAME_INFO  frame);
  void    EMUCJ1939RegCbFunc (J1939_CB_INFO    *cb_info);
  void    EMUCJ1939Stop      (int com_port);
#else
  #if defined(__linux__) || defined(__QNX__)
    extern uint8_t EMUCJ1939Init      (J1939_INIT_INFO   init);
    extern uint8_t EMUCJ1939Send      (J1939_FRAME_INFO  frame);
    extern void    EMUCJ1939RegCbFunc (J1939_CB_INFO    *cb_info);
    extern void    EMUCJ1939Stop      (int com_port);
  #else
    typedef uint8_t (*EMUC_J1939_INIT)        (J1939_INIT_INFO   init);
    typedef uint8_t (*EMUC_J1939_SEND)        (J1939_FRAME_INFO  frame);
    typedef void    (*EMUC_J1939_REG_CB_FUNC) (J1939_CB_INFO    *cb_info);
    typedef void    (*EMUC_J1939_STOP)        (int com_port);

    EMUC_J1939_INIT         EMUCJ1939Init;
    EMUC_J1939_SEND         EMUCJ1939Send;
    EMUC_J1939_REG_CB_FUNC  EMUCJ1939RegCbFunc;
    EMUC_J1939_STOP         EMUCJ1939Stop;
  #endif
#endif
/*-------------------------------------------------------------------------------------------------------*/
#endif