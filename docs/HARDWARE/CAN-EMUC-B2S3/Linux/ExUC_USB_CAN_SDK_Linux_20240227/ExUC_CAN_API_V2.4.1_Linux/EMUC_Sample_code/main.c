#if defined(_WIN32)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ini.h"
#include "lib_emuc_2.h"

#if defined(__linux__) || defined(__QNX__)
    #include <pthread.h>
    #include <signal.h>
    #include <unistd.h>
    #include <fcntl.h>
    #ifndef _STATIC
	#include <dlfcn.h>
        #if defined(__x86_64__)
            #define LIB_DYNAMIC_EMUC "./lib_emuc2_64.so"
        #else
            #define LIB_DYNAMIC_EMUC "./lib_emuc2_32.so"
        #endif
    #endif
#endif

#if defined(_WIN32)
    #include <process.h>
    #include <windows.h>
#endif

#define DEMO_RANGE_FILTER false

#define FILE_PATH_MAX 1024

#if defined(_WIN32)
    #if defined(_M_X64) || defined(__amd64__)
        #define LIB_EMUC "lib_emuc2_64.dll"
    #else
        #define LIB_EMUC "lib_emuc2_32.dll"
    #endif
    HMODULE hDLL = NULL;
    HANDLE  TRD_send = NULL;
    HANDLE  TRD_recv = NULL;

    EMUC_OPEN_DEVICE        EMUCOpenDevice;
    EMUC_CLOSE_DEVICE       EMUCCloseDevice;
    EMUC_SHOW_VER           EMUCShowVer;
    EMUC_RESET_CAN          EMUCResetCAN;
    EMUC_CLEAR_FILTER       EMUCClearFilter;
    EMUC_INIT_CAN           EMUCInitCAN;
    EMUC_SET_BAUDRATE       EMUCSetBaudRate;
    EMUC_SET_MODE           EMUCSetMode;
    EMUC_INIT_RANGE_FILTER  EMUCInitRangeFilter;
    EMUC_GET_RANGE_FILTER   EMUCGetRangeFilter;
    EMUC_SET_RANGE_FILTER   EMUCSetRangeFilter;
    EMUC_SET_FILTER         EMUCSetFilter;
    EMUC_SET_ERROR_TYPE     EMUCSetErrorType;
    EMUC_GET_CFG            EMUCGetCfg;
    EMUC_EXP_CFG            EMUCExpCfg;
    EMUC_IMP_CFG            EMUCImpCfg;
    EMUC_SEND               EMUCSend;
    EMUC_RECEIVE            EMUCReceive;
    EMUC_RECEIVE_NON_BLOCK  EMUCReceiveNonblock;
    EMUC_GET_BUS_ERROR      EMUCGetBusError;
    EMUC_SET_RECV_BLOCK     EMUCSetRecvBlock;
