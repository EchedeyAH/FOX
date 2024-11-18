#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "lib_canopen.h"

#if defined(__linux__)
    #include <sys/time.h>
    #include <unistd.h>
    extern int kbhit(void);
    #ifndef _STATIC
        #include <dlfcn.h>
        #if defined(__x86_64__)
            #define LIB_EMUC_CANOPEN "./lib_canopen_64.so"
        #else
            #define LIB_EMUC_CANOPEN "./lib_canopen_32.so"
        #endif

EMUC_CANOPEN_ENABLE EMUCCANOpenEnable = NULL;
EMUC_CANOPEN_DISABLE EMUCCANOpenDisable = NULL;
EMUC_WRITE_OD EMUCWriteOD = NULL;
EMUC_CANOPEN_TX EMUCCANOpenTx = NULL;
EMUC_CANOPEN_SET_STATE EMUCCANOpenSetState = NULL;
EMUC_CANOPEN_RQST_STATE EMUCCANOpenRqstState = NULL;
EMUC_CANOPEN_CHECK_NODE_STATE EMUCCANOpenCheckNodeState = NULL;
EMUC_CANOPEN_GET_VER EMUCCANOpenGetVer = NULL;
void *lib_hd = NULL;

int load_so(void)
{
    char *error = NULL;

    lib_hd = dlopen(LIB_EMUC_CANOPEN, RTLD_LAZY);

    if(!lib_hd)
    {
        printf("%s\n", dlerror());
        return 1;
    }

    EMUCCANOpenEnable          = (EMUC_CANOPEN_ENABLE)            dlsym(lib_hd, "EMUCCANOpenEnable");
    EMUCCANOpenDisable         = (EMUC_CANOPEN_DISABLE)           dlsym(lib_hd, "EMUCCANOpenDisable");
    EMUCWriteOD                = (EMUC_WRITE_OD)                  dlsym(lib_hd, "EMUCWriteOD");
    EMUCCANOpenTx              = (EMUC_CANOPEN_TX)                dlsym(lib_hd, "EMUCCANOpenTx");
    EMUCCANOpenSetState        = (EMUC_CANOPEN_SET_STATE)         dlsym(lib_hd, "EMUCCANOpenSetState");
    EMUCCANOpenRqstState       = (EMUC_CANOPEN_RQST_STATE)        dlsym(lib_hd, "EMUCCANOpenRqstState");
    EMUCCANOpenCheckNodeState  = (EMUC_CANOPEN_CHECK_NODE_STATE)  dlsym(lib_hd, "EMUCCANOpenCheckNodeState");
    EMUCCANOpenGetVer          = (EMUC_CANOPEN_GET_VER)           dlsym(lib_hd, "EMUCCANOpenGetVer");

    if((error = dlerror()) != NULL)
    {
        printf("%s\n", error);
        return 2;
    }

    return 0;
}

    #endif
#endif

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#if defined(_M_X64) || defined(__amd64__)
    #define LIB_EMUC_CANOPEN "lib_canopen_64.dll"
#else
    #define LIB_EMUC_CANOPEN "lib_canopen_32.dll"
#endif

HMODULE hDLL = NULL;

EMUC_CANOPEN_ENABLE EMUCCANOpenEnable = NULL;
EMUC_CANOPEN_DISABLE EMUCCANOpenDisable = NULL;
EMUC_WRITE_OD EMUCWriteOD = NULL;
EMUC_CANOPEN_TX EMUCCANOpenTx = NULL;
EMUC_CANOPEN_SET_STATE EMUCCANOpenSetState = NULL;
EMUC_CANOPEN_RQST_STATE EMUCCANOpenRqstState = NULL;
EMUC_CANOPEN_CHECK_NODE_STATE EMUCCANOpenCheckNodeState = NULL;
EMUC_CANOPEN_GET_VER EMUCCANOpenGetVer = NULL;

