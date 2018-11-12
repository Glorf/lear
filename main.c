#include "master.h"
#include "config.h"

#include <stdio.h>

int main() {
    printf("Welcome to puthttpd!\n");
    initialize_config("../httpd.yaml");
    run_master();
}