#ifndef __LIB_CANOPEN_H__
#define __LIB_CANOPEN_H__

#include <stdint.h>

#ifndef MAX_CAN_FRAME_DATA_LEN
#define MAX_CAN_FRAME_DATA_LEN 8
#endif

#define VER_LEN               16
#define MAX_CANOPEN_PORTS_NUM 2

typedef void (*CANOPEN_HANDLE_SDO_ABORT_CB) (uint8_t p, uint8_t n, uint16_t index, uint8_t subindex, uint8_t cs, uint32_t abort_code);
typedef void (*CANOPEN_HANDLE_SDO_CB) (uint8_t p, uint8_t n, uint16_t index, uint8_t subindex, uint8_t *buf, uint8_t buf_len);
typedef void (*CANOPEN_COB_CB) (uint8_t port, uint8_t fcode, uint8_t src, uint8_t *buf, uint8_t buf_len);
typedef void (*CANOPEN_UPDATE_CB) (int port);
typedef void (*CANOPEN_SET_STATE_CB) (uint8_t p, uint8_t state);
typedef void (*CANOPEN_TIME_CB) (uint8_t p, uint32_t milliseconds, uint16_t days);
typedef void (*CANOPEN_SYNC_CB) (uint8_t p, uint8_t counter);
typedef void (*CANOPEN_INIT_DEVICE_CB) (uint8_t p);
typedef void (*CANOPEN_CAN_RX_CB) (uint8_t p, uint32_t id, uint8_t *buf, uint8_t buf_len);
typedef void (*CANOPEN_CAN_TX_CB) (uint8_t p, uint32_t id, uint8_t *buf, uint8_t buf_len);

/* the higher level, the more debug message (0:lowest) */
enum
{
    DBG_OFF = 0,
    DBG_LVL_0,
    DBG_LVL_1,
    DBG_LVL_2,
    DBG_LVL_3,
    DBG_LVL_CNT
};

typedef struct
{
    uint32_t id;
    uint8_t buf[MAX_CAN_FRAME_DATA_LEN];
    uint8_t buf_len;

} CAN_TX_INFO;

typedef struct
{
  uint8_t nodeid;
  uint8_t state;
  uint32_t lastseen_timestamp;

} NSD_INFO;

typedef struct
{
    CANOPEN_HANDLE_SDO_ABORT_CB canopen_handle_sdo_abort_cb;
    CANOPEN_HANDLE_SDO_CB canopen_handle_sdo_cb;
    CANOPEN_COB_CB canopen_cob_cb;
    CANOPEN_UPDATE_CB canopen_update_cb;
    CANOPEN_SET_STATE_CB canopen_set_state_cb;
    CANOPEN_TIME_CB canopen_time_cb;
    CANOPEN_SYNC_CB canopen_sync_cb;
    CANOPEN_INIT_DEVICE_CB canopen_init_device_cb;
    CANOPEN_CAN_RX_CB canopen_can_rx_cb;
    CANOPEN_CAN_TX_CB canopen_can_tx_cb;

} CANOPEN_CB_INFO;

typedef int  (*EMUC_CANOPEN_ENABLE)           (char *setup_ini, CANOPEN_CB_INFO canopen_cb_info);
typedef void (*EMUC_CANOPEN_DISABLE)          (void);
typedef int  (*EMUC_WRITE_OD)                 (uint8_t p, uint16_t ix, uint8_t sx, uint8_t *buf, uint8_t bl);
typedef int  (*EMUC_CANOPEN_TX)               (uint8_t p, CAN_TX_INFO can_tx_info);
typedef int  (*EMUC_CANOPEN_SET_STATE)        (uint8_t p, uint8_t node_id, uint8_t state);
typedef int  (*EMUC_CANOPEN_RQST_STATE)       (uint8_t p, uint8_t node_id);
typedef int  (*EMUC_CANOPEN_CHECK_NODE_STATE) (uint8_t p, uint8_t nodeid, NSD_INFO *nsd_info);
typedef void (*EMUC_CANOPEN_GET_VER)          (char str[VER_LEN]);

#if defined(__linux__) && defined(_STATIC)
extern int  EMUCCANOpenEnable(char *setup_ini, CANOPEN_CB_INFO canopen_cb_info);
extern void EMUCCANOpenDisable(void);
extern int  EMUCWriteOD(uint8_t p, uint16_t ix, uint8_t sx, uint8_t *buf, uint8_t bl);
extern int  EMUCCANOpenTx(uint8_t p, CAN_TX_INFO can_tx_info);
extern int  EMUCCANOpenSetState(uint8_t p, uint8_t node_id, uint8_t state);
extern int  EMUCCANOpenRqstState(uint8_t p, uint8_t node_id);
extern int  EMUCCANOpenCheckNodeState(uint8_t p, uint8_t nodeid, NSD_INFO *nsd_info);
extern void EMUCCANOpenGetVer(char str[VER_LEN]);
#endif

#endif