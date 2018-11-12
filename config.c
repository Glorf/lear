#include "config.h"

#include <stdio.h>
#include <yaml.h>


struct map {
    char key[64];
    char value[64];
};

typedef struct map s_map;

s_map config[10];
int config_num;

int initialize_config(char path[]) {
    yaml_parser_t parser;
    yaml_event_t event;
    FILE *file = fopen(path, "rb");

    if(file == NULL) {
        perror("Failed to open config file");
        return -1;
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, file);

    config_num = 0;
    do {
        if(!yaml_parser_parse(&parser, &event)) {
            perror("Error while parsing YAML");
            break;
        }
        if(event.type == YAML_SCALAR_EVENT) {
            strcpy(config[config_num].key, (char*)(event.data.scalar.value));
            yaml_event_delete(&event);
            yaml_parser_parse(&parser, &event);
            strcpy(config[config_num].value, (char*)(event.data.scalar.value));
            printf("%s: %s\n", config[config_num].key, config[config_num].value);
            config_num++;
        }
    } while(event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);


    yaml_parser_delete(&parser);
    fclose(file);

    return config_num;
}

char* read_config_string(char key[], char defaults[]) {
    char *result = defaults;
    for(int i=0; i<config_num; i++) {
        if(strcmp(config[i].key, key) == 0) {
            result = config[i].value;
            break;
        }
    }

    return result;
}

int read_config_int(char key[], char defaults[]) {
    return atoi(read_config_string(key, defaults));
}
