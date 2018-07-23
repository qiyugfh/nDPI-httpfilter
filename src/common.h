// Created by fanghua on 2018-06-05
//

#ifndef NDPI_HTTP_FILTER_COMMON_H
#define NDPI_HTTP_FILTER_COMMON_H

#include <stdio.h>
#include <zlog.h>
#include <string.h>





#define MAX_SNIFF_DEVS_COUNT 8
#define MAX_BUF_LEN 256
#define RET_SUCCESS 0
#define RET_ERROR -1


extern const char *sniff_devs_key;
extern const char *filter_exp_key;
extern char sniff_devs[MAX_SNIFF_DEVS_COUNT][MAX_BUF_LEN];
extern char filter_exp[MAX_BUF_LEN];
extern zlog_category_t *root_category;
extern int num_process;


#define print_out(...) fprintf(stdout, __VA_ARGS__)

#define log_debug(...) zlog_debug(root_category, __VA_ARGS__)
#define log_error(...) zlog_error(root_category, __VA_ARGS__)
#define log_info(...) zlog_info(root_category, __VA_ARGS__)
#define log_warn(...) zlog_warn(root_category, __VA_ARGS__)




#endif

