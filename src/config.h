#ifndef NDPI_HTTP_FILTER_CONFIG_H
#define NDPI_HTTP_FILTER_CONFIG_H

#include "common.h"


//  filter all blank space in the input string , use it as output string
int trim(char *in_str, char *out_str);

int get_config_form_file(char *file_path, char *key, char *val);

int get_sniff_devs_from_file(char *file_path);
#endif
