 
#include "pSDK.h"

#include "papi.h"
#include "pmobile.h"
#include "pnetlibs.h"
#include "plibs.h"

#include <stdio.h>
#include <string.h>

static pmobile_t mlib_ = NULL;
static pthread_mutex_t auth_lock_;
static int logged_in_ = 0;
static int save_auth_ = 0;
static char * auth_ =  NULL;
static char filename_[] = "aurhrem.txt";
static char apiserver[64] = "binapi.pcloud.com";

typedef uint64_t pmobile_folderid_t;
typedef uint64_t pmobile_fileid_t;

#define PSYNC_INVALID_FOLDERID ((pmobile_folderid_t)-1)
#define PSYNC_INVALID_PATH NULL


pmobile_folderid_t get_folderid_by_path(const char * path,int create);
pmobile_fileid_t get_fileid_by_path(const char * remotepath,const char * filename, uint64_t *hash);

static uint64_t process_api_result(binresult* res) {
  uint64_t result;
  result=pmobile_find_result(res, "result", PARAM_NUM)->num;
  if (result){
    if (result == 2000)
    pthread_mutex_lock(&auth_lock_);
    logged_in_ = 0;
    if (auth_)
      pmobile_free(auth_);
    pthread_mutex_unlock(&auth_lock_);
    pmobile_free(res);
    return result;
  }
  return 1;
}

static uint64_t process_api_int_result(uint64_t result) {
  if (result){
    if (result == 2000)
    pthread_mutex_lock(&auth_lock_);
    logged_in_ = 0;
    if (auth_)
      pmobile_free(auth_);
    pthread_mutex_unlock(&auth_lock_);
    return result;
  }
  return 1;
}


static void init_param_str(binparam* t, const char *name, const char *val) {
  //{PARAM_STR, strlen(name), strlen(val), (name), {(uint64_t)((uintptr_t)(val))}}
  t->paramtype  = PARAM_STR;
  t->paramnamelen = strlen(name);
  t->opts = strlen(val);
  t->paramname = name;
  t->str = val;
}

static void init_param_num(binparam* t, const char *name, uint64_t val) {
  //{PARAM_NUM, strlen(name), 0, (name), {(val)}}
  t->paramtype  = PARAM_NUM;
  t->paramnamelen = strlen(name);
  t->opts = 0;
  t->paramname = name;
  t->num = val;
}

static void init_param_bool(binparam* t, const char *name, uint64_t val) {
  //{PARAM_NUM, strlen(name), 0, (name), {(val)}}
  t->paramtype  = PARAM_BOOL;
  t->paramnamelen = strlen(name);
  t->opts = 0;
  t->paramname = name;
  t->num = val?1:0;
}

static void api_set_server(const char *binapi) {
  size_t len;
  len=strlen(binapi)+1;
  if (len<=sizeof(apiserver)) {
    memcpy(apiserver, binapi, len);
    debug(D_NOTICE, "set %s as best api server", apiserver);
  }
}

static char * binresult_to_json(binresult* res)
{
  
}

int psdk_send_api_command(const char *command, char **result, int login, int numparam, const char * fmt, ...) 
{
  binparam *t;
  va_list ap;
  int j, p = 0;
  long pint = 0;
  char * pstr = NULL;
  char * next = NULL;
  int fmtlen = strlen(fmt);
  char * fmtind = fmt;
  binresult* res;

  if (login) {
    pthread_mutex_lock(&auth_lock_);
    if (logged_in_) {
      t = (binparam *) pmobile_malloc(mlib_, (numparam+1)*sizeof(binparam));
      init_param_str(t, "auth", auth_);
    }
    else { 
      pthread_mutex_unlock(&auth_lock_);
      return 19;
    }
    pthread_mutex_unlock(&auth_lock_);
  } else t = (binparam *) pmobile_malloc(mlib_, (numparam)*sizeof(binparam));
 
  va_start(ap, fmt); //Requires the last fixed parameter (to get the address)
    for(j=0; j<numparam; j++) {
      next = strchr(fmtind,'%');
      if (!next)
        break;
      char * name = strndup(fmtind, (next - fmtind -1));
      if (next[1] == 's') {
        pstr = va_arg(ap, char *);
        init_param_str(t, name,pstr);
      } if (next[1] == 'b') {
        pint = va_arg(ap, long);
        init_param_bool(t, name,pint);
      } else {
        pint = va_arg(ap, long);
        init_param_num(t, name,pint);
      }
      
    }
  va_end(ap);
  
  res = pmobile_api_run_command(mlib_, command, t, apiserver);
  pmobile_free(t);
  
  result = binresult_to_json(res);
  
  pmobile_free(res);
  return 0; 
}

