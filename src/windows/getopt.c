#include "getopt.h"
#include <stdio.h>
#include <string.h>

char *optarg;
int optind = 1, opterr = 1, optopt;
static const char *nextchar;

static int short_option(int argc, char *const argv[], const char *optstring)
{
    const char *spec; char option;
    if (nextchar == NULL || *nextchar == '\0') {
        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') return -1;
        if (strcmp(argv[optind], "--") == 0) { ++optind; return -1; }
        nextchar = argv[optind] + 1;
    }
    option = *nextchar++; optopt = option; spec = strchr(optstring, option);
    if (spec == NULL || option == ':') {
        if (*nextchar == '\0') ++optind;
        if (opterr && optstring[0] != ':') fprintf(stderr, "unknown option -- %c\n", option);
        return '?';
    }
    if (spec[1] == ':') {
        int optional = spec[2] == ':';
        if (*nextchar != '\0') { optarg = (char *)nextchar; ++optind; }
        else if (!optional && optind + 1 < argc) { optarg = argv[++optind]; ++optind; }
        else { optarg = NULL; ++optind; if (!optional) return optstring[0] == ':' ? ':' : '?'; }
        nextchar = NULL;
    } else if (*nextchar == '\0') { ++optind; nextchar = NULL; optarg = NULL; }
    return option;
}
int getopt(int argc, char *const argv[], const char *optstring) { return short_option(argc, argv, optstring); }
int getopt_long(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex)
{
    if ((nextchar == NULL || *nextchar == '\0') && optind < argc && strncmp(argv[optind], "--", 2) == 0 && argv[optind][2]) {
        const char *name = argv[optind] + 2, *equals = strchr(name, '=');
        size_t length = equals ? (size_t)(equals - name) : strlen(name); int i;
        for (i = 0; longopts[i].name; ++i) {
            if (strlen(longopts[i].name) == length && strncmp(name, longopts[i].name, length) == 0) {
                if (longindex) *longindex = i; optarg = equals ? (char *)(equals + 1) : NULL;
                if (longopts[i].has_arg == required_argument && !optarg) {
                    if (optind + 1 >= argc) { ++optind; return '?'; } optarg = argv[++optind];
                }
                ++optind; if (longopts[i].flag) { *longopts[i].flag = longopts[i].val; return 0; } return longopts[i].val;
            }
        }
        ++optind; return '?';
    }
    return short_option(argc, argv, optstring);
}

