#ifndef _MIME_H
#define _MIME_H 1

#include "tlv.h"

/** @file
 * Utilities to work with MIME types
 **/

/** association between a TLV type and a MIME type */
struct tlv_mime_type {
        /** TLV type */
        unsigned char type;
        /** extension used for this type */
        char *ext;
        /** MIME type */
        char *mime;
};

/* Add most used types at the top for a faster lookup
 *
 * List from:
 *  en.wikipedia.org/wiki/Internet_media_type#List_of_common_media_types
 **/
/** Known mime types **/
static const struct tlv_mime_type tlv_mime_types[] = {

        /* common types */
        { -1      , "html" , "text/html; charset=utf-8" },

        /*000,  TLVs */
        { TLV_BMP , "bmp"  , "image/bmp"                },
        { TLV_JPEG, "jpeg" , "image/jpeg"               },
        { TLV_JPEG, "jpg"  , "image/jpeg"               },
        { TLV_PNG , "png"  , "image/png"                },
        { TLV_MP3 , "mp3"  , "audio/mpeg"               },
        { TLV_MP4 , "mp4"  , "audio/mp4"                },
        { TLV_OGG , "ogg"  , "audio/ogg"                },
        { TLV_GIF , "gif"  , "image/gif"                },
        { TLV_TIFF, "tiff" , "image/tiff"               },
        { TLV_PDF , "pdf"  , "application/pdf"          },
        { TLV_TEXT, "txt"  , "text/plain"               },

        /*000,  other common types */
        { -1      , "css"  , "text/css; charset=utf-8"  },
        { -1      , "js"   , "application/javascript"   },

        /*000,  other types */
        { -1      , "csv"  , "text/csv"                 },
        { -1      , "gzip" , "application/gzip"         },
        { -1      , "json" , "application/json"         },
        { -1      , "rss"  , "application/rss+xml"      },
        { -1      , "svg"  , "image/svg+xml"            },
        { -1      , "xml"  , "application/xml"          },
        { -1      , "zip"  , "application/zip"          },

        { -1      , "md"   , "text/plain"               } /* Markdown */
};

/** Number of registered MIME types **/
#define MIME_TYPES_COUNT (sizeof(tlv_mime_types)/sizeof(struct tlv_mime_type))

/**
 * Return the mime type for a file from its extension, or NULL if it cannot be
 * determined. The pointer is statically allocated, strdup it if necessary.
 * @param path the path of the file
 * @return the mime type
 **/
const char *get_mime_type_from_path(const char *path);

/**
 * Return the mime type for a TLV from its type, or NULL if it cannot be
 * determined. The pointer is statically allocated, strdup it if necessary.
 * @param type the TLV type
 * @return the mime type
 **/
const char *get_mime_type_from_tlv(unsigned char type);

#endif
