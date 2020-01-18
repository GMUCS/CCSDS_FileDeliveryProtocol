
/*------------------------------------------------------------------------------
CMPT-361-AS50(1) - 2017 Fall - Introduction to Networks
Assignment #2
Evan Giese 1689223

This is my utils.c file, it contains useful function and abstract data types
to use for general functionality. 
------------------------------------------------------------------------------*/
#include <stdio.h>
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <libgen.h>
#include "port.h"


#define MAX_LEN 255
#define ID_LEN 10

//see header file
int checkAlloc(void *mem)
{
    if (mem == NULL)
    {
        ssp_error("malloc");
        return -1;
    }
    return 1;
}


//see header file
Config *configuration(int argc, char **argv)
{
    int ch;
    Config *conf = calloc(sizeof(Config), 1);
    if (checkAlloc(conf) < 0) 
        return NULL;
        

    conf->timer = 15;
    conf->verbose_level = 0;
    conf->client_cfdp_id = 0;
    conf->my_cfdp_id = 0;

    uint32_t tmp;
    while ((ch = getopt(argc, argv, "t: i: c: v: h")) != -1)
    {
        switch (ch)
        {
        case 't':
            tmp = strtol(optarg, NULL, 10);
            conf->timer = tmp;
            break;

        case 'i':
            tmp = strtol(optarg, NULL, 10);
            conf->my_cfdp_id = tmp;
            break;
        
        case 'v':
            tmp = strtol(optarg, NULL, 10);
            conf->verbose_level = (uint8_t) tmp;
            break;

        case 'c': 
            tmp = strtol(optarg, NULL, 10);
            conf->client_cfdp_id = tmp;
            break;

        case 'h':
            printf("\n-----------HELP MESSAGE------------\n");
            printf("\nusage: %s [options] \n\n", basename(argv[0]));
            printf("Options: %s%s%s%s\n",
                    "-t timeout\n",
                    "-i my cfdp id for server\n",
                    "-c client id\n",
                    "-v verbose level (1-3)"
                    "-h HelpMessage");

            printf("Default port number mis 1111\n");
            printf("\n---------------END----------------\n");
            break;
        default:
            printf("\ngot something not found using default\n");
            break;
        }
    }
    return conf;
}




//size is the number of bytes we want to print
void ssp_print_hex(char *stuff, int size){
    
    uint32_t current_packet_index = 0;
    ssp_printf("printing number of bytes: %u\n", size);

        for (int j = 0; j < size; j += 1) {
            ssp_printf("%x.", 
            stuff[current_packet_index]);
            current_packet_index += 1; 
        }
        ssp_printf("\n");
}   


int is_negative(int number) {
    int negative_mask = 0x80000000;
    int is_negative = number & negative_mask;
    return is_negative;
}


void reset_timeout(int *prevtime) {
    *prevtime = ssp_time_count();
}

int check_timeout(int *prevtime, uint32_t timeout) {

    int prev = *prevtime;
    int current_time = ssp_time_count();
    int time_out = prev + timeout;

    if (current_time >= time_out) {
        *prevtime = current_time;
        return 1;
    }
    //wrap around the overflow condition
    else if (current_time < prev) {
        *prevtime = current_time;
    }
    return 0; 
}