#else 
    #if defined(__linux__) || defined(__QNX__)
        void      *TRD_recv_rtn;
        void      *TRD_send_rtn;
        pthread_t  TRD_recv = 0;
        pthread_t  TRD_send = 0;
		#ifdef _STATIC
        extern char *comports[];
		#endif
        static void *TRD_recv_fx (void *ptr);
        static void *TRD_send_fx (void *ptr);
        static void  end_emuc (int type);

        #ifndef _STATIC
            EMUC_OPEN_DEVICE        EMUCOpenDevice      = NULL;
            EMUC_CLOSE_DEVICE       EMUCCloseDevice     = NULL;
            EMUC_SHOW_VER           EMUCShowVer         = NULL;
            EMUC_RESET_CAN          EMUCResetCAN        = NULL;
            EMUC_CLEAR_FILTER       EMUCClearFilter     = NULL;
            EMUC_INIT_CAN           EMUCInitCAN         = NULL;
            EMUC_SET_BAUDRATE       EMUCSetBaudRate     = NULL;
            EMUC_SET_MODE           EMUCSetMode         = NULL;
            EMUC_INIT_RANGE_FILTER  EMUCInitRangeFilter = NULL;
            EMUC_GET_RANGE_FILTER   EMUCGetRangeFilter  = NULL;
            EMUC_SET_RANGE_FILTER   EMUCSetRangeFilter  = NULL;
            EMUC_SET_FILTER         EMUCSetFilter       = NULL;
            EMUC_SET_ERROR_TYPE     EMUCSetErrorType    = NULL;
            EMUC_GET_CFG            EMUCGetCfg          = NULL;
            EMUC_EXP_CFG            EMUCExpCfg          = NULL;
            EMUC_IMP_CFG            EMUCImpCfg          = NULL;
            EMUC_SEND               EMUCSend            = NULL;
            EMUC_RECEIVE            EMUCReceive         = NULL;
            EMUC_RECEIVE_NON_BLOCK  EMUCReceiveNonblock = NULL;
            EMUC_GET_BUS_ERROR      EMUCGetBusError     = NULL;
            EMUC_SET_RECV_BLOCK     EMUCSetRecvBlock    = NULL;

            void *lib_handle = NULL;

            int load_so(void)
            {
                char *error = NULL;

                lib_handle = dlopen(LIB_DYNAMIC_EMUC, RTLD_LAZY);

                if(!lib_handle)
                {
                    printf("%s\n", dlerror());
                    return 1;
                }
				else
					printf("load dll success %s\n",LIB_DYNAMIC_EMUC);

                EMUCOpenDevice      = (EMUC_OPEN_DEVICE)       dlsym(lib_handle, "EMUCOpenDevice");
                EMUCCloseDevice     = (EMUC_CLOSE_DEVICE)      dlsym(lib_handle, "EMUCCloseDevice");
                EMUCShowVer         = (EMUC_SHOW_VER)          dlsym(lib_handle, "EMUCShowVer");
                EMUCResetCAN        = (EMUC_RESET_CAN)         dlsym(lib_handle, "EMUCResetCAN");
                EMUCClearFilter     = (EMUC_CLEAR_FILTER)      dlsym(lib_handle, "EMUCClearFilter");
                EMUCInitCAN         = (EMUC_INIT_CAN)          dlsym(lib_handle, "EMUCInitCAN");
                EMUCSetBaudRate     = (EMUC_SET_BAUDRATE)      dlsym(lib_handle, "EMUCSetBaudRate");
                EMUCSetMode         = (EMUC_SET_MODE)          dlsym(lib_handle, "EMUCSetMode");
                EMUCInitRangeFilter = (EMUC_INIT_RANGE_FILTER) dlsym(lib_handle, "EMUCInitRangeFilter");
                EMUCGetRangeFilter  = (EMUC_GET_RANGE_FILTER)  dlsym(lib_handle, "EMUCGetRangeFilter");
                EMUCSetRangeFilter  = (EMUC_SET_RANGE_FILTER)  dlsym(lib_handle, "EMUCSetRangeFilter");
                EMUCSetFilter       = (EMUC_SET_FILTER)        dlsym(lib_handle, "EMUCSetFilter");
                EMUCSetErrorType    = (EMUC_SET_ERROR_TYPE)    dlsym(lib_handle, "EMUCSetErrorType");
                EMUCGetCfg          = (EMUC_GET_CFG)           dlsym(lib_handle, "EMUCGetCfg");
                EMUCExpCfg          = (EMUC_EXP_CFG)           dlsym(lib_handle, "EMUCExpCfg");
                EMUCImpCfg          = (EMUC_IMP_CFG)           dlsym(lib_handle, "EMUCImpCfg");
                EMUCSend            = (EMUC_SEND)              dlsym(lib_handle, "EMUCSend");
                EMUCReceive         = (EMUC_RECEIVE)           dlsym(lib_handle, "EMUCReceive");
                EMUCReceiveNonblock = (EMUC_RECEIVE_NON_BLOCK) dlsym(lib_handle, "EMUCReceiveNonblock");
                EMUCGetBusError     = (EMUC_GET_BUS_ERROR)     dlsym(lib_handle, "EMUCGetBusError");
                EMUCSetRecvBlock    = (EMUC_SET_RECV_BLOCK)    dlsym(lib_handle, "EMUCSetRecvBlock");

                if((error = dlerror()) != NULL)
               {
                   printf("%s\n", error);
                   return 2;
               }

                return 0;
            }
        #endif //_STATIC
    #endif // define linux
#endif

#if defined(_WIN32)
    static DWORD WINAPI TRD_recv_fx (LPVOID ptr);
    static DWORD WINAPI TRD_send_fx (LPVOID ptr);
    static BOOL end_emuc (DWORD type);
    static void load_dll (void);
#endif


/*----------------------------------------------------------*/
static void get_ini_setting (void);
static void demo_emuc (void);
static void start_emuc (void);
static void send_group (void);
static void print_data (CAN_FRAME_INFO frame_info);
static void print_config (void);
static int  handler_ini (void* user, const char* section, const char* name, const char* value);


/* setup.ini */
/*----------------------------------------------------------*/
typedef struct
{
    /* [setting] */
    int com_port;
    int send_cnt_once;
    int sleep_interval;
    int send_channel;
    bool show_send_rtn;
    bool get_bus_err;
    bool re_send;
    bool recv_block;

    /* [data] */
    int data_length;
    unsigned char test_data[DATA_LEN];

    /* [id] */
    bool id_increase;
    unsigned int id;

} INI_ITEM;

INI_ITEM  ini_item;


/*---------------------------------------------------------------------------------*/
int main (void)
{
    get_ini_setting();
    demo_emuc();
    start_emuc();
    return 0;

} /* END: main() */


