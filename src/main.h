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
 * check if add option have a good formation
 * and keep only args and cut option but keep
 * timekeepper in flag matching
 * @param argc : lenght to argv
 * @param argv : parameters to command line
 * @param f_d  : flag to option date
 * @param f_co : flag to option compound
 * @param f_dz : flag to option dazibao
 * @param f_ty : flag to option type
 * @param f_in : flag to option "-" -> input
 * @return count to parameters
 **/
int check_option_add(int argc, char **argv, int *f_d, int *f_co, int *f_dz,
                int *f_ty, int *f_in);

/**
 * check if option type counter of args exist
 * @param argc : lenght to argv
 * @param type_args : to write a good version of type after to check
 * @param op_type : type write to command line
 * @param f_dz : flag to option dazibao
 * @return 0 is good or -1 to error
 **/
int check_type_args(int argc, char *type_args, char *op_type, int f_dz);

/**
 * check 2 point:
 *      - if path have a goof file
 *      - if tlv to create will be a good size
 * @param argc : lenght to argv
 * @param argv : parameters to command line
 * @param f_d  : flag to option date
 * @param f_co : flag to option compound
 * @param f_dz : flag to option dazibao
 * @return 0 is good or -1 to error
 **/
int check_args(int argc, char **argv, int *f_dz, int *f_co, int *f_d);

/**
 * manage to all check verification to parameters command line
 * @param argc : lenght to argv
 * @param argv : parameters to line command
 * @param daz : path from dazibao to execute action add
 * @return 0 is good or -1 to error
 **/
int cmd_add(int argc, char **argv, char *daz);

/**
 * create all tlv to ask to command line
 * and add to dazibao daz
 * @param argc : lenght to argv
 * @param argv : parameters to command line
 * @param f_co  : flag to option date
 * @param f_dz : flag to option compound
 * @param f_d : flag to option dazibao
 * @param f_in : flag to option - -> input
 * @param type : tab to type to args
 * @param daz : path to dazibao to execute add
 * @return 0 is good or -1 to error
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
