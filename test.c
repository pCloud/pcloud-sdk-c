#include "pSDK.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
  char *err = NULL;
  psdk_init(1, &err);
  
  char * authurl = psdk_authorize("lCmd5dtXNYX","1029", "LKAqbD8y39L93Y3WtdbRHy2mLQ8y");
  printf ("Paste this in browser [%s]\n", authurl );
  
  // psdk_set_user_pass("ivan.stoev@pcloud.com", "MontEchRis10", 1, 101);
  
  psdk_wait_authorized();

  char * res = psdk_list_folder("/", &err);
  
  printf ("List root: \n%s\n", res);
  
  free (authurl);
  free(res);
  free(err);
  
}