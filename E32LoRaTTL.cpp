#include "E32LoRaTTL.h"

E32LoRaTTL::E32LoRaTTL(int M0, int M1, int AUX, Stream * _lora, Stream * _debug)
{
    this->m_M0 = M0;
    this->m_M1 = M1;
    this->m_AUX = AUX;
    this->m_LoRa = _lora;
    this->m_Debug = _debug;

    pinMode(this->m_M0, OUTPUT);
    pinMode(this->m_M1, OUTPUT);
    pinMode(this->m_AUX, INPUT);
}

bool E32LoRaTTL::ready()
{
    return (digitalRead(this->m_AUX) == HIGH);
}

RET_STATUS E32LoRaTTL::SetAddressAndChannel(uint8_t addrH, uint8_t addrL, AIR_CHAN_TYPE channel)
{
    // Fixed-point transmission mode, +10dB by default
    return SetAddressAndChannel(addrH, addrL, channel, 
        TRANSMISSION_MODE::TRSM_FP_MODE, TRANSMISSION_POWER::TSMT_PWR_10DB);
}

RET_STATUS E32LoRaTTL::SetAddressAndChannel(uint8_t addrH, uint8_t addrL, AIR_CHAN_TYPE channel, 
        TRANSMISSION_MODE transmissionMode, TRANSMISSION_POWER transmissionPower)
{
    struct CFGstruct CFG;
    RET_STATUS status = RET_SUCCESS;

    status = SleepModeCmd(R_CFG, (void* )&CFG);

    if (status != RET_SUCCESS)
    {
        return status;
    }

    CFG.ADDL = addrL;
    CFG.ADDH = addrH;
    CFG.CHAN = channel;

    this->m_TransmissionMode = transmissionMode;

    CFG.OPTION_bits.trsm_mode = transmissionMode;
    CFG.OPTION_bits.tsmt_pwr = transmissionPower;

    status = SettingModule(&CFG);

    if (status != RET_SUCCESS)
    {
        return status;
    }

    SwitchMode(MODE_0_NORMAL);

    return status;
}

RET_STATUS E32LoRaTTL::SleepModeCmd(uint8_t CMD, void* pBuff)
{
  RET_STATUS STATUS = RET_SUCCESS;

  if (m_Debug != NULL)
  {
    m_Debug->print(F("SleepModeCmd: 0x"));  m_Debug->println(CMD, HEX);
  }

  waitReady();

  SwitchMode(MODE_3_SLEEP);

  switch (CMD)
  {
    case W_CFG_PWR_DWN_SAVE:
      STATUS = Write_CFG_PDS((struct CFGstruct* )pBuff);
      break;
    case R_CFG:
      STATUS = Read_CFG((struct CFGstruct* )pBuff);
      break;
    case W_CFG_PWR_DWN_LOSE:
      break;
    case R_MODULE_VERSION:
      Read_module_version((struct MVerstruct* )pBuff);
      break;
    case W_RESET_MODULE:
      Reset_module();
      break;

    default:
      return RET_INVALID_PARAM;
  }

  waitReady();
  return STATUS;
}

RET_STATUS E32LoRaTTL::SettingModule(struct CFGstruct *pCFG)
{
  RET_STATUS STATUS = RET_SUCCESS;

  STATUS = SleepModeCmd(W_CFG_PWR_DWN_SAVE, (void* )pCFG);

  SleepModeCmd(W_RESET_MODULE, NULL);

  STATUS = SleepModeCmd(R_CFG, (void* )pCFG);

  return STATUS;
}

RET_STATUS E32LoRaTTL::waitReady()
{
    RET_STATUS STATUS = RET_SUCCESS;

    uint8_t cnt = 0;

    while((!ready()) && (cnt++<TIME_OUT_CNT))
    {
        if (m_Debug != NULL)
        {
            m_Debug->print(".");
        }

        delay(100);
    }

    if(cnt==0)
    {
    }
    else if(cnt>=TIME_OUT_CNT)
    {
        STATUS = RET_TIMEOUT;
        if (m_Debug != NULL)
        {
            m_Debug->println(F(" TimeOut"));
        }
        
    }
    else
    {
        if (m_Debug != NULL)
        {
            m_Debug->println("");
        }
    }

    return STATUS;
}

bool E32LoRaTTL::chkModeSame(MODE_TYPE mode)
{
  static MODE_TYPE pre_mode = MODE_INIT;

  if(pre_mode == mode)
  {
    return true;
  }
  else
  {
    if (m_Debug != NULL)
    {
      m_Debug->print(F("SwitchMode: from "));  
      m_Debug->print(pre_mode, HEX);  
      m_Debug->print(F(" to "));  
      m_Debug->println(mode, HEX);
    }
    pre_mode = mode;
    return false;
  }
}

void E32LoRaTTL::SwitchMode(MODE_TYPE mode)
{
  if(!chkModeSame(mode))
  {
    waitReady();

    switch (mode)
    {
      case MODE_0_NORMAL:
        digitalWrite(m_M0, LOW);
        digitalWrite(m_M1, LOW);
        break;
      case MODE_1_WAKE_UP:
        digitalWrite(m_M0, HIGH);
        digitalWrite(m_M1, LOW);
        break;
      case MODE_2_POWER_SAVIN:
        digitalWrite(m_M0, LOW);
        digitalWrite(m_M1, HIGH);
        break;
      case MODE_3_SLEEP:
        digitalWrite(m_M0, HIGH);
        digitalWrite(m_M1, HIGH);
        break;
      default:
        return ;
    }

    waitReady();
    delay(10);
  }
}

