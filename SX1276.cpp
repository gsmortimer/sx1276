/*
  SX1276.cpp - Library for control of Semtech SX1276 LoRa Radio in LoRa Mode
  Created by George Mortimer, Lockdown 2020.
  Released into the public domain.

  This library is based on SX1276/7/8 Datasheet
  It has been tested a bit with SX1276 / RFM95W
  It probably works with SX1277 & SX1278 but has not been tested.
  Nothing clever here, just a load of routines to automate things,
   combined with a full set of low level commands.
  This should allow you to easily create and customise LoRa based
   Tx/Rx making use of all the SX1276 custom functions.
  Incorrect configuration is very likely to result in transmission
   not legal in your region. You have been warned!
  The "band plan feature" is designed to reduce this risk, but does not 
   eliminate it.

  call Init(1,1) to set up the modem for general use.
*/


/*   Debugging routines - prints debug messages  */
#ifdef DEBUG_BUILD
#  define DEBUG(...) { printf("DEBUG:  "); printf(__VA_ARGS__); printf("\r\n"); }
#else
#  define DEBUG(...) { do {} while (0); }
#endif

/*   Arduino/ESP32 specific includes   */
#ifdef ESP32
  #include "Arduino.h"
  #include <SPI.h>
  
/*   Raspberry pi specific includes   */
#else
  #include <wiringPi.h>
  #include <wiringPiSPI.h>
  #include <math.h>
  using std::round;
#endif

/*   Includes   */

#include <string>
#include "SX1276.h"


/*  SX1276
 *   
 *  Class initialisation. and Set up of SPI connection to modem. Also resets Modem
 *  Pi GPIO Numbering uses WiringPi convention.
 */
SX1276::
SX1276 (int     spiClk,    // [Optional] Default: 1000000. SPI clock speed in Hz. 
        uint8_t NSS_Pin,   // [Optional] Default: 15(Arduino) 6(Pi). GPIO pin number for SX1276 NSS. For Pi in addition to SPI0_CS0
        uint8_t ResetPin,  // Optional, Default: 15(Arduino) 0(Pi).  GPIO pin number for SX1276 Reset.   
        uint8_t SCK_Pin,   // Optional, Default: 14. GPIO pin number for SX1276 SCK_Pin. Not used for Pi, which is always SPI0_SCLK
        uint8_t MISO_Pin,  // Optional, Default: 12. GPIO pin number for SX1276 MISO_Pin. Not used for Pi, which is always SPI0_MISO
        uint8_t MOSI_Pin)  // Optional, Default: 13. GPIO pin number for SX1276 MOSI_Pin. Not used for Pi, which is always SPI0_MOSI
{
  _NSS_pin = NSS_Pin;
  _ResetPin = ResetPin;
  _spiClk = spiClk;

  /*   SPI setup   */  

  #ifdef ESP32
    pinMode (_NSS_pin, OUTPUT);
    spi = new SPIClass(HSPI);
    spi->begin(SCK_Pin, MISO_Pin, MOSI_Pin, NSS_Pin);
  #else
    wiringPiSetup() ;
    delay(10);
    wiringPiSPISetup(0,_spiClk);
    delay(10); 
    pinMode (_NSS_pin, OUTPUT);
  #endif 
  /*   Reset SX1276   */ 
 
  Reset();
  //_FreqLimitLower = 137e6;  // Lowest Frequency Range for SX1276
  //_FreqLimitUpper = 1020e6; // Highest Frequency Range for SX1276
}


/*  Init 
 *   
 *  Set up modem to LoRa mode, and optionally set regional restrictions 
 *  
 *  Returns: 0 if successful:
 *           -1 if modem reset failed. 
 */
int SX1276::
Init (uint8_t PA_Boost, //Optional.
                        // OUTPUT_RFO (default): Antenna connected to RFO-LF/HF. 
                        // OUTPUT_PA_BOOST: Antenna connected to PA_Boost (use this for RFM95 Boards)
      uint8_t BandPlan) //Optional.
                        // BANDPLAN_NONE (default): No Bandplan, unrestricted Frequency and Power output. Frequency left as factory default.
                        // BANDPLAN_EU868: EU868 Band Restrictions. ref EN 300 220-2 V3.2.1. Frequency set to 868MHz.
{ 
  if (PA_Boost != OUTPUT_RFO && PA_Boost != OUTPUT_PA_BOOST)
  {
    DEBUG ("Init Error: PA_Boost Out of Range");
    return -1;
  }
/* Initialise TX Timer  */
  _TxTimerMs = 0;
  for (int p=0;p<10;p++)
  {
    _TXwindowTime[p]=0;
  }
  _TXHoldUntil = millis();

/* Initialise Modem  */
  if (Reset() != 0)
  {
    DEBUG ("Init Error: Modem reset failure");
    return -1;
  }
  Mode(SX1276_MODE_SLEEP);
  delay(10);
  LongRangeMode(SX1276_LORA);
  AutomaticIFOn(0); // Per errata note. (Spurious Reception)
  IfFreq2(0x40);    // Per errata note. (Spurious Reception)
  IfFreq1(0x00);    // Per errata note. (Spurious Reception)
  delay(10);
  Mode(SX1276_MODE_STDBY);
  PaSelect(PA_Boost);

/* Configure Band Plan Limits */

  _BandPlan = BandPlan;
  if (_BandPlan == BANDPLAN_NONE) // No Band Restrictions
  {
    _TXPowerLimit = 20;       // Max
    _DutyCycleMsHour = 1800000;    // Seconds, 50% Duty
    _TXHoldoff = 0;           // Allow continuous Trasmission
    _BWLimit = 9;
  }

  else if (_BandPlan == BANDPLAN_EU868) // EU868 Restrictions ref EN 300 220-2 V3.2.1
  {
    //_FreqLimitLower = 863e6;   // Lowest Frequency 
    //_FreqLimitUpper = 870e6;  // Highest Frequency
    _TXHoldoff = 1;           // Times TX period. Need to find figure for this.
    Frequency(869.5e6);        // set Freq for 869.5 Mhz (Centre of band 54)
  }
  else
  {
    DEBUG ("Init Error: Invalid Bandplan");
    return -1;
  }
 return 0;
}


