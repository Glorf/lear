//
// Created by mbien on 08.11.18.
//

#ifndef PUTHTTPD_CONFIG_H
#define PUTHTTPD_CONFIG_H

#include "extras.h"

int init_config(char path[]);
s_string read_config_string(char key[], char defaults[]);
long read_config_long(char key[], char defaults[]);

#endif //PUTHTTPD_CONFIG_H
