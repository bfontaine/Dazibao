#ifndef _MIME_H
#define _MIME_H 1

/** @file
 * Utilities to work with MIME types
 **/

/* Add most used types at the top for a faster lookup
 *
 * List from:
 *  en.wikipedia.org/wiki/Internet_media_type#List_of_common_media_types
 **/
/** Known mime types **/
static const char *mime_types_ext[][2] = {

        /* common types */
        { "html" , "text/html; charset=utf-8" },

        /* TLVs */
        { "bmp"  , "image/bmp"                },
        { "dib"  , "image/bmp"                },
        { "jpeg" , "image/jpeg"               },
        { "jpg"  , "image/jpeg"               },
        { "png"  , "image/png"                },

        /* other common types */
        { "css"  , "text/css; charset=utf-8"  },
        { "js"   , "application/javascript"   },

        /* other types */
        { "csv"  , "text/csv"                 },
        { "gif"  , "image/gif"                },
        { "gzip" , "application/gzip"         },
        { "json" , "application/json"         },
        { "pdf"  , "application/pdf"          },
        { "rss"  , "application/rss+xml"      },
        { "svg"  , "image/svg+xml"            },
        { "tiff" , "image/tiff"               },
        { "xml"  , "application/xml"          },
        { "zip"  , "application/zip"          },

        { "md"   , "text/plain"               }, /* Markdown */
        { "txt"  , "text/plain"               }
};

/** Number of registered MIME types **/
#define MIME_TYPES_COUNT (sizeof(mime_types_ext)/sizeof(char*[2]))

/**
 * Return the mime type for a file from its extension, or NULL if it cannot be
 * determined. The pointer is statically allocated, strdup it if necessary.
 * @param path the path of the file
 * @return the mime style
 **/
const char *get_mime_type(const char *path);

#endif