/*---------------------------------------------------------------------------------*/
static void get_ini_setting (void)
{
    char dest[FILE_PATH_MAX];
    char *setup_ini = "setup.ini";

    memset(dest, 0, sizeof(dest));

    #if defined(__linux__)
        char path[FILE_PATH_MAX];
        int  len, i;

        memset(path, 0, sizeof(path));

        pid_t pid = getpid();

        sprintf(path, "/proc/%d/exe", pid);
        if(readlink(path, dest, FILE_PATH_MAX) >= 0)
        {
            len = strlen(dest);

            for(i=len; i>0; i--)
            {
                if(dest[i] == '/')
                    break;
                else
                    dest[i] = 0;
            }

            strcat(dest, setup_ini);
        }
    #elif defined(__QNX__)
        char path[FILE_PATH_MAX];
        int  fd, len, i;

        memset(path, 0, sizeof(path));

        pid_t pid = getpid();

        sprintf(path, "/proc/%d/exefile", pid);

        fd = open(path, O_RDONLY);
        if(fd < 0)
        {
            perror("get execute path");
            exit(1);
        }

        len = read(fd, dest, sizeof(dest)-1);
        if(len < 0)
            exit(1);
        
        close(fd);
    
        for(i=len; i>0; i--)
        {
            if(dest[i] == '/')
                break;
            else
                dest[i] = 0;
        }

        strcat(dest, setup_ini);

    #else
        strcat(dest, setup_ini);
    #endif

    if(ini_parse(dest, handler_ini, &ini_item) < 0)
    {
        printf("Open setup.ini failed\n");
        exit(1);
    }

} /* END: get_ini_setting() */


