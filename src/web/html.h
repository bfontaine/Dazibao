#ifndef _HTML_H
#define _HTML_H 1

#include "dazibao.h"

/**
 * Make an HTML representation of a TLV, NULL-terminated.
 **/
int tlv2html(dz_t dz, off_t off, char **html);

/**
 * Make an HTML representation of a dazibao, NULL-terminated.
 **/
int dz2html(dz_t dz, char **html);

#endif
