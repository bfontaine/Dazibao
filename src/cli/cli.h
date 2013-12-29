#ifndef _CLI_H
#define _CLI_H 1

/** @file
 * Main program used for the command-line interface
 **/

/** format of the help text */
#define CLI_USAGE_FMT "RTFM"

#include "tlv.h"
#include "mdazibao.h"

/* TODO document functions below */

/**
 * Make a tlv and store it in tlv param
 * @param tlv TLV to set
 * @param argc number of input(s)
 * @param argv inputs(s)
 * @param type types used to force type (or NULL)
 * @param date flag determinating if we want a dated TLV or not
 * @return 0 on success, -1 on error
 **/
int cli_mk_tlv(tlv_t *tlv, int argc, char **argv, char *type, char date);

/**
 * Parse arguments and make a tlv from inputs
 * @param argc number of argument(s)
 *        (probably should be (argc - 2) from main)
 * @param argv argument(s)
 *        (probably should be &argv[1] from main)
 * @return 0 on success, -1 on error
 * @see cli_mk_tlv
 **/
int cli_add(int argc, char **argv);

/**
 * Extract a TLV from a dazibao and write it into a file.
 * The file name is offset + file extension.
 * @param dz dazibao where the TLV comes from.
 * @param offset offset of the TLV
 * @param name_mod will be added to offset when creating file.
 *        Usefull to avoid name collision.
 * @return 0 on success, -1 on error
 **/
int cli_extract_tlv(dz_t *dz, off_t offset, int name_mod);

/**
 * Extract a long TLV
 * @param dz dazibao where the TLV comes from.
 * @param offset offset of the TLV
 * @param name_mod will be added to offset when creating file.
 *        Usefull to avoid name collision.
 * @return 0 on success, -1 on error
 **/
int cli_extract_ltlv(dz_t *dz, tlv_t *tlv, int offset, int name_mod);

/**
 * Extract every TLV in a dazibao.
 * dz_next_tlv(dz, ...) *MUST* return the first TLV.
 * Note that this function break dazibao type abstraction (meaning that
 * any change in dazibao structure in API will break this function).
 * @param dz dazibao containing TLVs
 * @param name_mod will be added to offset when creating file.
 *        Usefull to avoid name collision.
 * @return 0 on success, -1 on error
 **/
int cli_extract_all(dz_t *dz, int name_mod);

/**
 * Parse offsets, open dazibao, and launch extraction
 * @param argc number of argument(s)
 * @param argv arguments ([offset] DAZIBAO)
 * @return 0 on success, -1 on error
 **/
int cli_extract(int argc, char **argv);

/**
 * Print a dazibao (or a TLV list) on stdout
 * Helper for cli_dump_dz
 * Note that this function break dazibao type abstraction (meaning that
 * any change in dazibao structure in API will break this function).
 * @param dz dazibao to print
 * @param indent current indentation level
 * @param lvl remaining indention level allowed
 * @param debug flag to print or do not print pads
 * @return 0 on success, -1 on error
 * @see cli_dump_dz
 **/
int cli_print_dz(dz_t *dz, int indent, int lvl, int debug);

/**
 * Parse arguments and launching dazibao printing.
 * @param argc number of argument(s)
 * @param argv value(s) of argument(s)
 * @see cli_print_dz
 * @return 0 on success, -1 on error
 **/
int cli_dump_dz(int argc, char **argv);

/**
 * Compact a dazibao
 * @param file path of the dazibao to compact
 **/
int cli_compact_dz(char *file);

/**
 * Parse arg and launch TLV deletion
 * @param argc number of argument(s)
 * @param argv value(s) of argument(s)
 *        (OFFSET [OFFSET] DAZIBAO)
 * @return 0 on success, -1 on error
 */
int cli_rm_tlv(int argc, char **argv);
#endif