/*---------------------------------------------------------------------------------*/
static void demo_emuc (void)
{
    int             rtn;
    #if defined(__QNX__)
    const char     *file_name = "/tmp/emuc_config";
    #else
    const char     *file_name = "emuc_config";
    #endif
    VER_INFO        ver_info;
    FILTER_INFO     filter_info;
    CFG_INFO        cfg_info;
    NON_BLOCK_INFO  non_block_info;
    CAN_FRAME_INFO  frame_recv;

    #if defined(_WIN32)
        load_dll();
    #endif
    
    #if defined(__linux__) || defined(__QNX__)
        #ifndef _STATIC
            load_so();
        #endif
    #endif

    /* ----- EMUCOpenDevice() ----- */
    rtn = EMUCOpenDevice(ini_item.com_port);
    if(rtn)
    {
     	#if defined(__linux__) || defined(__QNX__)
			#if defined(_STATIC)
        		printf("Open %s failed !\n", comports[ini_item.com_port]);
			#else
        		printf("Open COM%d failed !\n", ini_item.com_port + 1);
			#endif	
        #else
        	printf("Open COM%d failed !\n", ini_item.com_port + 1);
        #endif
        exit(1);
    }
    else
    {
        #if defined(__linux__) || defined(__QNX__)
			#if defined(_STATIC)
        		printf("Open %s successfully !\n", comports[ini_item.com_port]);
			#else
        		printf("Open COM%d successfully !\n", ini_item.com_port + 1);
			#endif	
        #else
        	printf("Open COM%d successfully !\n", ini_item.com_port + 1);
        #endif
        printf("==============================================\n");
    }

    /* ----- EMUCInitCAN() ----- */
    rtn = EMUCInitCAN(ini_item.com_port, EMUC_INACTIVE, EMUC_INACTIVE);
    if(rtn)
    {
        printf("EMUC initial CAN failed !\n");
    }
    else
    {
        printf("EMUC initial CAN successfully !\n");
        printf("==============================================\n");
    }

    /* ----- EMUCShowVer() ----- */
    rtn = EMUCShowVer(ini_item.com_port, &ver_info);
    if(rtn)
    {
        printf("EMUC show version failed !\n");
    }
    else
    {
        printf("EMUC show version successfully !\n");
        printf("FW ver: %s\n", ver_info.fw);
        printf("LIB ver: %s\n", ver_info.api);
        printf("Model: %s\n", ver_info.model);
        printf("==============================================\n");
    }


    /* ----- EMUCResetCAN() ----- */
    rtn = EMUCResetCAN(ini_item.com_port);
    if(rtn)
    {
        printf("EMUC reset CAN failed !\n");
    }
    else
    {
        printf("EMUC reset CAN successfully !\n");
        printf("==============================================\n");
    }


    /* ----- EMUCClearFilter() ----- */
    rtn = EMUCClearFilter(ini_item.com_port, EMUC_CAN_1);
    rtn = rtn + EMUCClearFilter(ini_item.com_port, EMUC_CAN_2);
    if (rtn)
    {
        printf("EMUC clear filter failed !\n");
    }
    else
    {
        printf("EMUC clear filter successfully !\n");
        printf("==============================================\n");
    }



    /* ----- EMUCSetBaudRate() ----- */
    rtn = EMUCSetBaudRate(ini_item.com_port, EMUC_BAUDRATE_1M, EMUC_BAUDRATE_1M);
    // rtn = EMUCSetBaudRate(ini_item.com_port, EMUC_BAUDRATE_250K, EMUC_BAUDRATE_250K);
    // rtn = EMUCSetBaudRate(ini_item.com_port, EMUC_BAUDRATE_400K, EMUC_BAUDRATE_400K);
    // rtn = EMUCSetBaudRate(ini_item.com_port, EMUC_BAUDRATE_500K, EMUC_BAUDRATE_500K);
    // rtn = EMUCSetBaudRate(ini_item.com_port, EMUC_BAUDRATE_50K, EMUC_BAUDRATE_50K);
    if (rtn)
    {
        printf("EMUC set baud rate failed !\n");
    }
    else
    {
        printf("EMUC set baud rate successfully !\n");
        printf("==============================================\n");
    }


    /* ----- EMUCSetErrorType() ----- */
    rtn = EMUCSetErrorType(ini_item.com_port, EMUC_DIS_ALL);
    if (rtn)
    {
        printf("EMUC set error type failed !\n");
    }
    else
    {
        printf("EMUC set error type successfully !\n");
        printf("==============================================\n");
    }


    /* ----- EMUCSetMode() ----- */
    rtn = EMUCSetMode(ini_item.com_port, EMUC_NORMAL, EMUC_NORMAL);
    if (rtn)
    {
        printf("EMUC set mode failed !\n");
    }
    else
    {
        printf("EMUC set mode successfully !\n");
        printf("==============================================\n");
    }


    /* ----- EMUCSetFilter() ----- */
    filter_info.CAN_port = EMUC_CAN_1;
    filter_info.flt_type = EMUC_EID;
    filter_info.flt_id   = 0x0012ABCD;
    filter_info.mask     = 0x1FFFFFFF;


    rtn = EMUCSetFilter(ini_item.com_port, &filter_info);
    if(rtn)
    {
        printf("EMUC set CAN1 filter failed !\n");
    }
    else
    {
        printf("EMUC set CAN1 filter successfully !\n");
        printf("==============================================\n");
    }


    /* ----- EMUCGetCfg() ----- */
    rtn = EMUCGetCfg(ini_item.com_port, &cfg_info);
    if(rtn)
    {
        printf("EMUC get config. failed !\n");
    }
    else
    {
        printf("EMUC get config. successfully !\n");
        for(int i=0; i<CAN_NUM; i++)
        {
            printf("----------------------------------------------\n");
            printf("CAN %d:\n", i+1);
            printf("baud rate = %d\n", cfg_info.baud[i]);
            printf("mode = %d\n", cfg_info.mode[i]);
            printf("filter type = %d\n", cfg_info.flt_type[i]);
            printf("filter id = %08X\n", cfg_info.flt_id[i]);
            printf("filter mask = %08X\n", cfg_info.flt_mask[i]);
        }
        printf("----------------------------------------------\n");
        printf("error set = %d\n", cfg_info.err_set);
        printf("==============================================\n");
    }


    /* ----- EMUCExpCfg() ----- */
    rtn = EMUCExpCfg(ini_item.com_port, file_name);
    if(rtn)
    {
        printf("EMUC export config. failed !\n");
    }
    else
    {
        printf("EMUC export config. successfully !\n");
        printf("==============================================\n");
    }


    /* ----- EMUCImpCfg() ----- */
    rtn = EMUCImpCfg(ini_item.com_port, file_name);
    if(rtn)
    {
        printf("EMUC import config. failed !\n");
    }
    else
    {
        printf("EMUC import config. successfully !\n");
        printf("==============================================\n");
    }


    /* Send & Receive must initial CAN to EMUC_ACTIVE */
    /*-----------------------------------------------------------------------------------------*/
    EMUCClearFilter(ini_item.com_port, EMUC_CAN_1);  /* not necessary */
    EMUCClearFilter(ini_item.com_port, EMUC_CAN_2);  /* not necessary */
    EMUCInitCAN    (ini_item.com_port, EMUC_ACTIVE, EMUC_ACTIVE);

    /* ----- EMUCReceiveNonblock() ----- */
    non_block_info.cnt = 20;
    non_block_info.interval = 2 * 1000;
    non_block_info.can_frame_info = (CAN_FRAME_INFO *) malloc(non_block_info.cnt * sizeof(CAN_FRAME_INFO));

    printf("Non-block receive ------> Time start !\n");
    rtn = EMUCReceiveNonblock(ini_item.com_port, &non_block_info);
    if(rtn)
    {
        printf("Non-block receive ------> Time out or Data enough !\n");
        for(int i=0; i<rtn; i++)
        {
            switch(non_block_info.can_frame_info[i].msg_type)
            {
                case EMUC_DATA_TYPE:
                                        print_data(non_block_info.can_frame_info[i]);
                                        break;
                case EMUC_EEERR_TYPE:
                                        printf("EEPROM Error !\n");
                                        break;
                case EMUC_BUSERR_TYPE:
                                        printf("Bus Error !\n");
                                        printf("error data (CAN 1): ");
                                        for(int j=0; j<DATA_LEN_ERR; j++)
                                        {
                                            printf("%02X ", frame_recv.data_err[EMUC_CAN_1][j]);
                                        }
                                        printf("\n");
                                        printf("error data (CAN 2): ");
                                        for(int j=0; j<DATA_LEN_ERR; j++)
                                        {
                                            printf("%02X ", frame_recv.data_err[EMUC_CAN_2][j]);
                                        }
                                        printf("\n");
                                        break;
            }
        }
        printf("==============================================\n");
    }
    else
    {
        printf("Non-block receive ------> Time out (No data) !\n");
        printf("==============================================\n");
    }

    free(non_block_info.can_frame_info);


    /* To enable receive block: linux is ready (windows not yet) */
    #if defined(__linux__) || defined(__QNX__)
    if(ini_item.recv_block == true)
    {
        EMUCSetRecvBlock(ini_item.com_port, true);
    }
    #endif

#if DEMO_RANGE_FILTER
    RANGE_FILTER_INFO CAN1_range_filter =
    {
        .id_range[0].active = true,
        .id_range[0].type   = EMUC_SID,
        .id_range[0].head   = 0x100,
        .id_range[0].tail   = 0x10F,

        .id_range[1].active = true,
        .id_range[1].type   = EMUC_SID,
        .id_range[1].head   = 0x1F0,
        .id_range[1].tail   = 0x1FF
    };

    RANGE_FILTER_INFO CAN2_range_filter =
    {
        .id_range[0].active = true,
        .id_range[0].type   = EMUC_EID,
        .id_range[0].head   = 0x10000000,
        .id_range[0].tail   = 0x1000000F,

        .id_range[1].active = true,
        .id_range[1].type   = EMUC_EID,
        .id_range[1].head   = 0x100000F0,
        .id_range[1].tail   = 0x100000FF
    };

    rtn = EMUCSetRangeFilter(ini_item.com_port, &CAN1_range_filter, &CAN2_range_filter);
    if(rtn)
        printf("EMUC Set Range Filter failed !\n");
    else
    {
        printf("EMUC Set Range Filter successfully !\n");
        printf("==============================================\n");
    }

    rtn = EMUCInitRangeFilter(ini_item.com_port, EMUC_RANGE_FILTER_EN, EMUC_RANGE_FILTER_EN);
    if(rtn)
    {
        printf("EMUC Init Range Filter failed !\n");
    }
    else
    {
        printf("EMUC Init Range Filter successfully !\n");
        printf("==============================================\n");
    }

    memset(&CAN1_range_filter, 0, sizeof(RANGE_FILTER_INFO));
    memset(&CAN2_range_filter, 0, sizeof(RANGE_FILTER_INFO));

    rtn = EMUCGetRangeFilter(ini_item.com_port, &CAN1_range_filter, &CAN2_range_filter);
    if(rtn)
    {
        printf("EMUC Get Range Filter failed !\n");
    }
    else
    {
        printf("EMUC Get Range Filter successfully !\n");
        printf("==============================================\n");
        printf("----------------------------------------------\n");
        printf("CAN1 Range Filter Info\n");
        printf("Status: %s\n", CAN1_range_filter.sts ? "Enable" : "Disable");
        printf("Index Active  ID_type  Range(Hex)\n");
        for(int i=0; i<ID_RANGE_NUM; i++)
        {
            printf("%4d. %6c  %7s  0x%X ~ 0x%X\n", (i+1),
                                                   (CAN1_range_filter.id_range[i].active == true    ) ? 'O'   : 'X',
                                                   (CAN1_range_filter.id_range[i].type   == EMUC_SID) ? "SID" : "EID",
                                                    CAN1_range_filter.id_range[i].head,
                                                    CAN1_range_filter.id_range[i].tail);
        }

        printf("----------------------------------------------\n");
        printf("CAN2 Range Filter Info\n");
        printf("Status: %s\n", CAN2_range_filter.sts ? "Enable" : "Disable");
        printf("Index Active  ID_type  Range(Hex)\n");
        for(int i=0; i<ID_RANGE_NUM; i++)
        {
            printf("%4d. %6c  %7s  0x%X ~ 0x%X\n", (i+1),
                                                   (CAN2_range_filter.id_range[i].active == true    ) ? 'O'   : 'X',
                                                   (CAN2_range_filter.id_range[i].type   == EMUC_SID) ? "SID" : "EID",
                                                    CAN2_range_filter.id_range[i].head,
                                                    CAN2_range_filter.id_range[i].tail);
        }
        printf("==============================================\n");
    }
#endif

    /* To start CAN: must initial CAN to EMUC_ACTIVE */
    EMUCInitCAN(ini_item.com_port, EMUC_ACTIVE, EMUC_ACTIVE);

} /* END: demo_emuc() */



