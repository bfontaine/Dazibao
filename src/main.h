#ifndef _MAIN_H
#define _MAIN_H 1

/** @file
 * Main program used for the command-line interface
 **/

/** format of the help text */
#define CLI_USAGE_FMT \
        "Usage:\n" \
        "    %s <cmd> <option and args> <dazibao>\n\n" \
        "cmd:\n" \
        "    create: create an empty dazibao\n" \
        "        command: create [--merge <dazibao args to merge>] <dazibao>" \
                                                                         "\n" \
        "        options:\n" \
        "            -m, --merge: (todo)\n" \
        "\n" \
        "    add: add a TLV\n" \
        "        command: add [--date] [--compound] <tlv args> <dazibao>\n" \
        "        options:\n" \
        "            -d, --date: (todo)\n" \
        "            -c, --compound: (todo)\n" \
        "            -C, --dazibao: (todo)\n" \
        "\n" \
        "    rm: remove a TLV\n" \
        "        command: rm <offset> <dazibao>\n" \
        "\n" \
        "    dump: dump a Dazibao\n" \
        "        command: dump [--debug ] [--depth] <depth> <dazibao>\n" \
        "        options:\n" \
        "            -D, --depth: (todo)\n" \
        "            -d, --debug: (todo)\n" \
        "\n" \
        "    compact: compact a dazibao\n" \
        "        command: compact [--recusive] <dazibao>\n" \
        "        options:\n" \
        "            -r, --recursive: (todo)\n"

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param daz
 **/
int cmd_add(int argc, char **argv, char *daz);

/**
 * @param daz
 **/
int action_add(int argc, char **argv, int f_co, int f_dz, int f_d, int f_in,
       char *type , char *daz);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param daz
 **/
int cmd_rm(int argc, char **argv, char *daz);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param daz
 **/
int cmd_dump(int argc, char **argv, char *daz);

/**
 * @param daz
 * @param flag_debug
 * @param flag_depth
 **/
int action_dump(char *daz, int flag_debug, int flag_depth);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param daz
 **/
int cmd_compact(int argc, char **argv, char *daz);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param daz
 **/
int cmd_create(int argc, char **argv, char *daz);

/**
 * print the help text
 * @param exec the name of the executable
 **/
void print_usage(char *exec);

#endif