void E32LoRaTTL::cleanUARTBuf()
{
  while (m_LoRa->available())
  {
    m_LoRa->read();
  }
}

void E32LoRaTTL::triple_cmd(SLEEP_MODE_CMD_TYPE Tcmd)
{
  uint8_t CMD[3] = {Tcmd, Tcmd, Tcmd};
  m_LoRa->write(CMD, 3);
  delay(50);  //need to check
}

RET_STATUS E32LoRaTTL::GetModuleInfo(uint8_t* pReadbuf, uint8_t buf_len)
{
  RET_STATUS STATUS = RET_SUCCESS;
  uint8_t Readcnt, idx;

  Readcnt = m_LoRa->available();
  if (Readcnt == buf_len)
  {
    for(idx=0;idx<buf_len;idx++)
    {
      *(pReadbuf+idx) = m_LoRa->read();
      if (m_Debug != NULL)
      {
        m_Debug->print(F(" 0x"));
        m_Debug->print(0xFF & *(pReadbuf+idx), HEX);    // print as an ASCII-encoded hexadecimal
        m_Debug->println("");
      }
    }
    
  }
  else
  {
    STATUS = RET_DATA_SIZE_NOT_MATCH;
    if (m_Debug != NULL)
    {
      m_Debug->print(F("  RET_DATA_SIZE_NOT_MATCH - Readcnt: "));  m_Debug->println(Readcnt);
    }
    cleanUARTBuf();
  }

  return STATUS;
}

RET_STATUS E32LoRaTTL::Write_CFG_PDS(struct CFGstruct* pCFG)
{
  m_LoRa->write((uint8_t *)pCFG, 6);

  waitReady();
  delay(1200);  //need ti check

  return RET_SUCCESS;
}

RET_STATUS E32LoRaTTL::Read_CFG(struct CFGstruct* pCFG)
{
  RET_STATUS STATUS = RET_SUCCESS;

  //1. read UART buffer.
  cleanUARTBuf();

  //2. send CMD
  triple_cmd(R_CFG);

  //3. Receive configuration
  STATUS = GetModuleInfo((uint8_t *)pCFG, sizeof(CFGstruct));
  if((STATUS == RET_SUCCESS) && (m_Debug != NULL))
  {
    m_Debug->print(F("  HEAD:     "));  m_Debug->println(pCFG->HEAD, HEX);
    m_Debug->print(F("  ADDH:     "));  m_Debug->println(pCFG->ADDH, HEX);
    m_Debug->print(F("  ADDL:     "));  m_Debug->println(pCFG->ADDL, HEX);
    m_Debug->print(F("  CHAN:     "));  m_Debug->println(pCFG->CHAN, HEX);
  }

  return STATUS;
}

RET_STATUS E32LoRaTTL::Read_module_version(struct MVerstruct* MVer)
{
  RET_STATUS STATUS = RET_SUCCESS;

  //1. read UART buffer.
  cleanUARTBuf();

  //2. send CMD
  triple_cmd(R_MODULE_VERSION);

  //3. Receive configure
  STATUS = GetModuleInfo((uint8_t *)MVer, sizeof(MVerstruct));
  if((STATUS == RET_SUCCESS) && (m_Debug != NULL))
  {
    m_Debug->print(F("  HEAD:     0x"));  m_Debug->println(MVer->HEAD, HEX);
    m_Debug->print(F("  Model:    0x"));  m_Debug->println(MVer->Model, HEX);
    m_Debug->print(F("  Version:  0x"));  m_Debug->println(MVer->Version, HEX);
    m_Debug->print(F("  features: 0x"));  m_Debug->println(MVer->features, HEX);
  }

  return RET_SUCCESS;
}

void E32LoRaTTL::Reset_module()
{
  triple_cmd(W_RESET_MODULE);
  waitReady();
  delay(1000);
}

RET_STATUS E32LoRaTTL::ReceiveMsg(uint8_t *pdatabuf, uint8_t *data_len)
{
    SwitchMode(MODE_0_NORMAL);

    int idx = 0; 
    int avail = 0;

    while (((avail = m_LoRa->available()) > 0) || (digitalRead(this->m_AUX) == LOW))
    {
        if (avail > 0)
        {
            *(pdatabuf+idx) = m_LoRa->read();
            idx++;
        }
    }

    *data_len = idx;
    *(pdatabuf+idx) = 0;

    return idx > 0 ? RET_SUCCESS : RET_NOT_IMPLEMENT;
}

RET_STATUS E32LoRaTTL::SendMsg(uint8_t addrH, uint8_t addrL, AIR_CHAN_TYPE channel, 
                                uint8_t *pdatabuf, uint8_t data_len)
{
    SwitchMode(MODE_0_NORMAL);

    if(!ready())
    {
        return RET_NOT_READY;
    }

    if (m_TransmissionMode == TRANSMISSION_MODE::TRSM_FP_MODE)
    {
        m_LoRa->write(addrH);
        m_LoRa->write(addrL);
        m_LoRa->write(channel);
    }

    m_LoRa->write(pdatabuf, data_len);

  return RET_SUCCESS;
}

