#define DEBUG_BUILD
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include "SX1276.cpp"
int main ()
{
  //  std::cout << "Return: " << ret << std::endl;
  FILE *  logfile;
  int counter,rcvlen,repeat,sync,freq;
  //  printf ("chars (hex): %x   %x\n",s[0],s[1]);
  SX1276 * lora = NULL;
  lora = new SX1276(1000000,6,0);
//  char send [30] = "Test cpp\0";
  char rcv [255] = {0};
  lora->Init(1,1);
  lora->PowerDBm(2);
  sync=0x34;
  freq=864e6;
  lora->ImplicitHeaderModeOn(0);
  while (1)
  {
    lora->Frequency(freq);
    lora->SyncWord(sync); //LORAWAN syncword is 0x34, default is 0x12
    printf ("Syncword set to 0x%x\n",sync);
    printf ("Freq set to %d %d\n",freq,lora->Frequency());
    for (char spread = 12;spread<=12;spread++)
    {
      lora->SpreadingFactor(spread);
      printf ("RXing at spread %d:\n",spread);
      rcvlen = lora->RXContinuous(rcv,sizeof(rcv),2050);
      if (rcvlen > 0)
      {
        printf ("saving to log\n");
        logfile = fopen ("loralog.txt","a");
        fprintf (logfile, "\n\nData Rcvd (SF:%d, Sync:0x%x, Freq:%d)\n",spread,sync,freq);
        for (counter=0;counter<rcvlen;counter++)
          fprintf (logfile, "%c", rcv[counter] );
        fclose (logfile);
      }
   }
   sync+=0x04;
   if (sync>0xfd)
     sync=0x00;
   fflush(stdout);
}

//  printf("TX..\n");
  delay (50);
//  printf ("RX return: %d\n",lora->RXContinuous(rcv,sizeof(rcv)));
//  printf ("relay data: \n");
//  printf (rcv);
  return 0;
}