/*  CAD
 *
 *  Initiate Channel activity detection. and attempt to decode data.
 *  Remains in CAD mode until either a activity is detected, or timeout reached.
 */
int SX1276::
CAD (char *    rxdata,     //char array to write data to. 
     size_t    datalen,    //set to sizeof(rxdata).
     uint16_t  timeout)    //Timeout period in ms. Default: 5000.
{
    uint32_t t = millis() + timeout; 
    int rx = 1;
    int cadcount=0;
    Mode(SX1276_MODE_STDBY); 
    ClearFlags();
    Mode(SX1276_MODE_CAD);
    DEBUG ("CAD");
    while (rx == 1 && millis() < t) // Monitor IRQ flags and wait until timer is up
    {
      if (CadDetected() == 1)
      {
       DEBUG ("Cad Detected..");
       ClearFlags(); 
       rx = RXContinuous(rxdata,200); //try and RX detected signal. Unlikely to work. remove this?
      }
      else if (CadDone() == 1)
      {
        cadcount++;
        CadDone(1); // Clear CadDone Flag
        Mode(SX1276_MODE_CAD); 
      }
      else
      {
        delay (3); // stop cpu hogging
      }
    }

    DEBUG ("End CAD. cad calls: %d",cadcount);

    ClearFlags();
    Mode(SX1276_MODE_STDBY); 
    return 0;
}

/*  RXContinuous 
 *   
 *  Initiate Receive mode. and attempt to decode data.
 *  Remains in Receive mode until either data recieved, or timeout reached.
 *  Returns Number of bytes receieved if packed recieved.
 *  Otherwise returns 0.
 *  If the character array provided was too small, The data is partially written to the array, but -1 is returned.
 */
int SX1276::
RXContinuous (char *    rxdata,    //char array to write data to. 
              size_t    datalen,   //set to sizeof(rxdata).
              uint16_t  timeout)   //Timeout period in ms. Default: 5000.
{
    uint8_t rxbytes;
    int ret;
    uint32_t t = millis() + timeout ; // 
    Mode(SX1276_MODE_STDBY);
    uint8_t FifoRxAddress;
    FifoAddrPtr(FifoRxBaseAddr()); // set Set FifoPtrAddr to FifoRxBaseAddr
    ClearFlags();
    Mode(SX1276_MODE_RXCONTINUOUS); 
    DEBUG ("Rxing.."); 
    while (RxDone() == 0 && (millis() < t || timeout == 0 )) {// Monitor IRQ flags and wait until TxDone Flag is set
      if (ModemStatus() & 1 == 1)
      {
        DEBUG ("Sig Detected..");
        t += 4; //extend timeout if signal detected
      }
      if (ModemStatus() & 2 == 1)
      {
        DEBUG ("Sig Synced..");
      }
       if (ModemStatus() & 4 == 1)
      {
        DEBUG ("RX ongoing..");
      }
       if (ModemStatus() & 8 == 1)
      {
        DEBUG ("Header info valid..");
      }
      if (ModemStatus() & 16 == 1)
      {
        DEBUG ("Modem Clear..");
      }
      delay(3); // stop cpu hogging
    }
    if (RxDone()) 
    {
      rxbytes = FifoRxBytesNb();
      if (rxbytes > datalen) 
      {
        ret = -1;
        rxbytes = datalen;
      }
      else ret = rxbytes;
      FifoRxAddress = FifoRxCurrentAddr(); // get start address of last packet 
      DEBUG ("Fifo address ptr=%d", FifoAddrPtr());
      FifoAddrPtr(FifoRxAddress); // set Set FifoPtrAddr to FifoRxCurrentAddr
      DEBUG ("RX Success. Rxbytes = %d", rxbytes);
      DEBUG ("FifoRxAddress=%d", FifoRxAddress);
      DEBUG ("RXDATA HEX:");
      for (int x = 0; x < rxbytes; x++)
      {
        rxdata[x] = Fifo(); 
        DEBUG (" %x",rxdata[x]);
      }
      DEBUG (rxdata);
//      delay(500);
//      TX(rxdata, rxbytes); //Relay back data *TESTING**

    }
    else
    {
      DEBUG ("Normal RX Timeout.");
      ret = 0;
    }
    if (RxTimeout()) DEBUG ("RxTimeout.");
    if (PayloadCrcError()) DEBUG ("PayloadCrcError");
    if (ValidHeader()) DEBUG ("ValidHeader");
    if (CadDetected()) DEBUG ("CadDetected");
    Mode(SX1276_MODE_STDBY); 
    return ret;
}


/*  RXContStart
 *   
 *  Initiate Receive mode. and attempt to decode data.
 *  Remains in Receive mode until data recieved.
 *  Returns Number of bytes receieved if packed recieved.
 *  Otherwise returns 0.
 *  If the character array provided was too small, The data is partially written to the array, but -1 is returned.
 */
int SX1276::
RXContStart (char *  rxdata,     //char array to write data to. 
             size_t  datalen)    //set to sizeof(rxdata)                   
{
    _RxDataPtr = rxdata;
    _RxDataLen = datalen;
    int ret;
    Mode(SX1276_MODE_STDBY);
    uint8_t FifoRxAddress;
    FifoAddrPtr(FifoRxBaseAddr()); // set Set FifoPtrAddr to FifoRxBaseAddr
    Dio0Mapping(0x00); //Set DIO0 Interrupt Pin to RxDone
    
    Mode(SX1276_MODE_RXCONTINUOUS); 
    DEBUG ("Rxing Continuously.."); 



    return 0;
}

