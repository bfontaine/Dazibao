#ifndef _CLI_H
#define _CLI_H 1

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

#define DZCLI_TYPE_SEPARATOR ","

int add(char *file, int in);
int mk_tlv(int argc, char **argv, int in, int out);
int compact_dz(char *file);
#endif