/*---------------------------------------------------------------------------------*/
static void start_emuc (void)
{
    print_config();

    /* thread for send & recv */
    #if defined(__linux__) || defined(__QNX__)
        pthread_create(&TRD_recv, NULL, TRD_recv_fx, NULL);
        pthread_create(&TRD_send, NULL, TRD_send_fx, NULL);
        signal(SIGINT, end_emuc);
        pthread_join(TRD_recv, &TRD_recv_rtn);
        pthread_join(TRD_send, &TRD_send_rtn);
    #else
        TRD_send = CreateThread(NULL, 0, TRD_send_fx, NULL, 0, NULL);
        TRD_recv = CreateThread(NULL, 0, TRD_recv_fx, NULL, 0, NULL);
        SetConsoleCtrlHandler((PHANDLER_ROUTINE) end_emuc, TRUE);
        WaitForSingleObject(TRD_recv, INFINITE);
        WaitForSingleObject(TRD_send, INFINITE);
    #endif


} /* END: start_emuc() */



/*---------------------------------------------------------------------------------*/
static void send_group (void)
{
    static int      cnt_record = 0;
    int             i;
    int             rtn;
    CAN_FRAME_INFO  frame_send;

    memset(&frame_send, 0, sizeof(CAN_FRAME_INFO));
    frame_send.CAN_port = ini_item.send_channel;
    frame_send.id_type = EMUC_EID;
    frame_send.rtr = EMUC_DIS_RTR;
    frame_send.dlc = ini_item.data_length;
    frame_send.id = ini_item.id;
    memcpy(frame_send.data, ini_item.test_data, DATA_LEN);

    for(i=1; i<=ini_item.send_cnt_once; i++)
    {
        if(ini_item.id_increase == true)
        {
            frame_send.id = (unsigned int) (ini_item.id + i - 1) ;
        }

        do
        {
            rtn = EMUCSend(ini_item.com_port, &frame_send);

            if (true == ini_item.show_send_rtn)
            {
                printf("%4d. send rtn=%d\n", i, rtn);
            }

            if (rtn == 0)
            {
                break;
            }

        } while (ini_item.re_send);

        if(ini_item.sleep_interval)
        {
            #if defined(__linux__) || defined(__QNX__)
                usleep(ini_item.sleep_interval * 1000);
            #else
                Sleep(ini_item.sleep_interval);
            #endif
        }
    }

    cnt_record += ini_item.send_cnt_once;
    printf("\nSend END -----------------------> cnt: %d, cumulation: %d\n\n", ini_item.send_cnt_once, cnt_record);


} /* END: send_group() */


