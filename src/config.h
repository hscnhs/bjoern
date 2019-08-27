#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

enum control_server {
    START = 0, //default
    RESTART,
    STOP,
};

enum arg_type {
    ARG_START,
    ARG_END,
    ARG_PARENT,
    ARG_OPT_BOOL,
    ARG_OPT_STRING,
};

typedef struct Config
{
    char* host; //http mode, need port
    char* unixsock; //unix sock mode
    char* wsgi; //wsgi callable object
    char* home; //virtualenv ?
    char* pid; //only work with daemon mode
    //char *command; //for -c
    int port;
    int daemon; //daemonize
    //int start;
    //int stop;
    //int restart;
    control_server cs; //restart and stop only work with daemon mode 
};

typedef struct ArgumentOption
{
    char option;
    char* long_option;
    void* value;
    const char* help;
    arg_type type;
    ArgumentOption** next; // for child argument
};

typedef struct Argparse {
    int index;
    int argc;
    char** argv;
    char* value; //current value
    ArgumentOption* options;
};

extern void argparse(Argparse* ap, ArgumentOption* ao, int argc, char** argv);

#ifdef __cplusplus
}
#endif

#endif