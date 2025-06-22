#pragma once

#include <stdio.h>
#include <stdlib.h>

#define BOLD_RED     "\033[1;31m"
#define BOLD_GREEN   "\033[1;32m"
#define BOLD_YELLOW  "\033[1;33m"

#define RED          "\033[0;31m"
#define GREEN        "\033[0;32m"
#define YELLOW       "\033[0;33m"

#define RESET        "\033[0m"  // Reset color

// Ce serait bien d'avoir __FILE__ et __LINE__ dans les erreurs et autres
#define gs_info_print(fmt, ...)                                                \
do {                                                                           \
    fprintf(stderr, BOLD_GREEN "[INFO]" RESET ":\t\t"  fmt "\n", __VA_ARGS__); \
} while(0)

#define gs_warn_print(fmt, ...)                                                \
do {                                                                           \
    fprintf(stderr, BOLD_YELLOW "[WARNING]" RESET ":\t" fmt "\n", __VA_ARGS__);\
} while(0)  

#define gs_error_print(fmt, ...)                                               \
do{                                                                            \
    fprintf(stderr, BOLD_RED "[ERROR]" RESET ":\t\t"  fmt "\n", __VA_ARGS__);  \
    exit(1);                                                                   \
} while(0)

#ifdef NDEBUG
    #define gs_debug_print(fmt, ...) ((void)0);
#else
    #define gs_debug_print(fmt, ...)                            \
    do {                                                        \
        fprintf(stdout, fmt "\n", __VA_ARGS__);                 \
    } while(0)
#endif
