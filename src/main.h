#ifndef _MAIN_H
#define _MAIN_H 1

/** @file
 * Main program used for the command-line interface
 **/

#define CLI_USAGE_FMT \
        "Usage:\n" \
        "    %s <cmd> <option and args> <dazibao>\n\n" \
        "cmd:\n" \
        "    create: create a empty dazibao or dazibao merges into arguments" \
                                                                         "\n" \
        "        command: create [--merge] <dazibao args to merge> <dazibao>" \
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

int cmd_add(int argc, char ** argv, char * daz);
int action_add(char *daz, unsigned char type);
int cmd_rm(int argc, char ** argv, char * daz);
int cmd_dump(int argc, char ** argv, char * daz);
int action_dump(char *daz, int flag_debug, int flag_depth);
int cmd_compact(int argc, char ** argv, char * daz);
int cmd_create(int argc, char ** argv, char * daz);
void print_usage(char *exec);

#endif