void load_dll(void)
{
    hDLL = LoadLibrary(LIB_EMUC_CANOPEN);

    if(hDLL == NULL)
    {
        printf("Load %s failed!\n", LIB_EMUC_CANOPEN);
        exit(1);
    }
    else
    {
        EMUCCANOpenEnable         = (EMUC_CANOPEN_ENABLE)           GetProcAddress(hDLL, "EMUCCANOpenEnable");
        EMUCCANOpenDisable        = (EMUC_CANOPEN_DISABLE)          GetProcAddress(hDLL, "EMUCCANOpenDisable");
        EMUCWriteOD               = (EMUC_WRITE_OD)                 GetProcAddress(hDLL, "EMUCWriteOD");
        EMUCCANOpenTx             = (EMUC_CANOPEN_TX)               GetProcAddress(hDLL, "EMUCCANOpenTx");
        EMUCCANOpenSetState       = (EMUC_CANOPEN_SET_STATE)        GetProcAddress(hDLL, "EMUCCANOpenSetState");
        EMUCCANOpenRqstState      = (EMUC_CANOPEN_RQST_STATE)       GetProcAddress(hDLL, "EMUCCANOpenRqstState");
        EMUCCANOpenCheckNodeState = (EMUC_CANOPEN_CHECK_NODE_STATE) GetProcAddress(hDLL, "EMUCCANOpenCheckNodeState");
        EMUCCANOpenGetVer         = (EMUC_CANOPEN_GET_VER)          GetProcAddress(hDLL, "EMUCCANOpenGetVer");

        if(!EMUCCANOpenEnable || !EMUCCANOpenDisable || !EMUCWriteOD || !EMUCCANOpenTx || !EMUCCANOpenSetState ||
           !EMUCCANOpenRqstState || !EMUCCANOpenCheckNodeState || !EMUCCANOpenGetVer)
        {
            printf("Get function from %s failed!\n", LIB_EMUC_CANOPEN);
            exit(1);
        }
    }
}
#endif /* _WIN32 */

void cb_handle_sdo_abort(uint8_t p, uint8_t n, uint16_t index, uint8_t subindex, uint8_t cs, uint32_t abort_code);
void cb_handle_sdo(uint8_t p, uint8_t n, uint16_t index, uint8_t subindex, uint8_t *buf, uint8_t buf_len);
void cb_cob_handle(uint8_t port, uint8_t fcode, uint8_t src, uint8_t *buf, uint8_t buf_len);
void cb_update_handle(int port);
void cb_set_state(uint8_t p, uint8_t state);
void cb_time(uint8_t p, uint32_t milliseconds, uint16_t days);
void cb_sync(uint8_t p, uint8_t counter);
void cb_init_device(uint8_t p);
void cb_can_rx(uint8_t p, uint32_t id, uint8_t *buf, uint8_t buf_len);
void cb_can_tx(uint8_t p, uint32_t id, uint8_t *buf, uint8_t buf_len);

/*----------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
    int    rtn = -1;
    char  *setup_ini = "setup.ini";
    uint8_t master_port = 0;
    uint8_t slave_port = 1;
    uint8_t slave_node_id = 0x06;
    char version[VER_LEN];
    CANOPEN_CB_INFO cb_info;
    NSD_INFO nsd_info;
    CAN_TX_INFO can_tx_info;

#ifdef _WIN32
    load_dll();
#endif

#if defined(__linux__) && !defined(_STATIC)
    if(load_so())
    {
        printf("Load %s failed!\n", LIB_EMUC_CANOPEN);
        return 1;
    }
#endif

    EMUCCANOpenGetVer(version);
    printf("EMUC-B202 CANopen ver.%s\n\n", version);

    cb_info.canopen_handle_sdo_abort_cb = cb_handle_sdo_abort;
    cb_info.canopen_handle_sdo_cb = cb_handle_sdo;
    cb_info.canopen_cob_cb = cb_cob_handle;
    cb_info.canopen_update_cb = cb_update_handle;
    cb_info.canopen_set_state_cb = cb_set_state;
    cb_info.canopen_time_cb = cb_time;
    cb_info.canopen_sync_cb = cb_sync;
    cb_info.canopen_init_device_cb = cb_init_device;
    cb_info.canopen_can_rx_cb = cb_can_rx;
    cb_info.canopen_can_tx_cb = cb_can_tx;

    if((rtn = EMUCCANOpenEnable(setup_ini, cb_info)))
    {
        printf("EMUCCANOpenEnable() failed! (rtn=%d)\n", rtn);
        return rtn;
    }
    else
    {
        printf("EMUCCANOpenEnable() successfully!\n");
    }
#if defined(_WIN32)
    Sleep(1000);
#else
    sleep(1);
#endif
    printf("\nEMUCCANOpenSetState(master_port=%d, slave_node_id=%02X, CMD(ex:op mode))\n", master_port, slave_node_id);
    EMUCCANOpenSetState(master_port, slave_node_id, 0x01);   /* CANOPEN_NMT_CMD_START 0x01
                                                                CANOPEN_NMT_CMD_STOP 0x02
                                                                CANOPEN_NMT_CMD_ENTER_PREOP 0x80
                                                                CANOPEN_NMT_CMD_RESET_NODE 0x81
                                                                CANOPEN_NMT_CMD_RESET_COMM 0x82 */
#if defined(_WIN32)
    Sleep(1000);
#else
    sleep(1);
