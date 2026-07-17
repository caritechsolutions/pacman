/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "avstring.h"

struct error_entry {
    int num;
    char *tag;
    char *str;
};
#define ERROR_TAG(tag) AVERROR_##tag, #tag
#define EERROR_TAG(tag) AVERROR(tag), #tag
#define AVERROR_INPUT_AND_OUTPUT_CHANGED (AVERROR_INPUT_CHANGED | AVERROR_OUTPUT_CHANGED)
struct error_entry error_entries[] = {
    { ERROR_TAG(BUG),                "Internal bug, should not have happened"         },
	{ ERROR_TAG(RSTRT),               "server probe. player send restart information" },
    { ERROR_TAG(DEMUXER_NOT_FOUND),  "Demuxer not found"                              },
    { ERROR_TAG(EOF),                "End of file"                                    },
    { ERROR_TAG(EXIT),               "Immediate exit requested"                       },
    { ERROR_TAG(INVALIDDATA),        "Invalid data found when processing input"       },
    { ERROR_TAG(OPTION_NOT_FOUND),   "Option not found"                               },
    { ERROR_TAG(PATCHWELCOME),       "Not yet implemented in FFmpeg, patches welcome" },
    { ERROR_TAG(PROTOCOL_NOT_FOUND), "Protocol not found"                             },
    { ERROR_TAG(STREAM_NOT_FOUND),   "Stream not found"                               },
    { ERROR_TAG(UNKNOWN),            "Unknown error occurred"                         },
    { ERROR_TAG(HTTP_BAD_REQUEST),   "Server returned 400 Bad Request"         },
    { ERROR_TAG(HTTP_UNAUTHORIZED),  "Server returned 401 Unauthorized (authorization failed)" },
    { ERROR_TAG(HTTP_FORBIDDEN),     "Server returned 403 Forbidden (access denied)" },
    { ERROR_TAG(HTTP_NOT_FOUND),     "Server returned 404 Not Found"           },
    { ERROR_TAG(HTTP_OTHER_4XX),     "Server returned 4XX Client Error, but not one of 40{0,1,3,4}" },
    { ERROR_TAG(HTTP_SERVER_ERROR),  "Server returned 5XX Server Error reply" },
    { EERROR_TAG(EACCES),            "Permission denied" },
    { EERROR_TAG(EAGAIN),            "Resource temporarily unavailable" },
    { EERROR_TAG(EBADF),             "Bad file descriptor" },
    { EERROR_TAG(EBUSY),             "Device or resource busy" },
    { EERROR_TAG(EDEADLK),           "Resource deadlock avoided" },
    { EERROR_TAG(EDOM),              "Numerical argument out of domain" },
    { EERROR_TAG(EEXIST),            "File exists" },
    { EERROR_TAG(EFAULT),            "Bad address" },
    { EERROR_TAG(EFBIG),             "File too large" },
    { EERROR_TAG(EINTR),             "Interrupted system call" },
    { EERROR_TAG(EINVAL),            "Invalid argument" },
    { EERROR_TAG(EIO),               "I/O error" },
    { EERROR_TAG(EISDIR),            "Is a directory" },
    { EERROR_TAG(EMFILE),            "Too many open files" },
    { EERROR_TAG(ENAMETOOLONG),      "File name too long" },
    { EERROR_TAG(ENFILE),            "Too many open files in system" },
    { EERROR_TAG(ENODEV),            "No such device" },
    { EERROR_TAG(ENOENT),            "No such file or directory" },
    { EERROR_TAG(ENOMEM),            "Cannot allocate memory" },
    { EERROR_TAG(ENOSPC),            "No space left on device" },
    { EERROR_TAG(ENOSYS),            "Function not implemented" },
    { EERROR_TAG(ENOTDIR),           "Not a directory" },
    { EERROR_TAG(ENOTEMPTY),         "Directory not empty" },
    { EERROR_TAG(ENOTTY),            "Inappropriate I/O control operation" },
    { EERROR_TAG(ENXIO),             "No such device or address" },
    { EERROR_TAG(EPERM),             "Operation not permitted" },
    { EERROR_TAG(EPIPE),             "Broken pipe" },
    { EERROR_TAG(ERANGE),            "Result too large" },
    { EERROR_TAG(EROFS),             "Read-only file system" },
    { EERROR_TAG(ESPIPE),            "Illegal seek" },
    { EERROR_TAG(ESRCH),             "No such process" },
    { EERROR_TAG(EXDEV),             "Cross-device link" },
    { ERROR_TAG(NEXT),               "next" },
};

static int av_strerror(int errnum, char *errbuf, size_t errbuf_size)
{
    int ret = 0, i;
    const struct error_entry *entry = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(error_entries); i++) {
        if (errnum == error_entries[i].num) {
            entry = &error_entries[i];
            break;
        }
    }
    if (entry) {
        av_strlcpy(errbuf, entry->str, errbuf_size);
    } else {
        ret = -1;
    }
    return ret;
}

char *av_make_error_string(char *errbuf, size_t errbuf_size, int errnum)
{
    av_strerror(errnum, errbuf, errbuf_size);
    return errbuf;
}