/*---------------------------------------------------------------------------------*/
static void print_data (CAN_FRAME_INFO frame_info)
{
    static unsigned long cnt_1 = 0;
    static unsigned long cnt_2 = 0;

    int i;

    /* CAN port & cnt */
    if (frame_info.CAN_port == EMUC_CAN_1)
    {
        cnt_1++;
        printf("(CAN 1) %lu. ", cnt_1);
    }
    else if (frame_info.CAN_port == EMUC_CAN_2)
    {
        cnt_2++;
        printf("(CAN 2) %lu. ", cnt_2);
    }

    /* ID */
    if (frame_info.id_type == EMUC_SID)
    {
        printf("ID: %03X ; ", frame_info.id);
    }
    else if (frame_info.id_type == EMUC_EID)
    {
        printf("ID: %08X ; ", frame_info.id);
    }

    /* Data */
    printf("Data: ");
    for (i = 0; i<frame_info.dlc; i++)
    {
        printf("%02X ", frame_info.data[i]);
    }
    printf("; ");

    /* Time */
    printf("Time: %s\n", frame_info.recv_time);

} /* END: print_data() */


/*---------------------------------------------------------------------------------*/
static void print_config (void)
{
    printf("\nConfig\n");
    printf("----------------------------------\n");
    printf("com_port: %d\n", ini_item.com_port);
    printf("send_cnt_once: %d\n", ini_item.send_cnt_once);
    printf("send_channel: %s\n", ini_item.send_channel ? "CAN2" : "CAN1");
    printf("show_send_rtn: %s\n", ini_item.show_send_rtn ? "on" : "off");
    printf("get_bus_err: %s\n", ini_item.get_bus_err ? "on" : "off");
    printf("re_send: %s\n", ini_item.re_send ? "on" : "off");
    printf("recv_block: %s\n", ini_item.recv_block ? "on" : "off");
    printf("data_length: %d\n", ini_item.data_length);
    printf("test_data: ");
    for (int i = 0; i<DATA_LEN; i++)
    {
        printf("%02X ", ini_item.test_data[i]);
    }
    printf("\n");
    printf("id_increase: %s\n", ini_item.id_increase ? "on" : "off");
    printf("id: %u\n", ini_item.id);
    printf("\n");
    printf("----------------------------------\n\n");


} /* END: print_config() */


