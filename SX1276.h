/*  SX1276_h - Library for control of Semtech SX1276 LoRa Radio in LoRa Mode
 *  Created by George Mortimer, Lockdown 2020.
 *  
 *  Limitations: Only LoRa mode implemented, no FSK mode support.
 *  Caution: Don't assume your transmissions are legal. Check.
 *
 *  Released into the public domain.
 */
#ifndef SX1276_h
#define SX1276_h
#include <string>

#ifdef ESP32
  #include "Arduino.h"
  #include <SPI.h>
  #define NSS_PIN_DEFAULT 15
  #define RESET_PIN_DEFAULT 2
#else
  #include <wiringPi.h>
  #include <wiringPiSPI.h>
  #include <math.h>
  using std::round;
  #define NSS_PIN_DEFAULT 6
  #define RESET_PIN_DEFAULT 0
#endif

#define TIMEOUT_DEFAULT    5000

#define SX1276_FSK         0
#define SX1276_LORA        1
#define BANDPLAN_NONE      0
#define BANDPLAN_EU868     1
#define OUTPUT_RFO         0 
#define OUTPUT_PA_BOOST    1 
#define SX1276_MODE_SLEEP  0
#define SX1276_MODE_STDBY  1 
#define SX1276_MODE_FSTX   2
#define SX1276_MODE_TX     3 
#define SX1276_MODE_FSRX   4 
#define SX1276_MODE_RXCONTINUOUS  5 
#define SX1276_MODE_RXSINGLE      6 
#define SX1276_MODE_CAD    7 

class SX1276
{
  public:
  
    SX1276            (int spiClk = 1000000, 
                       uint8_t NSS_Pin = NSS_PIN_DEFAULT,  
                       uint8_t ResetPin = RESET_PIN_DEFAULT, 
                       uint8_t SCK_Pin = 14, 
                       uint8_t MISO_Pin = 12, 
                       uint8_t MOSI_Pin = 13);
    int Frequency     (uint32_t Freq = 0);
    int Init          (uint8_t PA_Boost = OUTPUT_RFO, 
                       uint8_t BandPlan = BANDPLAN_NONE);
    int8_t PowerDBm   (int8_t NewPower = -99);
    int32_t BwHz      (int32_t BandWidth = 0);
    int TX            (char  *datain,      
                       size_t datalen);
    int RXContinuous  (char  *rxdata,      
                       size_t datalen,         
                       uint16_t timeout = TIMEOUT_DEFAULT);
    int RXContStart   (char *rxdata,     
                       size_t datalen);
    int CAD           (char  *rxdata,      
                       size_t datalen,         
                       uint16_t timeout = TIMEOUT_DEFAULT);
    void ClearFlags();
    int Reset();
    int TxTimer(uint32_t TXTimeToAdd = 0);

 /* Direct Register Read/Write Function Prototypes */   
 