/*  TxTimer
 *  Manage Transmit duty cycle.
 *  We are allowed to Transmit for _DutyCycleMsHour milliseconds per hour.
 *  Ideally we would use a sliding window approach, but this resource intensive.
 *  A simpler method is to use short fixed slots, which don't slide but instead are moved between.
 *  Here we chose 10 360s (360000ms) slots. We allow up to _DutyCycleMsHour TX time for 
 *  the 10 most recent slots, including the slot we are in. 
 *  This time ranges from 9/10 hour to 1 hour, so worst case we could exceed a little  
 *  but overall it will average out. Now i've just written this, lets see if i can code it!
 *  x   x   x   x   x   x   x   x   x   x   *
 *  | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *                                      nnnn  
 *  * = TXTimerWindowRef
 *  n = now
 *  numbers = window number (array index)
 *  
 *  Returns
 *  
 *  Issues: If this isn't called for over 24 days (2^31 ms), then something bad will probably happen. 
 */
int SX1276::
TxTimer (uint32_t  TXTimeToAdd) // TX time to add to counter

{
  uint32_t now = millis();
  int32_t refoffset = now - _TXTimerWindowRef;
  //DEBUG ("refoffset=%d",refoffset);
  //DEBUG ("_TXTimerWindowRef=%d",_TXTimerWindowRef);
  int shiftnumber = 0;
  int p;
  if (refoffset > 0)  //If we have passed the reference time
  {
    shiftnumber = (refoffset / 360000) + 1; // 3600ms(debug). Should be 360000ms (360s or 1/10 hour)
    //DEBUG ("shiftnumber=%d",shiftnumber);
    while (shiftnumber > 1)
    {
      shiftnumber--;
      //shift window time values.
      for (p=9;p>0;p--)
      {
        _TXwindowTime[p] = _TXwindowTime[p-1];
      }
      //zero most recent window
      _TXwindowTime[0]=0;
      //shift windows by 1/10 hour
      _TXTimerWindowRef += 360000; // 3600ms(debug). Should be 360000ms (360s or 1/10 hour)
    }
  }
  _TXwindowTime[0]+=TXTimeToAdd;

 /*  Calculate _TXTimer */
  _TxTimerMs=0;
  for (p=0;p<10;p++)
  {
     //DEBUG ("_TXwindowTime[%d]=%d\n",p,_TXwindowTime[p]);
    _TxTimerMs += _TXwindowTime[p];
  }

 /* if we have exceeded our quota */
  if (_TxTimerMs >= _DutyCycleMsHour)
  {
     DEBUG ("TXTimer Error: Quota Exceeded");
     return -1;
  }
  return _TxTimerMs;
}

/*  TX 
 *  Transmit string of characters.
 *  If no new frequency is provided, the previous Frequency in Hz is returned
 *  If the transmission failed, -1 is returned
 *  If the transmission was successful, the transmission duration in ms is returned
 *  String length limit is 255 bytes
 *  TX Frequency, SF, Bandwidth, Power, CRC, Header mode, and Coding Rate must be set using:
 *  Frequency(), SpreadingFactor(), BandwidthHz, PowerDBm() RxPayloadCrcOn(), 
 *  ImplicitHeaderModeOn(), and CodingRate().
 *  
 *  Returns: Time taken to TX in ms on success
 *           -1 if data to TX is too long
 *           -2 if prevented by holdoff period.
 *           -3 if Duty cycle budget exceeded
 *           -4 if Bandwidth Prohibited by Band Plan
 *           -5 if Tx on Frequency Prohibited by Band Plan
 *
 *  ToDo: If time since last TX is > ~ 25 days then holdoff function may break?
 *  
 */
int SX1276::
TX (char *  datain,     // Array of chars to transmit
    size_t  datalen)    // Length of array (number of chars to transmit)
{
    uint32_t txtime;
    int      tempPowerDBm;
    tempPowerDBm = PowerDBm();
    if (_TXPowerLimit <= -99) // If Tx prohibited on this freq by Band Plan
    {
      DEBUG ("Error: TX Frequency not in band.");      
      return -5;
    }
    if (Bw() > _BWLimit)
    {
      DEBUG ("Error: BW Limit Exceeded.");
      return -4;
    }
    if (datalen == 0 || datalen > 255) {
      DEBUG ("Error TX data too long");
      return -1;
    }
    if  ((int32_t) (_TXHoldUntil - millis()) > 0)
    {
      DEBUG ("Error: Holdoff"); 
      return -2;
    }
    if (TxTimer() < 0)
    {
      DEBUG ("Error: TX Time limit exceeded"); 
      return -3;
    }
    if (tempPowerDBm > _TXPowerLimit)
    {
      DEBUG ("Warning: TX Power %ddB Exceeds Limit of %ddB. Power reduced", PowerDBm(), _TXPowerLimit)
      PowerDBm(_TXPowerLimit);
    }
    Mode(SX1276_MODE_STDBY); 
    PayloadLength(datalen); // write payload length (bytes)
    FifoAddrPtr(FifoTxBaseAddr()); 
    
    // push data byte onto FIFO one byte at a time
    for (int x = 0; x < datalen; x++) {
      Fifo(datain[x]);
    }
    ClearFlags();
    txtime = millis(); 
    Mode(SX1276_MODE_TX); 
    DEBUG ("Txing..");

    // Monitor IRQ flags and wait until TxDone Flag is set or timeout reached
    while (TxDone() == 0 && (uint32_t) (millis() - txtime) < TIMEOUT_DEFAULT ) {
      delay(10);
    }
    txtime = millis() - txtime;
    TxTimer(txtime); 
    _TXHoldUntil = millis() + txtime * _TXHoldoff;
    TxDone(1); // clear TxDone flag
    DEBUG ("TX Done.");
    Mode(SX1276_MODE_STDBY); // set LORA mode, STBY
    PowerDBm(tempPowerDBm);
    return txtime;
}


