
#pragma once
#ifndef _PSDK_H
#define _PSDK_H

#ifdef __cplusplus
extern "C" {
#endif

#define SDKP_STR(name, val) {PARAM_STR, strlen(name), strlen(val), (name), {(uint64_t)((uintptr_t)(val))}}
#define SDKP_NUM(name, val) {PARAM_NUM, strlen(name), 0, (name), {(val)}}
#define SDKP_BOOL(name, val) {PARAM_BOOL, strlen(name), 0, (name), {(val)?1:0}}
  
#include <stdint.h>
  
  typedef void (/*_cdecl*/ *CompletitionCallback)(uint32_t progress, int err, const char * msg);
  
  int psdk_init(char **err);

  int psdk_set_user_pass(const char * user, const char * pass, int save_auth, uint64_t ref);
  
  int psdk_upload_file(const char* localpath, const char *filename, const char * remotepath, CompletitionCallback callback);
  
  int psdk_download_file(const char* localpath, const char *filename, const char * remotepath, CompletitionCallback callback);
  
  // Checks and creates new folder with write permissions on it and adds suffix to the name if necessary i.e. New Folder (1) etc..
  int psdk_check_create_folder(const char * path);
  
  // Creates remote folder no checks no suffix
  int psdk_create_folder_by_path(const char *path, char **err);
  
  
  int psdk_send_api_command(const char *command, char **err, const char * fmt, ...);
  
  void psdk_stop();

#ifdef __cplusplus
}
#endif
  
#endif //_PSDK_H 