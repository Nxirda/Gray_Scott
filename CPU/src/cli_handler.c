#include "cli_handler.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

typedef struct arguments_s
{
    char flag;
    char *long_flag;
    u8 value;
} arguments_t;


static const int nb_opts        = 6;
static const int max_args_count = 17;
static const int max_digits     = 15;
static const arguments_t arguments[6] = 
{
    {'r', "-num_rows"        , 1},
    {'c', "-num_cols"        , 1},
    {'f', "-output_frequency", 1},
    {'s', "-simulation_steps", 1},
    {'o', "-output_file"     , 1},
    {'i', "-interactive"     , 0}
};

static void print_helper(char *prog_name)
{
    fprintf(stdout, "Usage is %s [-h]\n", prog_name);
    puts("");

    fprintf(stdout, "Available options are : \n");
    puts("");

    for(int i = 0; i < nb_opts; i++)
    {
        fprintf(stdout, "Opt : -%c \t -%s\n", arguments[i].flag, arguments[i].long_flag);
    }
    return;
}

static u8 string_is_digit(char *string, size_t len)
{
    size_t i = 0;
    while((i < len) && (string[i] != '\0'))
    {
        if((string[i] < '0') || (string[i] > '9'))
            return 1;

        i++;
    }
    return 0;
}

void parse_arguments(int argc, char *argv[argc+1], args_t *args)
{
    args->num_rows          = 10;
    args->num_cols          = 10;
    args->output_frequency  = 5;
    args->steps             = 20;
    args->file_name         = "output.bin";
    args->interactive       = 0;

    if(argc == 1)
        return;
   
    char *curr_arg = "";
    for(int i = 1; i < argc; i++)
    {
        curr_arg = argv[i];
        
        if(!strncmp(curr_arg, "--", max_args_count))
            return;

        if(curr_arg[0] != '-')
        {
            fprintf(stderr, "Invalid argument %s, use --help to see the available commands\n"
                    , argv[i]);
            exit(1);
        }
       
        curr_arg += 1;
        if((curr_arg[0] == 'h') || !strncmp(curr_arg, "-help", max_args_count))
        {
            print_helper(argv[0]);
            exit(0);
        }

        // Needs more checks but flemme
        if(argc > i+1)
        {
            char *next_arg = argv[i+1];
            size_t len = strnlen(next_arg, max_digits);

            if((*curr_arg == arguments[0].flag) || 
                !strncmp(curr_arg, arguments[0].long_flag, max_args_count))
            {
                if(string_is_digit(next_arg, len))
                { 
                    goto invalid_argument;
                }
                args->num_rows = strtoul(next_arg, NULL, 10);
            }
            else if((*curr_arg == arguments[1].flag) || 
                !strncmp(curr_arg, arguments[1].long_flag, max_args_count))
            {
                if(string_is_digit(next_arg, len))
                {
                    goto invalid_argument;
                }
                args->num_cols = strtoul(next_arg, NULL, 10);
            }
            else if((*curr_arg == arguments[2].flag) || 
                !strncmp(curr_arg, arguments[2].long_flag, max_args_count))
            {
                if(string_is_digit(next_arg, len))
                {
                    goto invalid_argument;
                }
                args->output_frequency = strtoul(next_arg, NULL, 10);
            }
            else if((*curr_arg == arguments[3].flag) || 
                !strncmp(curr_arg, arguments[3].long_flag, max_args_count))
            {
                if(string_is_digit(next_arg, len))
                {
                    goto invalid_argument;
                }
                args->steps = strtoul(next_arg, NULL, 10);
            }
            else if((*curr_arg == arguments[4].flag) || 
                !strncmp(curr_arg, arguments[4].long_flag, max_args_count))
            {
                args->file_name = next_arg;
            }
            else if((*curr_arg == arguments[5].flag) || 
                !strncmp(curr_arg, arguments[5].long_flag, max_args_count))
            {
                if(string_is_digit(next_arg, len))
                {
                    goto invalid_argument;
                }
                args->interactive = (u8)strtoul(next_arg, NULL, 10);
            }
            else
            {
                goto unknown_flag; 
            }
            i++;
        }
        else goto missing_argument;
    }
    return;

    invalid_argument :
        fprintf(stderr, "Invalid argument provided to the %s flag\n", curr_arg);
        exit(1);

    unknown_flag :
        fprintf(stderr, "Unkown flag %s while parsing\n", curr_arg);
        exit(1);

    missing_argument :
        fprintf(stderr, "Missing argument to flag %s while parsing\n", curr_arg);
        exit(1);
}
