/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "argparse.h"

#define OPT_UNSET 1
#define OPT_LONG  (1 << 1)

static const char *
prefix_skip(const char *str, const char *prefix)
{
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static int
prefix_cmp(const char *str, const char *prefix)
{
    for (;; str++, prefix++)
        if (!*prefix) {
            return 0;
        } else if (*str != *prefix) {
            return (unsigned char)*prefix - (unsigned char)*str;
        }
}

static void
argparse_error(argparse *ap, argparse_option *opt,
               const char *reason, int flags)
{
    (void)ap;
    if (flags & OPT_LONG) {
        fprintf(stderr, "error: option `--%s` %s\n", opt->long_option, reason);
    } else {
        fprintf(stderr, "error: option `-%c` %s\n", opt->option, reason);
    }
    exit(1);
}

static int
argparse_getvalue(argparse* ap, argparse_option* opt,  int flags)                
{
    const char *s = NULL;
    switch (opt->type) {
    case ARGPARSE_OPT_BOOLEAN:
        if (flags & OPT_UNSET) {
            *(int *)opt->value = *(int *)opt->value - 1;
        } else {
            *(int *)opt->value = *(int *)opt->value + 1;
        }
        if (*(int *)opt->value < 0) {
            *(int *)opt->value = 0;
        }
        break;
    case ARGPARSE_OPT_STRING:
        if (ap->optvalue) {
            *(const char **)opt->value = ap->optvalue;
            ap->optvalue = NULL;
        } else if (ap->argc > 1) {
            ap->argc--;
            *(const char **)opt->value = *++ap->argv;
        } else {
            argparse_error(ap, opt, "requires a value", flags);
        }
        break;
    case ARGPARSE_OPT_INTEGER:
        errno = 0;
        if (ap->optvalue) {
            *(int *)opt->value = strtol(ap->optvalue, (char **)&s, 0);
            ap->optvalue = NULL;
        } else if (ap->argc > 1) {
            ap->argc--;
            *(int *)opt->value = strtol(*++ap->argv, (char **)&s, 0);
        } else {
            argparse_error(ap, opt, "requires a value", flags);
        }
        if (errno)
            argparse_error(ap, opt, strerror(errno), flags);
        if (s[0] != '\0')
            argparse_error(ap, opt, "expects an integer value", flags);
        break;
    case ARGPARSE_OPT_FLOAT:
        errno = 0;
        if (ap->optvalue) {
            *(float *)opt->value = strtof(ap->optvalue, (char **)&s);
            ap->optvalue       = NULL;
        } else if (ap->argc > 1) {
            ap->argc--;
            *(float *)opt->value = strtof(*++ap->argv, (char **)&s);
        } else {
            argparse_error(ap, opt, "requires a value", flags);
        }
        if (errno)
            argparse_error(ap, opt, strerror(errno), flags);
        if (s[0] != '\0')
            argparse_error(ap, opt, "expects a numerical value", flags);
        break;
    default:
        assert(0);
    }

    return 0;
}

static void
argparse_options_check(argparse_option* ao)
{
    for (; ao->type != ARGPARSE_OPT_END; ao++) {
        switch (ao->type) {
        case ARGPARSE_OPT_END:
        case ARGPARSE_OPT_BOOLEAN:
        case ARGPARSE_OPT_INTEGER:
        case ARGPARSE_OPT_FLOAT:
        case ARGPARSE_OPT_STRING:
        case ARGPARSE_OPT_GROUP:
            continue;
        default:
            fprintf(stderr, "wrong option type: %d", ao->type);
            break;
        }
    }
}

static int
argparse_short_opt(argparse* ap, argparse_option *ao)
{
    for (; ao->type != ARGPARSE_OPT_END; ao++) {
        if (ao->option == *ap->optvalue) {
            ap->optvalue = ap->optvalue[1] ? ap->optvalue + 1 : NULL;
            return argparse_getvalue(ap, ao, 0);
        }
    }
    return -2;
}

static int
argparse_long_opt(argparse* ap, argparse_option* ao)
{
    for (; ao->type != ARGPARSE_OPT_END; ao++) {
        const char *rest;
        int opt_flags = 0;
        if (!ao->long_option)
            continue;

        rest = prefix_skip(ap->argv[0] + 2, ao->long_option);
        if (!rest) {
            // negation disabled?
            if (ao->flags & OPT_NONEG) {
                continue;
            }
            // only OPT_BOOLEAN/OPT_BIT supports negation
            if (ao->type != ARGPARSE_OPT_BOOLEAN) {
                continue;
            }

            if (prefix_cmp(ap->argv[0] + 2, "no-")) {
                continue;
            }
            rest = prefix_skip(ap->argv[0] + 2 + 3, ao->long_option);
            if (!rest)
                continue;
            opt_flags |= OPT_UNSET;
        }
        if (*rest) {
            if (*rest != '=')
                continue;
            ap->optvalue = rest + 1;
        }
        return argparse_getvalue(ap, ao, opt_flags | OPT_LONG);
    }
    return -2;
}

int
argparse_init(argparse* ap, argparse_option* ao, int flags)
{
    memset(ap, 0, sizeof(*ap));
    ap->options     = ao;
    ap->flags       = flags;
    ap->description = NULL;
    ap->epilog      = NULL;
    return 0;
}

void
argparse_describe(argparse* ap, const char *description,
                  const char *epilog)
{
    ap->description = description;
    ap->epilog      = epilog;
}

int
argparse_parse(argparse* ap, int argc, char** argv)
{
    ap->argc = argc - 1;
    ap->argv = malloc((argc - 1) * sizeof(char*));
    for(int i = 1; i < argc - 1; i++)
        ap->argv[i - 1] = argv[i];
    
    ap->out = malloc(argc * sizeof(char*));
    for(int i = 0; i < argc; i++)
        ap->out[i] = argv[i];

    argparse_options_check(ap->options);

    for (; ap->argc; ap->argc--, ap->argv++) {
        const char *arg = ap->argv[0];
        if (arg[0] != '-' || !arg[1]) {
            if (ap->flags & ARGPARSE_STOP_AT_NON_OPTION) {
                goto end;
            }
            // if it's not option or is a single char '-', copy verbatim
            ap->out[ap->cpidx++] = ap->argv[0];
            continue;
        }
        // short option
        if (arg[1] != '-') {
            ap->optvalue = arg + 1;
            switch (argparse_short_opt(ap, ap->options)) {
            case -1:
                break;
            case -2:
                goto unknown;
            }
            while (ap->optvalue) {
                switch (argparse_short_opt(ap, ap->options)) {
                case -1:
                    break;
                case -2:
                    goto unknown;
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            ap->argc--;
            ap->argv++;
            break;
        }
        // long option
        switch (argparse_long_opt(ap, ap->options)) {
        case -1:
            break;
        case -2:
            goto unknown;
        }
        continue;

unknown:
        fprintf(stderr, "error: unknown option `%s`\n", ap->argv[0]);
        argparse_usage(ap);
        exit(1);
    }

end:
    memmove(ap->out + ap->cpidx, ap->argv,
            ap->argc * sizeof(*ap->out));
    ap->out[ap->cpidx + ap->argc] = NULL;

    return ap->cpidx + ap->argc;
}

void
argparse_usage(argparse *ap)
{
    fprintf(stdout, "Usage:\n");

    // print description
    if (ap->description)
        fprintf(stdout, "%s\n", ap->description);

    fputc('\n', stdout);

    argparse_option* ao;

    // figure out best width
    size_t usage_opts_width = 0;
    size_t len;
    ao = ap->options;
    for (; ao->type != ARGPARSE_OPT_END; ao++) {
        len = 0;
        if ((ao)->option) {
            len += 2;
        }
        if ((ao)->option && (ao)->long_option) {
            len += 2;           // separator ", "
        }
        if ((ao)->long_option) {
            len += strlen((ao)->long_option) + 2;
        }
        if (ao->type == ARGPARSE_OPT_INTEGER) {
            len += strlen("=<int>");
        }
        if (ao->type == ARGPARSE_OPT_FLOAT) {
            len += strlen("=<flt>");
        } else if (ao->type == ARGPARSE_OPT_STRING) {
            len += strlen("=<str>");
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4;      // 4 spaces prefix

    ao = ap->options;
    for (; ao->type != ARGPARSE_OPT_END; ao++) {
        size_t pos = 0;
        int pad    = 0;
        if (ao->type == ARGPARSE_OPT_GROUP) {
            fputc('\n', stdout);
            fprintf(stdout, "%s", ao->help);
            fputc('\n', stdout);
            continue;
        }
        pos = fprintf(stdout, "    ");
        if (ao->option) {
            pos += fprintf(stdout, "-%c", ao->option);
        }
        if (ao->long_option && ao->option) {
            pos += fprintf(stdout, ", ");
        }
        if (ao->long_option) {
            pos += fprintf(stdout, "--%s", ao->long_option);
        }
        if (ao->type == ARGPARSE_OPT_INTEGER) {
            pos += fprintf(stdout, "=<int>");
        } else if (ao->type == ARGPARSE_OPT_FLOAT) {
            pos += fprintf(stdout, "=<float>");
        } else if (ao->type == ARGPARSE_OPT_STRING) {
            pos += fprintf(stdout, "=<str>");
        }
        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        fprintf(stdout, "%*s%s\n", pad + 2, "", ao->help);
    }

    // print epilog
    if (ap->epilog)
        fprintf(stdout, "%s\n", ap->epilog);
}

int
argparse_help_cb(argparse *ap, argparse_option *ao)
{
    argparse_usage(ap);
    exit(0);
}
