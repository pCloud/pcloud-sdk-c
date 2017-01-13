#include "pSDK.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
  char *err = NULL;
  psdk_init(1, &err);
  
  char * authurl = psdk_authorize("xfxcvg","1034", "dxvxcvxcvzvxczvczxvdfsbdgfnghfmhgfm", 1);
  printf ("Paste this in browser [%s]\n", authurl );
  
  //psdk_wait_authorized();

  char * res = psdk_list_folder("/", &err);
  
  printf ("List root: \n%s\n", res);
  
  free (authurl);
  free(res);
  free(err);
  
}