/*---------------------------------------------------------------------------------*/
static int handler_ini (void *user, const char *section, const char *name, const char *value)
{
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    INI_ITEM *p_ini = (INI_ITEM *)user;

    /* [setting] section */
    if(MATCH("setting", "com_port"))
    {
        p_ini->com_port = (int) atoi(value);
    }
    else if(MATCH("setting", "send_cnt_once"))
    {
        p_ini->send_cnt_once = (int) atoi(value);
    }
    else if(MATCH("setting", "sleep_interval"))
    {
        p_ini->sleep_interval = (int) atoi(value);
    }
    else if(MATCH("setting", "send_channel"))
    {
        p_ini->send_channel = (int) atoi(value);
    }
    else if(MATCH("setting", "show_send_rtn"))
    {
        p_ini->show_send_rtn = (bool) atoi(value);
    }
    else if(MATCH("setting", "get_bus_err"))
    {
        p_ini->get_bus_err = (bool) atoi(value);
    }
    else if(MATCH("setting", "re_send"))
    {
        p_ini->re_send = (bool) atoi(value);
    }
    else if(MATCH("setting", "recv_block"))
    {
        p_ini->recv_block = (bool) atoi(value);
    }

    /* [data] section */
    else if(MATCH("data", "data_length"))
    {
        p_ini->data_length = (int) atoi(value);
    }
    else if(MATCH("data", "test_data_0"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[0]);
    }
    else if(MATCH("data", "test_data_1"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[1]);
    }
    else if(MATCH("data", "test_data_2"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[2]);
    }
    else if(MATCH("data", "test_data_3"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[3]);
    }
    else if(MATCH("data", "test_data_4"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[4]);
    }
    else if(MATCH("data", "test_data_5"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[5]);
    }
    else if(MATCH("data", "test_data_6"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[6]);
    }
    else if(MATCH("data", "test_data_7"))
    {
        sscanf(value, "0x%hhx", &(p_ini->test_data)[7]);
    }

    /* [id] section */
    else if(MATCH("id", "id_increase"))
    {
        p_ini->id_increase = (bool) atoi(value);
    }
    else if(MATCH("id", "id"))
    {
        p_ini->id = (unsigned int) atoi(value);
    }

    /* unknown section/name, error */
    else
    {
        return 0;
    }

    return 1;

} /* END: handler_ini() */

#if defined(_WIN32)
/*---------------------------------------------------------------------------------*/
static void load_dll (void)
{
    hDLL = LoadLibrary(LIB_EMUC);

    if(hDLL == NULL)
    {
        printf("Load %s failed!\n", LIB_EMUC);
        exit(1);
    }
    else
    {
        EMUCOpenDevice      = (EMUC_OPEN_DEVICE)       GetProcAddress(hDLL, "EMUCOpenDevice");
        EMUCCloseDevice     = (EMUC_CLOSE_DEVICE)      GetProcAddress(hDLL, "EMUCCloseDevice");
        EMUCShowVer         = (EMUC_SHOW_VER)          GetProcAddress(hDLL, "EMUCShowVer");
        EMUCResetCAN        = (EMUC_RESET_CAN)         GetProcAddress(hDLL, "EMUCResetCAN");
        EMUCClearFilter     = (EMUC_CLEAR_FILTER)      GetProcAddress(hDLL, "EMUCClearFilter");
        EMUCInitCAN         = (EMUC_INIT_CAN)          GetProcAddress(hDLL, "EMUCInitCAN");
        EMUCSetBaudRate     = (EMUC_SET_BAUDRATE)      GetProcAddress(hDLL, "EMUCSetBaudRate");
        EMUCSetMode         = (EMUC_SET_MODE)          GetProcAddress(hDLL, "EMUCSetMode");
        EMUCInitRangeFilter = (EMUC_INIT_RANGE_FILTER) GetProcAddress(hDLL, "EMUCInitRangeFilter");
        EMUCGetRangeFilter  = (EMUC_GET_RANGE_FILTER)  GetProcAddress(hDLL, "EMUCGetRangeFilter");
        EMUCSetRangeFilter  = (EMUC_SET_RANGE_FILTER)  GetProcAddress(hDLL, "EMUCSetRangeFilter");
        EMUCSetFilter       = (EMUC_SET_FILTER)        GetProcAddress(hDLL, "EMUCSetFilter");
        EMUCSetErrorType    = (EMUC_SET_ERROR_TYPE)    GetProcAddress(hDLL, "EMUCSetErrorType");
        EMUCGetCfg          = (EMUC_GET_CFG)           GetProcAddress(hDLL, "EMUCGetCfg");
        EMUCExpCfg          = (EMUC_EXP_CFG)           GetProcAddress(hDLL, "EMUCExpCfg");
        EMUCImpCfg          = (EMUC_IMP_CFG)           GetProcAddress(hDLL, "EMUCImpCfg");
        EMUCSend            = (EMUC_SEND)              GetProcAddress(hDLL, "EMUCSend");
        EMUCReceive         = (EMUC_RECEIVE)           GetProcAddress(hDLL, "EMUCReceive");
        EMUCReceiveNonblock = (EMUC_RECEIVE_NON_BLOCK) GetProcAddress(hDLL, "EMUCReceiveNonblock");
        EMUCGetBusError     = (EMUC_GET_BUS_ERROR)     GetProcAddress(hDLL, "EMUCGetBusError");
        EMUCSetRecvBlock    = (EMUC_SET_RECV_BLOCK)    GetProcAddress(hDLL, "EMUCSetRecvBlock");

        if(!EMUCOpenDevice      || !EMUCCloseDevice || !EMUCShowVer      || !EMUCResetCAN        || !EMUCClearFilter    ||
           !EMUCInitCAN         || !EMUCSetBaudRate || !EMUCSetMode      || !EMUCInitRangeFilter || !EMUCGetRangeFilter ||
           !EMUCSetRangeFilter  || !EMUCSetFilter   || !EMUCSetErrorType || !EMUCGetCfg          || !EMUCExpCfg         ||
           !EMUCImpCfg          || !EMUCSend        || !EMUCReceive      || !EMUCReceiveNonblock || !EMUCGetBusError    ||
           !EMUCSetRecvBlock)
        {
            printf("Get function from %s failed!\n", LIB_EMUC);
            exit(1);
        }
    }

} /* END: load_dll() */
#endif