    uint8_t Fifo();
    uint8_t Fifo(uint8_t x);
    uint8_t LongRangeMode();
    uint8_t LongRangeMode(uint8_t x);
    uint8_t AccessSharedReg();
    uint8_t AccessSharedReg(uint8_t x);
    uint8_t LowFrequencyModeOn();
    uint8_t LowFrequencyModeOn(uint8_t x);
    uint8_t Mode();
    uint8_t Mode(uint8_t x);
    uint32_t Frf();
    uint32_t Frf(uint32_t x);
    uint8_t PaSelect();
    uint8_t PaSelect(uint8_t x);
    uint8_t MaxPower();
    uint8_t MaxPower(uint8_t x);
    uint8_t OutputPower();
    uint8_t OutputPower(uint8_t x);
    uint8_t PaRamp();
    uint8_t PaRamp(uint8_t x);
    uint8_t OcpOn();
    uint8_t OcpOn(uint8_t x);
    uint8_t OcpTrim();
    uint8_t OcpTrim(uint8_t x);
    uint8_t LnaGain();
    uint8_t LnaGain(uint8_t x);
    uint8_t LnaBoostLf();
    uint8_t LnaBoostLf(uint8_t x);
    uint8_t LnaBoostHf();
    uint8_t LnaBoostHf(uint8_t x);
    uint8_t FifoAddrPtr();
    uint8_t FifoAddrPtr(uint8_t x);
    uint8_t FifoTxBaseAddr();
    uint8_t FifoTxBaseAddr(uint8_t x);
    uint8_t FifoRxBaseAddr();
    uint8_t FifoRxBaseAddr(uint8_t x);
    uint8_t FifoRxCurrentAddr();
    uint8_t RxTimeoutMask();
    uint8_t RxTimeoutMask(uint8_t x);
    uint8_t RxDoneMask();
    uint8_t RxDoneMask(uint8_t x);
    uint8_t PayloadCrcErrorMask();
    uint8_t PayloadCrcErrorMask(uint8_t x);
    uint8_t ValidHeaderMask();
    uint8_t ValidHeaderMask(uint8_t x);
    uint8_t TxDoneMask();
    uint8_t TxDoneMask(uint8_t x);
    uint8_t CadDoneMask();
    uint8_t CadDoneMask(uint8_t x);
    uint8_t FhssChangeChannelMask();
    uint8_t FhssChangeChannelMask(uint8_t x);
    uint8_t CadDetectedMask();
    uint8_t CadDetectedMask(uint8_t x);
    uint8_t RxTimeout();
    uint8_t RxTimeout(uint8_t x);
    uint8_t RxDone();
    uint8_t RxDone(uint8_t x);
    uint8_t PayloadCrcError();
    uint8_t PayloadCrcError(uint8_t x);
    uint8_t ValidHeader();
    uint8_t ValidHeader(uint8_t x);
    uint8_t TxDone();
    uint8_t TxDone(uint8_t x);
    uint8_t CadDone();
    uint8_t CadDone(uint8_t x);
    uint8_t FhssChangeChannel();
    uint8_t FhssChangeChannel(uint8_t x);
    uint8_t CadDetected();
    uint8_t CadDetected(uint8_t x);
    uint8_t FifoRxBytesNb();
    uint16_t ValidHeaderCnt();
    uint16_t ValidPacketCnt();
    uint8_t RxCodingRate();
    uint8_t ModemStatus();
    uint8_t PacketSnr();
    uint8_t PacketRssi();
    uint8_t Rssi();
    uint8_t PllTimeout();
    uint8_t CrcOnPayload();
    uint8_t FhssPresentChannel();
    uint8_t Bw();
    uint8_t Bw(uint8_t x);
    uint8_t CodingRate();
    uint8_t CodingRate(uint8_t x);
    uint8_t ImplicitHeaderModeOn();
    uint8_t ImplicitHeaderModeOn(uint8_t x);
    uint8_t SpreadingFactor();
    uint8_t SpreadingFactor(uint8_t x);
    uint8_t TxContinuousMode();
    uint8_t TxContinuousMode(uint8_t x);
    uint8_t RxPayloadCrcOn();
    uint8_t RxPayloadCrcOn(uint8_t x);
    uint16_t SymbTimeout();
    uint16_t SymbTimeout(uint16_t x);
    uint16_t PreambleLength();
    uint16_t PreambleLength(uint16_t x);
    uint8_t PayloadLength();
    uint8_t PayloadLength(uint8_t x);
    uint8_t PayloadMaxLength();
    uint8_t PayloadMaxLength(uint8_t x);
    uint8_t FreqHoppingPeriod();
    uint8_t FreqHoppingPeriod(uint8_t x);
    uint8_t FifoRxByteAddrPtr();
    uint8_t LowDataRateOptimize();
    uint8_t LowDataRateOptimize(uint8_t x);
    uint8_t AgcAutoOn();
    uint8_t AgcAutoOn(uint8_t x);
    uint8_t PpmCorrection();
    uint8_t PpmCorrection(uint8_t x);
    uint32_t FreqError();
    uint8_t RssiWideband();
    uint8_t IfFreq2();
    uint8_t IfFreq2(uint8_t x);
    uint8_t IfFreq1();
    uint8_t IfFreq1(uint8_t x);
    uint8_t AutomaticIFOn();
    uint8_t AutomaticIFOn(uint8_t x);
    uint8_t DetectionOptimize();
    uint8_t DetectionOptimize(uint8_t x);
    uint8_t InvertIQ_RX();
    uint8_t InvertIQ_RX(uint8_t x);
    uint8_t InvertIQ_TX();
    uint8_t InvertIQ_TX(uint8_t x);
    uint8_t HighBWOptimize1();
    uint8_t HighBWOptimize1(uint8_t x);
    uint8_t DetectionThreshold();
    uint8_t DetectionThreshold(uint8_t x);
    uint8_t SyncWord();
    uint8_t SyncWord(uint8_t x);
    uint8_t HighBWOptimize2();
    uint8_t HighBWOptimize2(uint8_t x);
    uint8_t InvertIQ2();
    uint8_t InvertIQ2(uint8_t x);
    uint8_t Dio0Mapping();
    uint8_t Dio0Mapping(uint8_t x);
    uint8_t Dio1Mapping();
    uint8_t Dio1Mapping(uint8_t x);
    uint8_t Dio2Mapping();
    uint8_t Dio2Mapping(uint8_t x);
    uint8_t Dio3Mapping();
    uint8_t Dio3Mapping(uint8_t x);
    uint8_t Dio4Mapping();
    uint8_t Dio4Mapping(uint8_t x);
    uint8_t Dio5Mapping();
    uint8_t Dio5Mapping(uint8_t x);
    uint8_t Version();
    uint8_t PaDac();
    uint8_t PaDac(uint8_t x);
    uint8_t FormerTemp();
    uint8_t AgcReferenceLevel();
    uint8_t AgcReferenceLevel(uint8_t x);
    uint8_t AgcStep1();
    uint8_t AgcStep1(uint8_t x);
    uint8_t AgcStep2();
    uint8_t AgcStep2(uint8_t x);
    uint8_t AgcStep3();
    uint8_t AgcStep3(uint8_t x);
    uint8_t AgcStep4();
    uint8_t AgcStep4(uint8_t x);
    uint8_t AgcStep5();
    uint8_t AgcStep5(uint8_t x);
    uint8_t PllBandwidth();
    uint8_t PllBandwidth(uint8_t x);