int psdk_init(char **err)
{
  mlib_ =(void *) pmobile_init();
  if ((uint64_t)mlib_ == PMOBILE_ERROR_NO_MEMORY) {
    *err = strndup("No memory to init the pcloud lib", 32);
    return 1;
  }
  
  if((uint64_t)mlib_ == PMOBILE_ERROR_INIT_SSL) {
    *err = strndup("Pmobile failed to init ssl!", 27);
    return 2;
  }
  
  if (pthread_mutex_init(&auth_lock_, NULL) != 0) {
    *err = pmobile_strndup(mlib_, "Mutex init failed!", 18);
    return 3;
  }
  return 0;
}

static int read_auth_from_file() {
  FILE *file = fopen (filename_, "r" );
  if (file != NULL) {
    char line [1000];
    memset(line,0, 1000);
    while(fgets(line,sizeof line,file)!= NULL) /* read a line from a file */ {
      pthread_mutex_lock(&auth_lock_);
      if (auth_) 
        pmobile_free (auth_);
      auth_ = pmobile_strndup(mlib_ ,line, 1000);
      logged_in_ = 1;
      pthread_mutex_unlock(&auth_lock_);
      return 1;
    }
    fclose(file);
  }
  return 0;
}

static int write_auth_to_file(const char * auth) {
  FILE *file = fopen(filename_, "w");
  if (file != NULL) {
    pthread_mutex_lock(&auth_lock_);
    if (auth_) 
      pmobile_free (auth_);
    auth_ = pmobile_strndup(mlib_, auth, 1000);
    fprintf(file,"%s",auth);
    logged_in_ = 1;
    pthread_mutex_unlock(&auth_lock_);
    fclose(file);
  }
  return 0;
}

static void set_auth(const char * auth) {
    pthread_mutex_lock(&auth_lock_);
    if (auth_) 
      pmobile_free (auth_);
    auth_ = pmobile_strndup(mlib_, auth, 1000);
    pthread_mutex_unlock(&auth_lock_);
}

int psdk_set_user_pass(const char * user, const char * pass, int save_auth, uint64_t ref)
{
  binparam params[] = { P_STR("username", user), P_STR("password", pass), P_NUM("getauth", 1) , 
    P_NUM("logout", 1), P_NUM("getregistrationinfo", 1)};
  binresult *res;
  const binresult *cres;
 // debug(D_NOTICE,"Just a test.");

  res = pmobile_api_run_command(mlib_, "userinfo", params, apiserver);
  if (unlikely_log(!res))
    return PMOBILE_ERROR_NET_ERROR;
  uint64_t ret = process_api_result(res);
  if (!ret)
    debug(D_WARNING, "userinfo returned error %lu: %s", ret, pmobile_find_result_str(res, "error"));
  else {
    const char * auth = pmobile_find_result_str(res, "auth");
    if (auth) {
      save_auth_ = save_auth;
      if (save_auth_)
        write_auth_to_file(auth);
      else 
        set_auth(auth);
      
     /* uint64_t prem = pmobile_find_result(res, "premium", PARAM_BOOL)->num;
      uint64_t bus = pmobile_find_result(res, "business", PARAM_BOOL)->num;
      if (prem || bus) {
        const binresult *reginfo = pmobile_find_result(res,"registrationinfo", PARAM_HASH);
        const binresult *ref = pmobile_check_result(reginfo, "ref", PARAM_NUM);
        if (ref && (ref->num == ref)) {
          if (save_auth_)
            write_auth_to_file(auth);
          else 
            set_auth(auth);
        }
      }*/
      cres=pmobile_find_result(pmobile_find_result(res, "apiserver", PARAM_HASH), "binapi", PARAM_STR);
      if (cres->length)
        api_set_server(cres->array[0]->str);
    } else ret = 18;
  }
  
  pmobile_free(res);
  return (int)ret;
}