/*  Frequency 
 *   
 *  Get or Set Tx/Rx Frequency in Hz.
 *  If no new frequency is provided, the previous Frequency in Hz is returned
 *  If the new frequency is out of range, 1 is returned
 *  If the new frequency is in range, 0 is returned
 *  Valid range is 137e6 and 1020e6 (Hardware limit)
 *  If a band plan is enabled: 
 *   - The range is addionally limited
 *   - The Tx Power is reduced according to the band plan
 *   - The Bandwidth is reduced according to the band plan
 *   - The Duty cycle is changed according to the band plan
 *  LowFrequencyMode is changed based on the Frequency
 *  
 *  Returns: Previous frequency setting on success
 *           -1 if failure to set frequency (frequency out of range)
 */
int SX1276::
Frequency (uint32_t Freq) // [Optional] New Frequency in Hz
{
  if (Freq == 0)
  {
    return (uint64_t) (Frf()) * 61035 / 1000; // Assumes 32Mhz Oscillator
  }
  if (Freq < 137e6 || Freq > 1020e6)
  {
    DEBUG ("Frequency Error: Out of Range");
    return -1;
  }
  if (_BandPlan == BANDPLAN_EU868)     //  EU868
  {
    if (Freq >= 863e6 + 62.5e3 && Freq <= 865e6 - 62.5e3) //Band 46a
    {
      _TXPowerLimit = 14;            // 25mW
      _DutyCycleMsHour = 3600;       // 0.1% Duty
      _BWLimit = 7;                  // 125 Khz Max
    }
    else if (Freq >= (865e6 + 62.5e3) && Freq <= (868e6 - 62.5e3)) //Band 47
    {
      _TXPowerLimit = 14;            // 25mW
      _DutyCycleMsHour = 36000;      // 1% Duty
      _BWLimit = 7;                  // 125 Khz Max
    }
    else if (Freq >= (868e6 + 62.5e3) && Freq <= (868.6e6 - 62.5e3)) //Band 48
    {
      _TXPowerLimit = 14;            // 25mW
      _DutyCycleMsHour = 36000;      // 1% Duty
      _BWLimit = 7;                  // 125 Khz Max
    }
    else if (Freq >= (868.7e6 + 62.5e3) && Freq <= (869.2e6 - 62.5e3)) //Band 50
    {
      _TXPowerLimit = 14;            // 25mW
      _DutyCycleMsHour = 3600;       // 0.1% Duty
      _BWLimit = 7;                  // 125 Khz Max
    }
    else if (Freq >= (869.4e6 + 62.5e3) && Freq <= (869.65e6 - 62.5e3)) //Band 54
    {
      _TXPowerLimit = 20;            // 100mW
      _DutyCycleMsHour = 360000;     // 10% Duty
      _BWLimit = 7;                  // 125 Khz Max
    }
    else if (Freq >= (869.7e6 + 62.5e3) && Freq <= (870.e6 - 62.5e3)) //Band 56b
    {
      _TXPowerLimit = 20;            // 100mW
      _DutyCycleMsHour = 36000;      // 1% Duty
      _BWLimit = 7;                  // 125 Khz Max
    }
    else // Outside Band, Disallow TXing 
    {
      DEBUG ("Frequency Note: Not in permitted TX Band");
      _TXPowerLimit = -99;
      _DutyCycleMsHour = 0;
      _BWLimit = 0;
    }
  }     
  /*  Set Low frequency mode according to datasheet */
  Frf (round(Freq / 61.035));
   if (Freq < 525000000)
  {
    LowFrequencyModeOn(1);
  }
  if (Freq > 779000000)
  {
    LowFrequencyModeOn(0);
  }
  return 0; 
}


/*  PowerDBm 
 *   
 *  Set power in dBm to an integer level:.
 *  Valid range between -3 and 14 if PA_Select off
 *  Valid range Bbtween 2 and 17 if PA_Select on
 *  ToDo: 20dBm not implemented.
 *  Returns: Preious power setting in dBm
 */
int8_t SX1276::
PowerDBm (int8_t NewPower) // [Optional] New Powerlevel in dBm
{
  float Power;
  float Maxpwr;

  if (PaSelect()==1)
  {
    Power = 17 - (15 - OutputPower());
  }
  else
  {
    Maxpwr = (10.8 + (0.6 * MaxPower()));
    Power = Maxpwr - (15 - OutputPower());
  }
  if (NewPower == -99) //unset value
  {
    return round(Power);
  }
  if (PaDac() != 4) //Ensure +20dBm mode is disabled
  {
    PaDac(4);
  }
  if (PaSelect()==1)
  {
    if (NewPower > 17)  NewPower = 17;
    if (NewPower < 2)  NewPower = 2;
    OutputPower (NewPower+15-17);
  }
  else
  {
    if (NewPower > 14)  NewPower = 14;
    if (NewPower < -3)  NewPower = -3;
    if (NewPower < 0)
    { 
      MaxPower(2);
      OutputPower (NewPower+3);
    } 
    else
    {
      MaxPower(7);
      OutputPower (NewPower);
    } 
  }
  return round(Power); 
}

/*  BwHz (Bandwidth in Hz)
 *   Get or set Lora Bandwidth in Hz.
 *  Valid inputs are: 7800   ret = 7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000
 *  Invalid input returns -1. No changes are made.
 *  Valid input of no value returns previously set bandwidth
 */