#endif
    
    printf("\nEMUCCANOpenRqstState(master_port=%d, slave_node_id=%02X)(Node guarding)\n", master_port, slave_node_id);
    EMUCCANOpenRqstState(master_port, slave_node_id);
#if defined(_WIN32)
    Sleep(50);
#else
    usleep(50*1000);
#endif

    printf("EMUCCANOpenCheckNodeState(master_port=%d, slave_node_id=%02X), nsd_info)(get node state)\n", master_port, slave_node_id);
    EMUCCANOpenCheckNodeState(master_port, slave_node_id, &nsd_info);
    printf("nsd_info=> nodeid:%02X\tstate:%02X\tlastseen_timestamp:%d\n", nsd_info.nodeid, nsd_info.state, nsd_info.lastseen_timestamp);
#if defined(_WIN32)
    Sleep(1000);
#else
    sleep(1);
#endif

    for(;;)
    {
        /* for writeOD */
        time_t rawtime;
        struct tm *timeinfo;
        uint16_t ix = 0x3000;
        uint8_t sx = 0x00;
        uint8_t buf[4];
        uint8_t bl = 3;

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        asctime(timeinfo);
        buf[0] = timeinfo->tm_sec;
        buf[1] = timeinfo->tm_min;
        buf[2] = timeinfo->tm_hour;
        printf("\nslave EMUCWriteOD(slave_port=%d, ix=%04X, sx=%02X, buf, bl)\n", slave_port, ix, sx);
        EMUCWriteOD(slave_port, ix, sx, buf, bl);

        /* for EMUCCANOpenTX */
        can_tx_info.id = 0x600 + slave_node_id;
        can_tx_info.buf[0] = (2<<5);
        can_tx_info.buf[1] = ix & 0xFF;
        can_tx_info.buf[2] = ix >> 8;
        can_tx_info.buf[3] = sx;
        can_tx_info.buf[4] = 0;
        can_tx_info.buf[5] = 0;
        can_tx_info.buf[6] = 0;
        can_tx_info.buf[7] = 0;
        can_tx_info.buf_len = 8;
        printf("master EMUCCANOpenTx(master_port=%d, can_tx_info)\n", master_port);
        EMUCCANOpenTx(master_port, can_tx_info);
    #if defined(_WIN32)
        Sleep(1000);
    #else
        sleep(1);
    #endif

        if(kbhit() != 0)
        {
            break;
        }
    }

    printf("EMUCCANOpenDisable()\n\n");
    EMUCCANOpenDisable();

    return 0;
}


/* callback function */
/*----------------------------------------------------------------------*/
void cb_handle_sdo_abort(uint8_t p, uint8_t n, uint16_t index, uint8_t subindex, uint8_t cs, uint32_t abort_code)
{

}

/*----------------------------------------------------------------------*/
void cb_handle_sdo(uint8_t p, uint8_t n, uint16_t index, uint8_t subindex, uint8_t *buf, uint8_t buf_len)
{

}

/*----------------------------------------------------------------------*/
void cb_cob_handle(uint8_t port, uint8_t fcode, uint8_t src, uint8_t *buf, uint8_t buf_len)
{

}

/*----------------------------------------------------------------------*/
void cb_update_handle(int port)
{

}

/*----------------------------------------------------------------------*/
void cb_set_state(uint8_t p, uint8_t state)
{

}

/*----------------------------------------------------------------------*/
void cb_time(uint8_t p, uint32_t milliseconds, uint16_t days)
{

}

/*----------------------------------------------------------------------*/
void cb_sync(uint8_t p, uint8_t counter)
{

}

/*----------------------------------------------------------------------*/
void cb_init_device(uint8_t p)
{

}

/*----------------------------------------------------------------------*/
void cb_can_rx(uint8_t p, uint32_t id, uint8_t *buf, uint8_t buf_len)
{
    if(p == 0)
    {
        printf("path:RX\tid:%03X\tdlc:%d\tdata:", id, buf_len);
        for(uint8_t i=0; i<buf_len; i++)
            printf("[%02X] ", buf[i]);
        printf("\n");
        if(id == 0x586)
        {
            int hour = buf[6];
            int min = buf[5];
            int sec = buf[4];

            printf("current time: %02d:%02d:%02d (from cb_can_rx())\n\n", hour, min, sec);
        }
    }
}

/*----------------------------------------------------------------------*/
void cb_can_tx(uint8_t p, uint32_t id, uint8_t *buf, uint8_t buf_len)
{
    if(p == 0)
    {
        printf("path:TX\tid:%03X\tdlc:%d\tdata:", id, buf_len);
        for(uint8_t i=0; i<buf_len; i++)
            printf("[%02X] ", buf[i]);
        printf("\n");
    }
}

/*----------------------------------------------------------------------*/
/* END: callback function */