static  void progress_callback(void *hndl, uint32_t progress, uint64_t bytesdone, uint64_t bytestotal) 
{
  CompletitionCallback callback = (CompletitionCallback)hndl;
  callback(progress, 0, "No error");
}
static  int error_callback(void *hndl, int err)
{
  CompletitionCallback callback = (CompletitionCallback)hndl;
  if (err == 512)
    callback(0, 0, "All done.");
  else
    callback(0, err,"Error");
  return err;
}


int psdk_download_file(const char* localpath, const char *filename, const char * remotepath, CompletitionCallback callback)
{
  pthread_mutex_lock(&auth_lock_);
  if (!logged_in_) {
    pthread_mutex_unlock(&auth_lock_);
    return 19;
  }
  pthread_mutex_unlock(&auth_lock_);
  
  if (auth_ || read_auth_from_file()) {
    uint64_t hash;
    pmobile_fileid_t fileid = get_fileid_by_path(remotepath, filename, &hash);
    if (fileid != PSYNC_INVALID_FOLDERID) {
      char *lpt = pmobile_malloc(mlib_, strlen(localpath)+strlen(filename)+1);
      strcpy(lpt, localpath);
      strcat(lpt, filename);
    
      pmobile_file_download_t up = pmobile_file_download_create(mlib_, auth_, fileid, hash, 0, 1, lpt, apiserver,
        100, &progress_callback, &error_callback, (void *)callback);
      pmobile_free(lpt);
      
      if (pmobile_is_error(up))
        callback(0, process_api_int_result(pmobile_to_error(up)), "Failed to start the download.");
      else 
        callback(0, 0, "Download started.");
    } else return 1;
  }
  return 0;
}

int psdk_upload_file(const char* localpath, const char *filename, const char * directory_name, CompletitionCallback callback)
{
  pthread_mutex_lock(&auth_lock_);
  if (!logged_in_) {
    pthread_mutex_unlock(&auth_lock_);
    return 19;
  }
  pthread_mutex_unlock(&auth_lock_);
  
  if (auth_ || read_auth_from_file()) {

    uint64_t folderid = get_folderid_by_path(directory_name, 1);
    if (folderid != PSYNC_INVALID_FOLDERID) {
      char *lpt = pmobile_malloc(mlib_, strlen(localpath)+strlen(filename)+1);
      strcpy(lpt, localpath);
      strcat(lpt, filename);
    
      pmobile_file_upload_t up = pmobile_file_upload_create(mlib_, auth_, folderid, 0, lpt, filename, apiserver,
        100, &progress_callback, &error_callback, (void *)callback);
      pmobile_free(lpt);
      
      if (pmobile_is_error(up))
        callback(0, process_api_int_result(pmobile_to_error(up)), "Failed to start the upload.");
      else 
        callback(0, 0, "Upload started.");
    } else return 1;
  }
  return 0;
}


void psdk_stop()
{
  pthread_mutex_lock(&auth_lock_);
  logged_in_ = 0;
  if (auth_)
    pmobile_free(auth_);
  pthread_mutex_unlock(&auth_lock_);
  //pmobile_destroy((pmobile_t)(lib_ptr_));
  pthread_mutex_destroy(&auth_lock_);
}

static uint64_t extract_folderid(binresult * res) {
  const binresult* meta = pmobile_find_result(res, "metadata", PARAM_HASH);
  return pmobile_find_result_num(meta, "folderid");
}

