#include "config.h"




int trim(char *in_str, char *out_str){
  if(in_str == NULL || strlen(in_str) == 0){
      return RET_ERROR;
  }
  
  int j = 0;
  for(int i=0; i < strlen(in_str); i++){
      if(in_str[i] == ' ' || in_str[i] == '\t' || in_str[i] == '\n'){
          continue;
      }
      out_str[j++] = in_str[i];
  }
  
  out_str[j] = '\0';
  return RET_SUCCESS;
}


int load_config_form_file(char *file_path, char *key, char *val){
  if(file_path == NULL || key == NULL || val == NULL){
	log_error("enter parameters cannot be nullptr");
	return RET_ERROR;
  }	

  FILE *fp = NULL;
  
  if((fp = fopen(file_path, "r")) == NULL){
    log_error("open file %s failed", file_path);
    return RET_ERROR;
  }
  
  char buf[MAX_BUF_LEN] = {0};
  while(fgets(buf, MAX_BUF_LEN, fp) != NULL){
    char trim_buf[MAX_BUF_LEN] = {0};
    if(trim(buf, trim_buf) != RET_SUCCESS){
      memset(buf, 0, MAX_BUF_LEN);
      continue;
    }
  
    int buf_len = strlen(trim_buf);
  
    // to skip text commet with flags "#" or "/*...*/"
    if((buf_len == 0)
    || (buf_len > 0 && trim_buf[0] == '#')
    || (buf_len > 1 && trim_buf[0] == '/' && trim_buf[1] =='*')){
      memset(buf, 0, MAX_BUF_LEN);
      continue;
    }
  
    char *p = strtok(trim_buf, "=");
    if (p != NULL){
      if(strcmp(p, key) == 0){
        if((p = strtok(NULL, ",")) != NULL){
          strcpy(val, p);
          break;
        }
      }
    }
  
    memset(buf, 0, MAX_BUF_LEN);
  }

  fclose(fp);

  if(strlen(val) == 0){
  	log_error("the config key %s has null value in file %s", key, file_path);
    return RET_ERROR;
  }
  
  return RET_SUCCESS;
}


int get_sniff_devs_from_file(char *file_path){
  if(file_path == NULL){
	log_error("enter parameters cannot be nullptr");
	return RET_ERROR;
  }	
	  
  FILE *fp = NULL;
  if((fp = fopen(file_path, "r")) == NULL){
    log_error("open file %s failed", file_path);
    return RET_ERROR;
  }
  
  char buf[MAX_BUF_LEN] = {0};
  while(fgets(buf, MAX_BUF_LEN, fp) != NULL){
    int buf_len = strlen(buf);
  
    // to skip text commet with flags "#" or "/*...*/"
    if((buf_len == 0)
    || (buf_len > 0 && buf[0] == '#')
    || (buf_len > 1 && buf[0] == '/' && buf[1] =='*')){
      memset(buf, 0, MAX_BUF_LEN);
      continue;
    }
  
    char *p = strtok(buf, "=");
    if (p != NULL){
	  char trim_p[MAX_BUF_LEN] = {0};
	  if(trim(p, trim_p) != RET_SUCCESS){
        memset(buf, 0, MAX_BUF_LEN);
        continue;
      }
      if(strcmp(trim_p, sniff_devs_key) == 0){
        if((p = strtok(NULL, "=")) != NULL){
		  char val_str[MAX_BUF_LEN] = {0}; 	
          strcpy(val_str, p);
		  char *v = strtok(val_str, ",");
		  char trim_v[MAX_BUF_LEN] = {0};
		  if(v != NULL){
			if(trim(v, trim_v) != RET_SUCCESS){
			  break;
			}
			
			strcpy(sniff_devs[0], trim_v);
			int i = 1;
			while(v = strtok(NULL, ",")){
			  memset(trim_v, 0, MAX_BUF_LEN);
			  if(trim(v, trim_v) != RET_SUCCESS){
			    break;
			  }
			
			  strcpy(sniff_devs[i++], trim_v);
			  if(i == MAX_SNIFF_DEVS_COUNT){
		        log_warn("the max sniff devices count is %d", MAX_SNIFF_DEVS_COUNT);
				break;
			  }
			}
		  }
		  break;
        }
      }
    }
  
    memset(buf, 0, MAX_BUF_LEN);
  }

  fclose(fp);

  if(strlen(sniff_devs[0]) == 0){
    return RET_ERROR;
  }

  return RET_SUCCESS;
}


