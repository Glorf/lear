//
// Created by mbien on 08.11.18.
//

#ifndef PUTHTTPD_CONFIG_H
#define PUTHTTPD_CONFIG_H

int initialize_config(char path[]);
char* read_config_string(char key[], char defaults[]);
int read_config_int(char key[], char defaults[]);

#endif //PUTHTTPD_CONFIG_H
