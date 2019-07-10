#ifndef E32_LORA_TTL_H_
#define E32_LORA_TTL_H_

#include <Arduino.h>

typedef enum 
{
    RET_SUCCESS = 0,
    RET_ERROR_UNKNOWN,	/* something shouldn't happened */
    RET_NOT_SUPPORT,
    RET_NOT_IMPLEMENT,
    RET_NOT_INITIAL,
    RET_INVALID_PARAM,
    RET_DATA_SIZE_NOT_MATCH,
    RET_BUF_TOO_SMALL,
    RET_TIMEOUT,
    RET_HW_ERROR,
} RET_STATUS;

enum MODE_TYPE
{
    MODE_0_NORMAL = 0,
    MODE_1_WAKE_UP,
    MODE_2_POWER_SAVIN,
    MODE_3_SLEEP,
    MODE_INIT = 0xFF
};

enum SLEEP_MODE_CMD_TYPE
{
    W_CFG_PWR_DWN_SAVE = 0xC0,
    R_CFG              = 0xC1,
    W_CFG_PWR_DWN_LOSE = 0xC2,
    R_MODULE_VERSION   = 0xC3,
    W_RESET_MODULE     = 0xC4
};

enum UART_FORMAT_TYPE
{
    UART_FORMAT_8N1 = 0x00,  /*no   parity bit one stop*/
    UART_FORMAT_8O1 = 0x01,  /*odd  parity bit one stop*/
    UART_FORMAT_8E1 = 0x02   /*even parity bitone stop*/
};

enum UART_BPS_TYPE
{
    UART_BPS_1200 = 0x00,
    UART_BPS_9600 = 0x03,
    UART_BPS_115200 = 0x07
};

enum AIR_BPS_TYPE
{
    AIR_BPS_300   = 0x00,
    AIR_BPS_2400  = 0x02,
    AIR_BPS_19200 = 0x05
};

//410~441MHz : 410M + CHAN*1M
enum AIR_CHAN_TYPE
{
    AIR_CHAN_410M = 0x00,
    AIR_CHAN_433M = 0x17,
    AIR_CHAN_441M = 0x1F
};

//OPTION+
enum TRANSMISSION_MODE
{
    TRSM_TT_MODE = 0x00,
    TRSM_FP_MODE = 0x01
};

#define OD_DRIVE_MODE		0x00
#define PP_DRIVE_MODE		0x01

enum WEAK_UP_TIME_TYPE
{
    WEAK_UP_TIME_250  = 0x00,
    WEAK_UP_TIME_1000 = 0x03,
    WEAK_UP_TIME_2000 = 0x07
};

#define DISABLE_FEC			0x00
#define ENABLE_FEC			0x01

//Transmit power
enum TRANSMISSION_POWER
{
    TSMT_PWR_20DB = 0x00,
    TSMT_PWR_17DB = 0x01,
    TSMT_PWR_14DB = 0x02,
    TSMT_PWR_10DB = 0x03
};
//OPTION-

#pragma pack(push, 1)
struct SPEDstruct {
    uint8_t air_bps : 3; //bit 0-2
    uint8_t uart_bps: 3; //bit 3-5
    uint8_t uart_fmt: 2; //bit 6-7
};

struct OPTIONstruct {
    uint8_t tsmt_pwr    : 2; //bit 0-1
    uint8_t enFWC       : 1; //bit 2
    uint8_t wakeup_time : 3; //bit 3-5
    uint8_t drive_mode  : 1; //bit 6
    uint8_t trsm_mode   : 1; //bit 7
};

struct CFGstruct {
    uint8_t HEAD;
    uint8_t ADDH;
    uint8_t ADDL;
    struct SPEDstruct   SPED_bits;
    uint8_t CHAN;
    struct OPTIONstruct OPTION_bits;
};

struct MVerstruct {
    uint8_t HEAD;
    uint8_t Model;
    uint8_t Version;
    uint8_t features;
};
#pragma pack(pop)

#define TIME_OUT_CNT	100
#define MAX_TX_SIZE		58

class E32LoRaTTL
{
private:
    int m_M0;
    int m_M1;
    int m_AUX;
    TRANSMISSION_MODE m_TransmissionMode;
    Stream * m_LoRa;
    Stream * m_Debug;
    bool ready();
    bool chkModeSame(MODE_TYPE mode);
    void cleanUARTBuf();
    void triple_cmd(SLEEP_MODE_CMD_TYPE Tcmd);
    RET_STATUS Write_CFG_PDS(struct CFGstruct* pCFG);
    RET_STATUS Read_CFG(struct CFGstruct* pCFG);
    RET_STATUS Read_module_version(struct MVerstruct* MVer);
    RET_STATUS GetModuleInfo(uint8_t* pReadbuf, uint8_t buf_len);
    void Reset_module();
    RET_STATUS SleepModeCmd(uint8_t CMD, void* pBuff);
    RET_STATUS SettingModule(struct CFGstruct *pCFG);
public:
    E32LoRaTTL(int M0, int M1, int AUX, Stream * _lora, Stream * _debug);
    RET_STATUS SetAddressAndChannel(uint8_t addrH, uint8_t addrL, AIR_CHAN_TYPE channel);
    RET_STATUS SetAddressAndChannel(uint8_t addrH, uint8_t addrL, AIR_CHAN_TYPE channel, 
            TRANSMISSION_MODE transmissionMode, TRANSMISSION_POWER transmissionPower);
    RET_STATUS waitReady();
    RET_STATUS ReceiveMsg(uint8_t *pdatabuf, uint8_t *data_len);
    RET_STATUS SendMsg(uint8_t addrH, uint8_t addrL, AIR_CHAN_TYPE channel, uint8_t *pdatabuf, uint8_t data_len);
    void SwitchMode(MODE_TYPE mode);
};

#endif