/*---------------------------------------------------------------------------------*/
#if defined(__linux__) || defined(__QNX__)
static void *TRD_recv_fx (void *ptr)
#else
static DWORD WINAPI TRD_recv_fx (LPVOID ptr)
#endif
{
    #if defined(__linux__) || defined(__QNX__)
    /* Let thread can be canceled at anytime */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    #endif

    int             i;
    int             rtn;
    CAN_FRAME_INFO  frame_recv;

    while(1)
    {
        memset(&frame_recv, 0, sizeof(CAN_FRAME_INFO));
        rtn = EMUCReceive(ini_item.com_port, &frame_recv);

        if (rtn == 1)
        {
            switch (frame_recv.msg_type)
            {
            case EMUC_DATA_TYPE:
            {
                print_data(frame_recv);
                break;
            }
            case EMUC_EEERR_TYPE:
            {
                printf("EEPROM Error !\n");
                break;
            }

            case EMUC_BUSERR_TYPE:
            {
                printf("Bus Error !\n");
                printf("error data (CAN 1): ");
                for (i = 0; i<DATA_LEN_ERR; i++)
                    printf("%02X ", frame_recv.data_err[EMUC_CAN_1][i]);
                printf("\n");

                printf("error data (CAN 2): ");
                for (i = 0; i<DATA_LEN_ERR; i++)
                    printf("%02X ", frame_recv.data_err[EMUC_CAN_2][i]);
                printf("\n");
                break;
            }
            case EMUC_GETBUS_TYPE:
            {
                printf("Bus status (CAN 1): ");
                for (i = 0; i<DATA_LEN_ERR; i++)
                    printf("%02X ", frame_recv.data_err[EMUC_CAN_1][i]);
                printf("\n");

                printf("Bus status (CAN 2): ");
                for (i = 0; i<DATA_LEN_ERR; i++)
                    printf("%02X ", frame_recv.data_err[EMUC_CAN_2][i]);
                printf("\n");
                break;
            }
            default:
                break;
            }
        }
    }

    #if defined(__linux__) || defined(__QNX__)
    return NULL;
    #else
    return 0;
    #endif

} /* END: TRD_recv_fx() */


/*---------------------------------------------------------------------------------*/
#if defined(__linux__) || defined(__QNX__)
static void *TRD_send_fx (void *ptr)
#else
static DWORD WINAPI TRD_send_fx (LPVOID ptr)
#endif
{
    #if defined(__linux__) || defined(__QNX__)
    /* Let thread can be canceled at anytime */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    #endif

    while (1)
    {
        printf("\n ------------------------- Press 'Enter' to send data, 'Ctrl + C' to exit -------------------------\n");
        getchar();

        send_group();

        if (true == ini_item.get_bus_err)
        {
            EMUCGetBusError(ini_item.com_port);
        }
    }

    #if defined(__linux__) || defined(__QNX__)
    return NULL;
    #else
    return 0;
    #endif

} /* END: TRD_send_fx() */


/*---------------------------------------------------------------------------------*/
#if defined(__linux__) || defined(__QNX__)
static void end_emuc (int type)
#else
static BOOL end_emuc (DWORD type)
#endif
{
    #if defined(__linux__) || defined(__QNX__)
    if(type == SIGINT)
    #else
    if(type == CTRL_C_EVENT)
    #endif
    {
        printf("\nExit the program ...\n");

        #if defined(__linux__) || defined(__QNX__)
        pthread_cancel(TRD_send);
        pthread_cancel(TRD_recv);
        #else
        TerminateThread(TRD_send, 0);
        TerminateThread(TRD_recv, 0);
        #endif

        EMUCCloseDevice(ini_item.com_port);
        exit(0);
    }

    /* prevent windows compiler warning */
    #if defined(_WIN32)
    return TRUE;
    #endif

} /* END: end_emuc() */