  private:
    #ifdef ESP32
    SPIClass * spi = NULL;
    #endif
    int _spiClk;    
    uint8_t spi_rx(uint8_t addr,
                   uint8_t bits=8,
                   uint8_t bitshift=0);
    uint8_t spi_tx(uint8_t addr, 
                   uint8_t spi_data,
                   uint8_t bits=8,
                   uint8_t bitshift=0);
    uint8_t _NSS_pin;
    uint8_t _ResetPin;
    int _BandPlan;
    int _FreqLimitLower;
    int _FreqLimitUpper;
    int _TXPowerLimit;
    uint32_t _DutyCycleMsHour;
    uint16_t _TXHoldoff;
    int _BWLimit;
    int _TXSeconds;
    uint32_t _TxTimerMs;
    uint32_t _TXHoldUntil;
    uint32_t _TXwindowTime[10];
    uint32_t _TXTimerWindowRef;
    char * _RxDataPtr;
    size_t _RxDataLen;
    const uint8_t RegFifo = 0x00;
    const uint8_t RegOpMode = 0x01;
    const uint8_t RegFrMsb = 0x06;
    const uint8_t RegFrMid = 0x07;
    const uint8_t RegFrLsb = 0x08;
    const uint8_t RegPaConfig = 0x09;
    const uint8_t RegPaRamp = 0x0A;
    const uint8_t RegOcp = 0x0B;
    const uint8_t RegLna = 0x0C;
    const uint8_t RegFifoAddrPtr = 0x0D;
    const uint8_t RegFifoTxBaseAddr = 0x0E;
    const uint8_t RegFifoRxBaseAddr = 0x0F;
    const uint8_t RegFifoRxCurrentAddr = 0x10;
    const uint8_t RegIrqFlagsMask = 0x11;
    const uint8_t RegIrqFlags = 0x12;
    const uint8_t RegRxNbBytes = 0x13;
    const uint8_t RegRxHeaderCntValueMsb = 0x14;
    const uint8_t RegRxHeaderCntValueLsb = 0x15;
    const uint8_t RegRxPacketCntValueMsb = 0x16;
    const uint8_t RegRxPacketCntValueLsb = 0x17;
    const uint8_t RegModemStat = 0x18;
    const uint8_t RegPktSnrValue = 0x19;
    const uint8_t RegPktRssiValue = 0x1A;
    const uint8_t RegRssiValue = 0x1B;
    const uint8_t RegHopChannel = 0x1C;
    const uint8_t RegModemConfig1 = 0x1D;
    const uint8_t RegModemConfig2 = 0x1E;
    const uint8_t RegSymbTimeoutLsb = 0x1F;
    const uint8_t RegPreambleMsb = 0x20;
    const uint8_t RegPreambleLsb = 0x21;
    const uint8_t RegPayloadLength = 0x22;
    const uint8_t RegMaxPayloadLength = 0x23;
    const uint8_t RegHopPeriod = 0x24;
    const uint8_t RegFifoRxByteAddr = 0x25;
    const uint8_t RegModemConfig3 = 0x26;
    const uint8_t RegPpmCorrection = 0x27;
    const uint8_t RegFeiMsb = 0x28;
    const uint8_t RegFeiMid = 0x29;
    const uint8_t RegFeiLsb = 0x2A;
    const uint8_t RegRssiWideband = 0x2C;
    const uint8_t RegIfFreq2 = 0x2F;
    const uint8_t RegIfFreq1 = 0x30;
    const uint8_t RegDetectOptimize = 0x31;
    const uint8_t RegInvertIQ = 0x33;
    const uint8_t RegHighBWOptimize1 = 0x36;
    const uint8_t RegDetectionThreshold = 0x37;
    const uint8_t RegSyncWord = 0x39;
    const uint8_t RegHighBWOptimize2 = 0x3A;
    const uint8_t RegInvertIQ2 = 0x3B;
    const uint8_t RegDioMapping1 = 0x40;
    const uint8_t RegDioMapping2 = 0x41;
    const uint8_t RegVersion = 0x42;
    const uint8_t RegPaDAC = 0x4D;
    const uint8_t RegFormerTemp = 0x5B;
    const uint8_t RegAgcRef = 0x62;
    const uint8_t RegAgcThresh1 = 0x62;
    const uint8_t RegAgcThresh2 = 0x63;
    const uint8_t RegAgcThresh3 = 0x64;
    const uint8_t RegPll = 0x70;

};

#endif
