#ifndef _MAIN_H
#define _MAIN_H 1

/** @file
 * Main program used for the command-line interface
 **/

/** format of the help text */
#define CLI_USAGE_FMT \
        "Usage:\n" \
        "    %s <cmd> <option and args> <dz_pathibao>\n\n" \
        "cmd:\n" \
        "    create: create an empty dz_pathibao\n" \
        "        command: create [--merge <dz_pathibao args to merge>] <dz_pathibao>" \
                                                                         "\n" \
        "        options:\n" \
        "            -m, --merge: (todo)\n" \
        "\n" \
        "    add: add a TLV\n" \
        "        command: add [--type] <type args> [--dz_pathibao] <dz_pathibo>\n" \
        "                       [--date] [--compound] <tlv args> <dz_pathibao>\n"\
        "        options:\n" \
        "            --type: allow to give a type with parameters, by\n" \
        "                    order for all args excepted to dz_pathibao option\n" \
        "            --date: create a tlv dated with all args after option\n" \
        "            --compound: create a tlv compound with alla args\n" \
        "                        after option\n" \
        "            --dz_pathibao: insert a dz_pathibao when size is respected\n" \
        "                       with tlv compound\n" \
        "\n" \
        "    rm: remove a TLV\n" \
        "        command: rm <offset> <dz_pathibao>\n" \
        "\n" \
        "    dump: dump a Dazibao\n" \
        "        command: dump [--debug ] [--depth] <depth> <dz_pathibao>\n" \
        "        options:\n" \
        "            -D, --depth: allow to show inside tlv\n" \
        "            -d, --debug: show tlv PAD1 and PADN\n" \
        "\n" \
        "    compact: compact a dz_pathibao\n" \
        "        command: compact <dz_pathibao>\n" \
        "\n" \
        "    extract: extract tlv from dz_pathibao\n" \
        "        command: extract <offset> <path futur file> <dz_pathibao>\n" \
        "\n"

/**
 * Parse the 'add' command options and return the number of trailing arguments.
 * @param argc length of argv
 * @param argv parameters from the command line
 * @param date_idx result parameter. Will be filled with the index of the date
 * in the resulting argv.
 * @param compound_idx result parameter. Will be filled with the index of the
 * '--compound' argument in the resulting argv.
 * @param dz_idx result parameter. Will be filled with the dz_pathibao index name
 * in the resulting argv.
 * @param type_idx result parameter. Will be filled with the type index in the
 * resulting argv.
 * @param input_idx result parameter. Will be filled with the input index of
 * the in the resulting argv.
 * @return parameters count
 **/
int check_option_add(int argc, char **argv, int *date_idx, int *compound_idx,
                int *dz_idx, int *type_idx, int *input_idx);

/**
 * check if option type counter of args exist
 * @param argc : lenght to argv
 * @param type_args : to write a good version of type after to check
 * @param op_type : type write to command line
 * @param f_dz : flag to option dz_pathibao
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
 * @param f_dz : flag to option dz_pathibao
 * @return 0 is good or -1 to error
 **/
int check_args(int argc, char **argv, int *f_dz, int *f_co, int *f_d);

/**
 * manage to all check verification to parameters command line
 * @param argc : lenght to argv
 * @param argv : parameters to line command
 * @param dz_path : path from dz_pathibao to execute action add
 * @return 0 is good or -1 to error
 **/
int cmd_add(int argc, char **argv, char *dz_path);

/**
 * create all tlv to ask to command line
 * and add to dz_pathibao dz_path
 * @param argc : lenght to argv
 * @param argv : parameters to command line
 * @param f_co  : flag to option date
 * @param f_dz : flag to option compound
 * @param f_d : flag to option dz_pathibao
 * @param f_in : flag to option - -> input
 * @param type : tab to type to args
 * @param dz_path : path to dz_pathibao to execute add
 * @return 0 is good or -1 to error
 **/
int action_add(int argc, char **argv, int f_co, int f_dz, int f_d, int f_in,
                char *type , char *dz_path);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param dz_path
 **/
int cmd_rm(int argc, char **argv, char *dz_path);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param dz_path
 **/
int cmd_dump(int argc, char **argv, char *dz_path);

/**
 * @param dz_path
 * @param flag_debug
 * @param flag_depth
 **/
int action_dump(char *dz_path, int flag_debug, int flag_depth);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param dz_path
 **/
int cmd_compact(int argc, char **argv, char *dz_path);

/**
 * @param argc arguments count
 * @param argv arguments array
 * @param dz_path
 **/
int cmd_create(int argc, char **argv, char *dz_path);

/**
 * print the help text
 * @param exec the name of the executable
 **/
void print_usage(char *exec);

/**
 * choose tlv to extract , if dated return tlv inside
 * make and realize command compact
 * @param dz_path
 * @param tlv
 * @param off ,is offset to tlv from dazibao
 **/
int choose_tlv_extract(dz_t *dz, tlv_t *tlv, long off);

/**
 * make and realize command compact
 * @param argc arguments count
 * @param argv arguments array
 * @param dz_path
 **/
int cmd_extract(int argc , char **argv, char *dz_path);
#endif
