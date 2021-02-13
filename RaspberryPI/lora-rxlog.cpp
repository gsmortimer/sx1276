// Recieve data and print it to log?
//#define DEBUG_BUILD
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <iostream>
#include <string.h>
#include "SX1276.cpp"
int main ()
{
  SX1276 * lora = NULL;
  lora = new SX1276(1000000,6,0);
  char send [255] = "Test cpp\0";
  char rcv  [255] = {0};
  int rxlen;
  if (lora->Init(OUTPUT_PA_BOOST,BANDPLAN_EU868)<0)
    printf("Init Error\n");
  lora->SpreadingFactor(10);
  if (lora->PowerDBm(2)<0)
    printf("Error power\n");
  if (lora->Frequency(869.5e6)<0)
    printf("Error setting freq\n");
  lora->BwHz(125e3);
  lora->SyncWord(42);
  printf("Starting RX..\n");
  int z=0;
  int hrs, mins, secs;
  int lon_d, lon_m, lat_d, lat_m;
  while (true)
  {
    rxlen = lora->RXContinuous(rcv,255,21000);
    if (rxlen > 0)
    {
      rcv[rxlen]=0; //null teminate
      //scanf("%s",&send);
      //printf ("Data: %s\n",rcv);
      hrs  = (rcv[0]*256 + rcv[1]) / 1800;
      mins = ((rcv[0]*256 + rcv[1]) % 1800) / 30;
      secs = (rcv[0]*256 + rcv[1]) % 30 * 2;
      lat_d = (rcv[2]*65536+rcv[3]*256+rcv[4]) / 60000;
      if (lat_d >= 90) lat_d -= 90;
      lat_m = (rcv[2]*65536+rcv[3]*256+rcv[4]) % 60000;
      lon_d = (rcv[5]*65536+rcv[6]*256+rcv[7]) / 60000;
      if (lon_d >= 180) lon_d -= 180;
      lon_m = (rcv[5]*65536+rcv[6]*256+rcv[7]) % 60000;
      printf ("Time: %d:%d:%d. Lat: %dd%d Lon: %dd%d\n",hrs,mins,secs,lat_d,lat_m,lon_d,lon_m);
      delay(1800);
    }   
  }


//  printf("TX..\n");
  delay (50);
//  printf ("RX return: %d\n",lora->RXContinuous(rcv,sizeof(rcv)));
//  printf ("relay data: \n");
//  printf (rcv);
  return 0;
}

