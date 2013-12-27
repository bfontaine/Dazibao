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
 * @param tlv
 * @param argc
 * @param argv
 * @param type
 * @param date
 **/
int cli_mk_tlv(tlv_t *tlv, int argc, char **argv, char *type, char date);

/**
 * @param argc
 * @param argv
 **/
int cli_add(int argc, char **argv);

/**
 * @param dz
 * @param offset
 * @param name_mod
 **/
int cli_extract_tlv(dz_t *dz, off_t offset, int name_mod);

/**
 * @param dz
 * @param tlv
 * @param offset
 * @param name_mod
 **/
int cli_extract_ltlv(dz_t *dz, tlv_t *tlv, int offset, int name_mod);

/**
 * @param dz
 * @param name_mod
 **/
int cli_extract_all(dz_t *dz, int name_mod);

/**
 * @param argc
 * @param argv
 **/
int cli_extract(int argc, char **argv);

/**
 * @param dz
 * @param indent
 * @param lvl
 * @param debug
 **/
int cli_print_dz(dz_t *dz, int indent, int lvl, int debug);

/**
 * @param argc
 * @param argv
 * @param out
 **/
int cli_dump_dz(int argc, char **argv, int out);

/**
 * @param file
 **/
int cli_compact_dz(char *file);
#endif
