#define DEBUG_BUILD
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
  if (lora->Init(OUTPUT_PA_BOOST,BANDPLAN_EU868)<0)
    printf("Init Error\n");
  lora->SpreadingFactor(6);
  if (lora->PowerDBm(-5)<0)
    printf("Error power\n");
  if (lora->Frequency(840e6)<0)
    printf("Error setting freq\n");
  int z=0;
  while (strcmp(send,".")!=0)
  {
    scanf("%s",&send);
    printf ("TXTIME: %d\n",lora->TX(send,strlen(send)));
    printf("Time %d\n", lora->TxTimer(z));
  }


//  printf("TX..\n");
  delay (50);
//  printf ("RX return: %d\n",lora->RXContinuous(rcv,sizeof(rcv)));
//  printf ("relay data: \n");
//  printf (rcv);
  return 0;
}