int32_t SX1276::
BwHz (int32_t BandWidth) //[Optional] New Bandwidth in Hz
{
  int32_t ret;

  /* Convert Current Bandwidth to Hz  */
  switch (Bw())
  {
    case 0:
      ret = 7800; 
      break;
    case 1:
      ret = 10400; 
      break;
    case 2:
      ret = 15600; 
      break;
    case 3:
      ret = 20800;
      break;
    case 4:
      ret = 31250;
      break;
    case 5:
      ret = 41700;
      break;
    case 6:
      ret = 62500;
      break;
    case 7:
      ret = 125000;
      break;
    case 8:
      ret = 250000;
      break;
    case 9:
      ret = 500000;
      break;
    default: 
      ret = -1;
  }

  /* Convert Requested Bandwidth from Hz  */
  switch (BandWidth)
  {
    case 0: // Default: Just return current bandwidth
      return ret;
    case 7800:
      BandWidth=0; 
      break;
    case 10400:
      BandWidth=1; 
      break;
    case 15600:
      BandWidth=2; 
      break;
    case 208003:
      BandWidth=3; 
      break;
    case 31250:
      BandWidth=4; 
      break;
    case 41700:
      BandWidth=5; 
      break;
    case 62500:
      BandWidth=6; 
      break;
    case 125000:
      BandWidth=7; 
      break;
    case 250000:
      BandWidth=8; 
      break;
    case 500000:
      BandWidth=9; 
      break;
    default:
      DEBUG ("BW Error: Invalid Bandwidth");
      return -1;
  }
 // if (BandWidth > _BWLimit) BandWidth = _BWLimit; // Move to TX routines
  
  /* Set Bandwidth  */
  Bw(BandWidth);
  
  /* Per errata note. (Spurious Reception) */
  if (BandWidth == 0)
  {
    AutomaticIFOn(0); 
    IfFreq2(0x48);    
    IfFreq1(0x00);    
  }
  else if (BandWidth < 6) 
  {
    AutomaticIFOn(0); 
    IfFreq2(0x44);    
    IfFreq1(0x00);    
  }
  else if (BandWidth < 9) 
  {
    AutomaticIFOn(0); 
    IfFreq2(0x40);    
    IfFreq1(0x00);    
  }
  else 
  {
    AutomaticIFOn(1);    
  }
 return ret;
}

/*  ClearFlags
 *   
 *  Clears all IRQ Flags
 */
void SX1276::ClearFlags()
{ spi_tx(RegIrqFlags,0xFF); }

/*  Reset
 *   
 *  Hardware Reset Modem
 *  Returns 0 on success
 *  Returns -1 on failure
 */
int SX1276::Reset()
{
  pinMode (_ResetPin, OUTPUT); 
  digitalWrite (_ResetPin, 0); 
  delay (10);
  pinMode (_ResetPin, INPUT); // Set pin to Hi-Z    
  delay (10);
  if (Mode() != SX1276_MODE_STDBY)
  {
    DEBUG ("Reset Error: Modem reset failure");
    return -1;
  }
  return 0;
}

/* Direct Parameter Read/Write Functions
 *  
 *  See SX1276 datasheet for more info on a particular parameter.
 *  All paramters specific to LoRa mode are provided.
 *  These functions read/write to the SPI registers directly.
 *  Use these when there is no alternate function above, or you need direct access.
 *  To read a parameter, don't supply a parameter.
 *  To write a parameter, supply a single paramter to write.
 *  When writing, the previously stored value is returned. Handy.
 *  Bit shifting and masking is handled, as are parameters that span multiple registers.
 *  trying to write out of range values will cause the value to overflow, 
 *  but will not affect adjactent parameters in a register.
 *  Note that some parameters are read only, and some are write only.
 */

uint8_t SX1276::Fifo()
{ return spi_rx(RegFifo); }
uint8_t SX1276::Fifo(uint8_t x)
{ return spi_tx(RegFifo,x); }

uint8_t SX1276::LongRangeMode() 
{ return spi_rx(RegOpMode,1,7); }
uint8_t SX1276::LongRangeMode(uint8_t x)
{ return spi_tx(RegOpMode,x,1,7); }


uint8_t SX1276::AccessSharedReg()
{ return spi_rx(RegOpMode,1,6); }
uint8_t SX1276::AccessSharedReg(uint8_t x)
{ return spi_tx(RegOpMode,x,1,6); }

uint8_t SX1276::LowFrequencyModeOn()
{ return spi_rx(RegOpMode,1,3); }
uint8_t SX1276::LowFrequencyModeOn(uint8_t x)
{ return spi_tx(RegOpMode,x,1,3); }

uint8_t SX1276::Mode()
{ return spi_rx(RegOpMode,3,0); }
uint8_t SX1276::Mode(uint8_t x)
{ return spi_tx(RegOpMode,x,3,0); }

uint32_t SX1276::Frf()
{ return spi_rx(RegFrMsb) * 0x10000 + spi_rx(RegFrMid) * 0x100 + spi_rx(RegFrLsb); }
uint32_t SX1276::Frf(uint32_t x)
{ return spi_tx(RegFrMsb,(x >> 16) & 0xFF) * 0x10000 + spi_tx(RegFrMid,(x >> 8) & 0xFF) * 0x100 + spi_tx(RegFrLsb,(x) & 0xFF); }


uint8_t SX1276::PaSelect()
{ return spi_rx(RegPaConfig,1,7); }
uint8_t SX1276::PaSelect(uint8_t x)
{ return spi_tx(RegPaConfig,x,1,7); }

uint8_t SX1276::MaxPower()
{ return spi_rx(RegPaConfig,3,4); }
uint8_t SX1276::MaxPower(uint8_t x)
{ return spi_tx(RegPaConfig,x,3,4); }


uint8_t SX1276::OutputPower()
{ return spi_rx(RegPaConfig,4,0); }
uint8_t SX1276::OutputPower(uint8_t x)
{ return spi_tx(RegPaConfig,x,4,0); }

uint8_t SX1276::PaRamp()
{ return spi_rx(RegPaRamp,4,0); }
uint8_t SX1276::PaRamp(uint8_t x)
{ return spi_tx(RegPaRamp,x,4,0); }

uint8_t SX1276::OcpOn()
{ return spi_rx(RegOcp,1,5); }
uint8_t SX1276::OcpOn(uint8_t x)
{ return spi_tx(RegOcp,x,1,5); }

uint8_t SX1276::OcpTrim()
{ return spi_rx(RegOcp,5,0); }
uint8_t SX1276::OcpTrim(uint8_t x)
{ return spi_tx(RegOcp,x,5,0); }

uint8_t SX1276::LnaGain()
{ return spi_rx(RegLna,3,5); }
uint8_t SX1276::LnaGain(uint8_t x)
{ return spi_tx(RegLna,x,3,5); }

