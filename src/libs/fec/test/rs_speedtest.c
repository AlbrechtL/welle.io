#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "fec.h"

int main(){
  unsigned char block[255];
  int i;
  void *rs;
  struct rusage start,finish;
  double extime;
  int trials = 10000;

  for(i=0;i<223;i++)
    block[i] = 0x01;

  rs = init_rs_char(8,0x187,112,11,32,0);
  encode_rs_char(rs,block,&block[223]);

  getrusage(RUSAGE_SELF,&start);
  for(i=0;i<trials;i++){
#if 0
    block[0] ^= 0xff; /* Introduce an error */
    block[2] ^= 0xff; /* Introduce an error */
#endif
    decode_rs_char(rs,block,NULL,0);
  }
  getrusage(RUSAGE_SELF,&finish);
  extime = finish.ru_utime.tv_sec - start.ru_utime.tv_sec + 1e-6*(finish.ru_utime.tv_usec - start.ru_utime.tv_usec);

  printf("Execution time for %d Reed-Solomon blocks using general decoder: %.2f sec\n",trials,extime);
  printf("decoder speed: %g bits/s\n",trials*223*8/extime);

  exit(0);
}
