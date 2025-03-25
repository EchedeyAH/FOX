#define   PCIE86XX_AI_BI_5V     0x0
#define   PCIE86XX_AI_BI_10V    0x1

#define   PCIE86XX_AO_UNI_5V    0       //0  ~  5V
#define   PCIE86XX_AO_BI_5V     1       //+/-   5V
#define   PCIE86XX_AO_UNI_10V   2       //0  ~ 10V
#define   PCIE86XX_AO_BI_10V    3       //+/-  10V

/* IXPCI strcture for read EEPROM */
typedef struct ixpci_eep{
        unsigned long offset;
        unsigned char uBLOCK;           /* 0 Read EEPROM, 1 Recover EEPROM */
        unsigned short count;
        unsigned short read_eep[128];   /* Read EEPROM value */
        float CalAI[2][16];
        float CalAIShift[2][16];
        float CalAO[4][3];
        float CalAOShitf[4][3];
        char AI_ch;                     /* card's AI channel numbers */
        char AO_ch;                     /* card's AO channel numbers */
} ixpci_eep_t;

void calibration(ixpci_eep_t*);