uint8_t SX1276::LnaBoostLf()
{ return spi_rx(RegLna,2,3); }
uint8_t SX1276::LnaBoostLf(uint8_t x)
{ return spi_tx(RegLna,x,2,3); }

uint8_t SX1276::LnaBoostHf()
{ return spi_rx(RegLna,2,0); }
uint8_t SX1276::LnaBoostHf(uint8_t x)
{ return spi_tx(RegLna,x,2,0); }

uint8_t SX1276::FifoAddrPtr()
{ return spi_rx(RegFifoAddrPtr); }
uint8_t SX1276::FifoAddrPtr(uint8_t x)
{ return spi_tx(RegFifoAddrPtr,x); }

uint8_t SX1276::FifoTxBaseAddr()
{ return spi_rx(RegFifoTxBaseAddr); }
uint8_t SX1276::FifoTxBaseAddr(uint8_t x)
{ return spi_tx(RegFifoTxBaseAddr,x); }

uint8_t SX1276::FifoRxBaseAddr()
{ return spi_rx(RegFifoRxBaseAddr); }
uint8_t SX1276::FifoRxBaseAddr(uint8_t x)
{ return spi_tx(RegFifoRxBaseAddr,x); }

uint8_t SX1276::FifoRxCurrentAddr()
{ return spi_rx(RegFifoRxCurrentAddr); }

uint8_t SX1276::RxTimeoutMask()
{ return spi_rx(RegIrqFlagsMask,1,7); }
uint8_t SX1276::RxTimeoutMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,7); }

uint8_t SX1276::RxDoneMask()
{ return spi_rx(RegIrqFlagsMask,1,6); }
uint8_t SX1276::RxDoneMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,6); }

uint8_t SX1276::PayloadCrcErrorMask()
{ return spi_rx(RegIrqFlagsMask,1,5); }
uint8_t SX1276::PayloadCrcErrorMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,5); }

uint8_t SX1276::ValidHeaderMask()
{ return spi_rx(RegIrqFlagsMask,1,4); }
uint8_t SX1276::ValidHeaderMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,4); }

uint8_t SX1276::TxDoneMask()
{ return spi_rx(RegIrqFlagsMask,1,3); }
uint8_t SX1276::TxDoneMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,3); }

uint8_t SX1276::CadDoneMask()
{ return spi_rx(RegIrqFlagsMask,1,2); }
uint8_t SX1276::CadDoneMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,20); }

uint8_t SX1276::FhssChangeChannelMask()
{ return spi_rx(RegIrqFlagsMask,1,1); }
uint8_t SX1276::FhssChangeChannelMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,1); }

uint8_t SX1276::CadDetectedMask()
{ return spi_rx(RegIrqFlagsMask,1,0); }
uint8_t SX1276::CadDetectedMask(uint8_t x)
{ return spi_tx(RegIrqFlagsMask,x,1,0); }

uint8_t SX1276::RxTimeout()
{ return spi_rx(RegIrqFlags,1,7); }
uint8_t SX1276::RxTimeout(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,7); }

uint8_t SX1276::RxDone()
{ return spi_rx(RegIrqFlags,1,6); }
uint8_t SX1276::RxDone(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,6); }

uint8_t SX1276::PayloadCrcError()
{ return spi_rx(RegIrqFlags,1,5); }
uint8_t SX1276::PayloadCrcError(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,5); }

uint8_t SX1276::ValidHeader()
{ return spi_rx(RegIrqFlags,1,4); }
uint8_t SX1276::ValidHeader(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,4); }

uint8_t SX1276::TxDone()
{ return spi_rx(RegIrqFlags,1,3); }
uint8_t SX1276::TxDone(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,3); }

uint8_t SX1276::CadDone()
{ return spi_rx(RegIrqFlags,1,2); }
uint8_t SX1276::CadDone(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,20); }

uint8_t SX1276::FhssChangeChannel()
{ return spi_rx(RegIrqFlags,1,1); }
uint8_t SX1276::FhssChangeChannel(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,1); }

uint8_t SX1276::CadDetected()
{ return spi_rx(RegIrqFlags,1,0); }
uint8_t SX1276::CadDetected(uint8_t x)
{ return spi_tx(RegIrqFlags,x,1,0); }

uint8_t SX1276::FifoRxBytesNb()
{ return spi_rx(RegRxNbBytes); }

uint16_t SX1276::ValidHeaderCnt()
{ return spi_rx(RegRxHeaderCntValueMsb) * 0x100 + spi_rx(RegRxHeaderCntValueLsb); }

uint16_t SX1276::ValidPacketCnt()
{ return spi_rx(RegRxPacketCntValueMsb) * 0x100 + spi_rx(RegRxPacketCntValueLsb); }

uint8_t SX1276::RxCodingRate()
{ return spi_rx(RegModemStat,3,5); }

uint8_t SX1276::ModemStatus()
{ return spi_rx(RegModemStat,5,0); }

uint8_t SX1276::PacketSnr()
{ return spi_rx(RegPktSnrValue); }

uint8_t SX1276::PacketRssi()
{ return spi_rx(RegPktRssiValue); }

uint8_t SX1276::Rssi()
{ return spi_rx(RegRssiValue); }

uint8_t SX1276::PllTimeout()
{ return spi_rx(RegHopChannel,1,7); }

uint8_t SX1276::CrcOnPayload()
{ return spi_rx(RegHopChannel,1,6); }

uint8_t SX1276::FhssPresentChannel()
{ return spi_rx(RegHopChannel,6,0); }

uint8_t SX1276::Bw()
{ return spi_rx(RegModemConfig1,4,4); }
uint8_t SX1276::Bw(uint8_t x)
{ return spi_tx(RegModemConfig1,x,4,4); }

uint8_t SX1276::CodingRate()
{ return spi_rx(RegModemConfig1,3,1); }
uint8_t SX1276::CodingRate(uint8_t x)
{ return spi_tx(RegModemConfig1,x,3,1); }

