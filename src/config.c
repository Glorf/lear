#include "config.h"
#include "types.h"

#include <stdio.h>
#include <yaml.h>


struct map {
    char key[64];
    char value[64];
};

typedef struct map s_map;

s_map config[40];
int config_num;

s_global_config *global_config;

int init_config(char path[]) {
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

    typedef enum {KEY, VALUE} yaml_state;
    yaml_state state = KEY;
    char alias[64] = {'\0'};
    char key[64] = {'\0'};

    do {
        if(!yaml_parser_parse(&parser, &event)) {
            perror("Error while parsing YAML");
            break;
        }
        switch(event.type) {
            case YAML_ALIAS_EVENT:
            case YAML_SEQUENCE_START_EVENT:
            case YAML_SEQUENCE_END_EVENT:
            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_DOCUMENT_END_EVENT:
            case YAML_STREAM_END_EVENT:
            case YAML_NO_EVENT:
                continue;
            case YAML_SCALAR_EVENT:
                if (state == KEY) {
                    strcpy(key, (char *) (event.data.scalar.value));
                    state = VALUE;
                } else if (state == VALUE) {
                    strcpy(config[config_num].key, alias);
                    strcat(config[config_num].key, key);
                    strcpy(config[config_num].value, (char *) (event.data.scalar.value));
                    state = KEY;
                    config_num++;
                }
                break;
            case YAML_MAPPING_START_EVENT:
                if(state == VALUE) {
                    strcpy(alias, key);
                    strcat(alias, ".");
                    state = KEY;
                }
                break;
            case YAML_MAPPING_END_EVENT:
                alias[0] = '\0';
                //TODO: add deeper indentations
                break;
        }
    } while(event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);


    yaml_parser_delete(&parser);
    fclose(file);

    init_global_config();

    return config_num;
}

s_string read_config_string(char key[], char defaults[]) {
    s_string result;
    result.length = 0;
    for(int i=0; i<config_num; i++) {
        if(strcmp(config[i].key, key) == 0) {
            result = create_string(config[i].value, strlen(config[i].value));
            break;
        }
    }

    if(result.length == 0) {
        result = create_string(defaults, strlen(defaults));
    }

    return result;
}

long read_config_long(char key[], char defaults[]) {
    s_string str = read_config_string(key, defaults);
    char *strptr = to_c_string(&str);
    delete_string(&str);
    long ret = strtol(strptr, NULL, 10);
    free(strptr);

    return ret;
}

s_global_config *get_global_config() {
    return global_config;
}

void init_global_config() {
    //populate with defaults
    global_config = malloc(sizeof(s_global_config));
    global_config->max_URI_length = read_config_long("maxURILength", "128");
    global_config->max_request_size = read_config_long("maxRequestSize", "3072");
    global_config->max_block_size = read_config_long("requestBlockSize", "512");
    global_config->request_timeout_sec = read_config_long("requestTimeout", "30");

    //TODO: Move this to dynamic hosts map
    global_config->host.name = read_config_string("host.name", "localhost");
    global_config->host.root_path = read_config_string("host.webDir", "/home/mbien/Projekty/lear/web");
    global_config->host.not_found_path = read_config_string("host.notFound", "/404.html");
}