/*--------------------------------*-C-*--------------------------------------*
 * File:        utf8.c
 *---------------------------------------------------------------------------*
 * Code borrowed/stolen from `glibc-2.1.3/iconv/gconv_simple.c':
 * - Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
 * - This file is part of the GNU C Library.
 * - Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.
 * - Adapted for rxvt by Xianping Ge <xge@ics.uci.edu>, 2000.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *--------------------------------------------------------------------------*/
/*
 * Convert between UTF-8 string and UCS2 string formats.
 */
#include <stdio.h>

typedef unsigned long	uint32_t;
typedef unsigned short	u_int16_t;
#define REALLOC	realloc

u_int16_t      *
utf8_to_ucs2(const unsigned char *str, int len, int *ucs2_len)
{
    static u_int16_t *ucs2_str = NULL;
    static int      max_len_ucs2_str = 0;
    static uint32_t ch;
    static int      cnt = 0;
    int             i_str, i_ucs2_str;

    if (len == 0)
        return NULL;
    if (max_len_ucs2_str < len + 1) {
        max_len_ucs2_str = len + 1;
        ucs2_str = (u_int16_t *)REALLOC(ucs2_str, (sizeof(u_int16_t) * max_len_ucs2_str));
    }

    for (i_ucs2_str = 0, i_str = 0; i_str < len; i_str++) {
        if (cnt > 0) {
            uint32_t        byte = str[i_str];

            if ((byte & 0xc0) != 0x80) {
                i_str--;
                cnt = 0;
            } else {
                ch <<= 6;
                ch |= byte & 0x3f;
                if (--cnt == 0) {
                    ucs2_str[i_ucs2_str++] = ch;
                }
            }
        } else {
            ch = str[i_str];
            if (ch < 0x80) {
                /* One byte sequence.  */
                ucs2_str[i_ucs2_str++] = ch;
            } else {
                if (ch >= 0xc2 && ch < 0xe0) {
                    /* We expect two bytes.  The first byte cannot be 0xc0
                     * or 0xc1, otherwise the wide character could have been
                     * represented using a single byte.  */
                    cnt = 2;
                    ch &= 0x1f;
                } else if ((ch & 0xf0) == 0xe0) {
                    /* We expect three bytes.  */
                    cnt = 3;
                    ch &= 0x0f;
                } else if ((ch & 0xf8) == 0xf0) {
                    /* We expect four bytes.  */
                    cnt = 4;
                    ch &= 0x07;
                } else if ((ch & 0xfc) == 0xf8) {
                    /* We expect five bytes.  */
                    cnt = 5;
                    ch &= 0x03;
                } else if ((ch & 0xfe) == 0xfc) {
                    /* We expect six bytes.  */
                    cnt = 6;
                    ch &= 0x01;
                } else {
                    cnt = 1;
                }
                --cnt;
            }
        }
    }

    *ucs2_len = i_ucs2_str;
    return ucs2_str;
}

static const uint32_t encoding_mask[] = {
    ~0x7ff, ~0xffff, ~0x1fffff, ~0x3ffffff
};

static const unsigned char encoding_byte[] = {
    0xc0, 0xe0, 0xf0, 0xf8, 0xfc
};

unsigned char  *
ucs2_to_utf8(u_int16_t * ucs2_str, int len)
{
    static unsigned char *utf8 = NULL;
    static int      max_len_utf8 = 0;
    unsigned char  *outptr;
    int             i;

    if (max_len_utf8 < len * 6 + 1) {
        max_len_utf8 = len * 6 + 1;
        utf8 = (unsigned char *)REALLOC(utf8, max_len_utf8);
    }
    outptr = utf8;
    for (i = 0; i < len; i++) {
        uint32_t        wc1 = ucs2_str[i];
        uint32_t        wc = ((wc1&0xff)<<8) | (wc1>>8);

        if (wc < 0x80)
            /* It's an one byte sequence.  */
            *outptr++ = (unsigned char)wc;
        else {
            size_t          step;
            unsigned char           *start;

            for (step = 2; step < 6; ++step)
                if ((wc & encoding_mask[step - 2]) == 0)
                    break;

            start = outptr;
            *outptr = encoding_byte[step - 2];
            outptr += step;
            --step;
            do {
                start[step] = 0x80 | (wc & 0x3f);
                wc >>= 6;
            } while (--step > 0);
            start[0] |= wc;
        }
    }
    *outptr = '\0';
    return utf8;
}