uint8_t SX1276::ImplicitHeaderModeOn()
{ return spi_rx(RegModemConfig1,1,0); }
uint8_t SX1276::ImplicitHeaderModeOn(uint8_t x)
{ return spi_tx(RegModemConfig1,x,1,0); }

uint8_t SX1276::SpreadingFactor()
{ return spi_rx(RegModemConfig2,4,4); }
uint8_t SX1276::SpreadingFactor(uint8_t x)
{ return spi_tx(RegModemConfig2,x,4,4); }

uint8_t SX1276::TxContinuousMode()
{ return spi_rx(RegModemConfig2,1,3); }
uint8_t SX1276::TxContinuousMode(uint8_t x)
{ return spi_tx(RegModemConfig2,x,1,3); }

uint8_t SX1276::RxPayloadCrcOn()
{ return spi_rx(RegModemConfig2,1,2); }
uint8_t SX1276::RxPayloadCrcOn(uint8_t x)
{ return spi_tx(RegModemConfig2,x,1,2); }

uint16_t SX1276::SymbTimeout()
{ return spi_rx(RegModemConfig2,2,0) * 0x100 + spi_rx(RegSymbTimeoutLsb); }
uint16_t SX1276::SymbTimeout(uint16_t x)
{ return spi_tx(RegModemConfig2, (x >> 8) & 0xFF, 2,0) * 0x100 + spi_tx(RegSymbTimeoutLsb, x & 0xFF); }

uint16_t SX1276::PreambleLength()
{ return spi_rx(RegPreambleMsb) * 0x100 + spi_rx(RegPreambleLsb); }
uint16_t SX1276::PreambleLength(uint16_t x)
{ return spi_tx(RegPreambleMsb, (x >> 8) & 0xFF) * 0x100 + spi_tx(RegPreambleLsb, x & 0xFF); }

uint8_t SX1276::PayloadLength()
{ return spi_rx(RegPayloadLength); }
uint8_t SX1276::PayloadLength(uint8_t x)
{ return spi_tx(RegPayloadLength,x); }

uint8_t SX1276::PayloadMaxLength()
{ return spi_rx(RegMaxPayloadLength); }
uint8_t SX1276::PayloadMaxLength(uint8_t x)
{ return spi_tx(RegMaxPayloadLength,x); }

uint8_t SX1276::FreqHoppingPeriod()
{ return spi_rx(RegHopPeriod); }
uint8_t SX1276::FreqHoppingPeriod(uint8_t x)
{ return spi_tx(RegHopPeriod,x); }

uint8_t SX1276::FifoRxByteAddrPtr()
{ return spi_rx(RegFifoRxByteAddr); }

uint8_t SX1276::LowDataRateOptimize()
{ return spi_rx(RegModemConfig3,1,3); }
uint8_t SX1276::LowDataRateOptimize(uint8_t x)
{ return spi_tx(RegModemConfig3,x,1,3); }

uint8_t SX1276::AgcAutoOn()
{ return spi_rx(RegModemConfig3,1,2); }
uint8_t SX1276::AgcAutoOn(uint8_t x)
{ return spi_tx(RegModemConfig3,x,1,2); }

uint8_t SX1276::PpmCorrection()
{ return spi_rx(RegPpmCorrection); }
uint8_t SX1276::PpmCorrection(uint8_t x)
{ return spi_tx(RegPpmCorrection,x); }

uint32_t SX1276::FreqError()
{ return spi_rx(RegFeiMsb,4,0) * 0x10000 + spi_rx(RegFeiMid) * 0x100 + spi_rx(RegFeiLsb); }

uint8_t SX1276::RssiWideband()
{ return spi_rx(RegRssiWideband); }

uint8_t SX1276::IfFreq2()
{ return spi_rx(RegIfFreq2); }
uint8_t SX1276::IfFreq2(uint8_t x)
{ return spi_tx(RegIfFreq2,x); }

uint8_t SX1276::IfFreq1()
{ return spi_rx(RegIfFreq1); }
uint8_t SX1276::IfFreq1(uint8_t x)
{ return spi_tx(RegIfFreq1,x); }

uint8_t SX1276::AutomaticIFOn()
{ return spi_rx(RegDetectOptimize,1,7); }
uint8_t SX1276::AutomaticIFOn(uint8_t x)
{ return spi_tx(RegDetectOptimize,x,1,7); }

uint8_t SX1276::DetectionOptimize()
{ return spi_rx(RegDetectOptimize,3,0); }
uint8_t SX1276::DetectionOptimize(uint8_t x)
{ return spi_tx(RegDetectOptimize,x,3,0); }

uint8_t SX1276::InvertIQ_RX()
{ return spi_rx(RegInvertIQ,1,6); }
uint8_t SX1276::InvertIQ_RX(uint8_t x)
{ return spi_tx(RegInvertIQ,x,1,6); }

uint8_t SX1276::InvertIQ_TX()
{ return spi_rx(RegInvertIQ,1,0); }
uint8_t SX1276::InvertIQ_TX(uint8_t x)
{ return spi_tx(RegInvertIQ,x,1,0); }

uint8_t SX1276::HighBWOptimize1()
{ return spi_rx(RegHighBWOptimize1); }
uint8_t SX1276::HighBWOptimize1(uint8_t x)
{ return spi_tx(RegHighBWOptimize1,x); }

uint8_t SX1276::DetectionThreshold()
{ return spi_rx(RegDetectionThreshold); }
uint8_t SX1276::DetectionThreshold(uint8_t x)
{ return spi_tx(RegDetectionThreshold,x); }

uint8_t SX1276::SyncWord()
{ return spi_rx(RegSyncWord); }
uint8_t SX1276::SyncWord(uint8_t x)
{ return spi_tx(RegSyncWord,x); }

uint8_t SX1276::HighBWOptimize2()
{ return spi_rx(RegHighBWOptimize2); }
uint8_t SX1276::HighBWOptimize2(uint8_t x)
{ return spi_tx(RegHighBWOptimize2,x); }