pmobile_fileid_t extract_fileid(binresult *res,const char* filename, uint64_t *hash){
  int i;
  const binresult* contents = pmobile_find_result(res, "contents", PARAM_ARRAY);
  for (i = 0; i < contents->length; i++) {
    binresult* item = contents->array[i];
    if (!pmobile_find_result(item, "isfolder", PARAM_BOOL)->num)
    {
      if (strncmp(
            pmobile_find_result(item, "name", PARAM_STR)->str, 
            filename,
            strlen(filename)
              ) == 0)
      {
        *hash = pmobile_find_result(item, "hash", PARAM_NUM)->num;
        return pmobile_find_result(item, "fileid", PARAM_NUM)->num;
      }
    }
  }
  return PSYNC_INVALID_FOLDERID;
}


int psdk_create_folder_by_path(const char *path, char **err){
  binparam params[]={P_STR("auth", auth_), P_STR("path", path), P_STR("timeformat", "timestamp")};
  binresult *res;
  int ret;
  ret=pmobile_api_run_command_get_res(mlib_, "createfolder", params, err, &res, apiserver);
  if (ret)
    return process_api_int_result(ret);
  pmobile_free(res);
  return 0;
}

static int check_write_permissions(){return 0;} //TODO implement

static pmobile_folderid_t create_index_folder(const char * path) {
  char *buff=NULL;
  uint32_t bufflen;
  int ind = 1;
  pmobile_folderid_t folderid;

  while (ind < 100) {
    folderid=PSYNC_INVALID_FOLDERID;
    bufflen = strlen(path) + 1 /*zero char*/ + 3 /*parenthesis*/ + 3 /*up to 3 digit index*/;
    buff = (char *) pmobile_malloc(mlib_, bufflen);
    snprintf(buff, bufflen - 1, "%s (%d)", path, ind);
    folderid=get_folderid_by_path(buff, 1);
    if ((folderid!=PSYNC_INVALID_FOLDERID)&&check_write_permissions(folderid)) {
      pmobile_free(buff);
      break;
    }
    ++ind;

    pmobile_free(buff);
  }
  return folderid;
}

int psdk_create_folder (const char * path) {
  pmobile_folderid_t folderid=get_folderid_by_path(path, 1);
  
  if (folderid==PSYNC_INVALID_FOLDERID) {
    folderid = create_index_folder(path);
      if (folderid==PSYNC_INVALID_FOLDERID)
        return 0;
  }
  return 1; 
}

pmobile_fileid_t get_fileid_by_path(const char * remotepath,const char * filename, uint64_t *hash)
{
  pthread_mutex_lock(&auth_lock_);
  if (!logged_in_)
    return 19;
  pthread_mutex_unlock(&auth_lock_);
  
  pmobile_fileid_t fileid =  PSYNC_INVALID_FOLDERID;
  
  binparam params[] = { P_STR("auth", auth_), P_STR("path", remotepath)};
  binresult *res;
  
  res = pmobile_api_run_command(mlib_, "listfolder", params, apiserver);
  if (unlikely_log(!res))
    return PSYNC_INVALID_FOLDERID;
  
  int ret = process_api_result(res);
  if (ret == 0) 
    fileid = extract_fileid(res, filename, hash);
  
  pmobile_free(res);
  return fileid;
}

pmobile_folderid_t get_folderid_by_path(const char * path, int create)
{
  uint64_t folderid;

  pthread_mutex_lock(&auth_lock_);
  if (!logged_in_)
    return 19;
  pthread_mutex_unlock(&auth_lock_);

  binparam params[] = { P_STR("auth", auth_), P_STR("path", path)};
  binresult *res;
  
  res = pmobile_api_run_command(mlib_, "listfolder", params, apiserver);
  if (unlikely_log(!res))
    return PSYNC_INVALID_FOLDERID;
  int ret = process_api_result(res);
  if (ret == 0)
    folderid = extract_folderid(res);
  else folderid = PSYNC_INVALID_FOLDERID;
    
  if (ret == 2005) {
    pmobile_free(res);
    res = pmobile_api_run_command((pmobile_t)(mlib_), "createfolder", params, apiserver);
    if (unlikely_log(!res))
      return PSYNC_INVALID_FOLDERID;
    ret = pmobile_find_result_num(res, "result");
    if (ret == 0)
      folderid = extract_folderid(res);
    else folderid = PSYNC_INVALID_FOLDERID;
  }

  pmobile_free(res);
  return folderid;
}