uint8_t SX1276::InvertIQ2()
{ return spi_rx(RegInvertIQ2); }
uint8_t SX1276::InvertIQ2(uint8_t x)
{ return spi_tx(RegInvertIQ2,x); }

uint8_t SX1276::Dio0Mapping()
{ return spi_rx(RegDioMapping1,2,6); }
uint8_t SX1276::Dio0Mapping(uint8_t x)
{ return spi_tx(RegDioMapping1,x,2,6); }

uint8_t SX1276::Dio1Mapping()
{ return spi_rx(RegDioMapping1,2,4); }
uint8_t SX1276::Dio1Mapping(uint8_t x)
{ return spi_tx(RegDioMapping1,x,2,4); }

uint8_t SX1276::Dio2Mapping()
{ return spi_rx(RegDioMapping1,2,2); }
uint8_t SX1276::Dio2Mapping(uint8_t x)
{ return spi_tx(RegDioMapping1,x,2,2); }

uint8_t SX1276::Dio3Mapping()
{ return spi_rx(RegDioMapping1,2,0); }
uint8_t SX1276::Dio3Mapping(uint8_t x)
{ return spi_tx(RegDioMapping1,x,2,0); }

uint8_t SX1276::Dio4Mapping()
{ return spi_rx(RegDioMapping2,2,6); }
uint8_t SX1276::Dio4Mapping(uint8_t x)
{ return spi_tx(RegDioMapping2,x,2,6); }

uint8_t SX1276::Dio5Mapping()
{ return spi_rx(RegDioMapping2,2,4); }
uint8_t SX1276::Dio5Mapping(uint8_t x)
{ return spi_tx(RegDioMapping2,x,2,4); }

uint8_t SX1276::Version()
{ return spi_rx(RegVersion); }

uint8_t SX1276::PaDac()
{ return spi_rx(RegPaDAC,3,0); }
uint8_t SX1276::PaDac(uint8_t x)
{ return spi_tx(RegPaDAC,x,3,0); }

uint8_t SX1276::FormerTemp()
{ return spi_rx(RegFormerTemp); }

uint8_t SX1276::AgcReferenceLevel()
{ return spi_rx(RegAgcRef,6,0); }
uint8_t SX1276::AgcReferenceLevel(uint8_t x)
{ return spi_tx(RegAgcRef,x,6,0); }

uint8_t SX1276::AgcStep1()
{ return spi_rx(RegAgcThresh1,4,0); }
uint8_t SX1276::AgcStep1(uint8_t x)
{ return spi_tx(RegAgcThresh1,x,4,0); }

uint8_t SX1276::AgcStep2()
{ return spi_rx(RegAgcThresh2,4,4); }
uint8_t SX1276::AgcStep2(uint8_t x)
{ return spi_tx(RegAgcThresh2,x,4,4); }

uint8_t SX1276::AgcStep3()
{ return spi_rx(RegAgcThresh2,4,0); }
uint8_t SX1276::AgcStep3(uint8_t x)
{ return spi_tx(RegAgcThresh2,x,4,0); }

uint8_t SX1276::AgcStep4()
{ return spi_rx(RegAgcThresh3,4,4); }
uint8_t SX1276::AgcStep4(uint8_t x)
{ return spi_tx(RegAgcThresh3,x,4,4); }

uint8_t SX1276::AgcStep5()
{ return spi_rx(RegAgcThresh3,4,0); }
uint8_t SX1276::AgcStep5(uint8_t x)
{ return spi_tx(RegAgcThresh3,x,4,0); }

uint8_t SX1276::PllBandwidth()
{ return spi_rx(RegPll,4,0); }
uint8_t SX1276::PllBandwidth(uint8_t x)
{ return spi_tx(RegPll,x,4,0); }


/*  SPI Read and Write routines  */

uint8_t SX1276::spi_rx(uint8_t addr,uint8_t bits,uint8_t bitshift) {
  uint8_t spi_read;
  #ifdef ESP32  
    spi->beginTransaction(SPISettings(_spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(_NSS_pin, LOW);
    spi->transfer(addr);
    spi_read = spi->transfer(0);
    digitalWrite(_NSS_pin, HIGH);
    spi->endTransaction();
  #else
    uint8_t spi_array[2] = {0, 0};
    spi_array[0] = addr;
    digitalWrite(_NSS_pin, LOW);
    wiringPiSPIDataRW (0, spi_array,2);
    spi_read = spi_array[1];
    digitalWrite(_NSS_pin, HIGH);
  #endif
  if (bits!=8) {
    spi_read >>= bitshift;
    spi_read &= (0xFF >> (8 - bits));
  }
  return spi_read;
}
// Write Byte or bits to SPI Register. Returns previous byte/bits
uint8_t SX1276::spi_tx(uint8_t addr, uint8_t spi_data,uint8_t bits,uint8_t bitshift) {
  uint8_t spi_read;
  uint8_t bitmask;

  if (bits!=8) {
    spi_read = spi_rx(addr);
    bitmask = (0xFF >> (8 - bits));
    spi_data &= bitmask;
    bitmask <<= bitshift; 
    spi_data <<= bitshift;     
    spi_data += spi_read & ~bitmask;  
    spi_read >>= bitshift;
    spi_read &= (0xFF >> (8 - bits));
  }
  #ifdef ESP32   
    spi->beginTransaction(SPISettings(_spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(_NSS_pin, LOW);
    spi->transfer((addr | 0x80));
    spi->transfer(spi_data);
    digitalWrite(_NSS_pin, HIGH);
    spi->endTransaction();
  #else
    uint8_t spi_array[2] = {0, 0};
    spi_array[0] = (addr | 0x80);
    spi_array[1] = spi_data;
    digitalWrite(_NSS_pin, LOW);
    wiringPiSPIDataRW (0, spi_array,2);
    spi_read = spi_array[1];
    digitalWrite(_NSS_pin, HIGH);
  #endif
  return spi_read;
}
