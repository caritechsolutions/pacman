/*
 * RTMP network protocol
 * Copyright (c) 2009 Konstantin Shishkov
 *
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

/**
 * @file
 * RTMP protocol digest
 */

#include <stdint.h>
#if 0/*ff hmac*/
#include "../avutil/hmac.h"
#endif
#include "rtmp.h"

#ifdef CONFIG_DEMUX_TLS
#if defined(CONFIG_HAVE_WOLFSSL)
#include <cyassl/openssl/sha.h>
#include <cyassl/openssl/ssl.h>
#include <cyassl/openssl/hmac.h>
#include <cyassl/ctaocrypt/arc4.h>
typedef Arc4* RC4_handle;
#define RC4_alloc(h)	        *h = av_malloc(sizeof(Arc4))
#define RC4_setkey(h,l,k)	    Arc4SetKey(h,k,l)
#define RC4_encrypt(h,l,d)	    Arc4Process(h,(uint8_t *)d,(uint8_t *)d,l)
#define RC4_encrypt2(h,l,s,d)	Arc4Process(h,(uint8_t *)d,(uint8_t *)s,l)
#define RC4_free(h)	            av_free(h)
#elif defined(CONFIG_HAVE_OPENSSL)
#include <openssl/sha.h>
#include <openssl/hmac.h>
typedef void* RC4_handle;
#define RC4_alloc(h)	      gxloge("%s %d: not support\n", __func__, __LINE__)
#define RC4_setkey(h,l,k)	  gxloge("%s %d: not support\n", __func__, __LINE__)
#define RC4_encrypt(h,l,d)	  gxloge("%s %d: not support\n", __func__, __LINE__)
#define RC4_encrypt2(h,l,s,d) gxloge("%s %d: not support\n", __func__, __LINE__)
#define RC4_free(h)	          gxloge("%s %d: not support\n", __func__, __LINE__)
#endif

#define HMAC_setup(ctx, key, len)	HMAC_Init(&ctx, key, len, EVP_sha256())
#define HMAC_crunch(ctx, buf, len)	HMAC_Update(&ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen)	HMAC_Final(&ctx, dig, &dlen); HMAC_cleanup(&ctx)
#define HMAC_free(ctx)              HMAC_cleanup(&ctx)
#endif


int ff_rtmp_calc_digest(const uint8_t *src, int len, int gap,
                        const uint8_t *key, int keylen, uint8_t *dst)
{
#if 0/*ff hmac*/
    AVHMAC *hmac;

    hmac = av_hmac_alloc(AV_HMAC_SHA256);
    if (!hmac)
        return AVERROR(ENOMEM);

    av_hmac_init(hmac, key, keylen);
    if (gap <= 0) {
        av_hmac_update(hmac, src, len);
    } else { //skip 32 bytes used for storing digest
        av_hmac_update(hmac, src, gap);
        av_hmac_update(hmac, src + gap + 32, len - gap - 32);
    }
    av_hmac_final(hmac, dst, 32);

    av_hmac_free(hmac);
#else/*ssl hmac*/
#ifdef CONFIG_DEMUX_TLS
    HMAC_CTX hmac;
    unsigned int hmac_len = 32;

    HMAC_setup(hmac, key, keylen);

    if (gap <= 0) {
        HMAC_crunch(hmac, src, len);
    } else {
        HMAC_crunch(hmac, src, gap);
        HMAC_crunch(hmac, src + gap + 32, len - gap - 32);
    }
    HMAC_finish(hmac, dst, hmac_len);
    HMAC_free(hmac);
#endif
#endif
    return 0;
}

int ff_rtmp_calc_digest_pos(const uint8_t *buf, int off, int mod_val,
                            int add_val)
{
    int i, digest_pos = 0;

    for (i = 0; i < 4; i++)
        digest_pos += buf[i + off];
    digest_pos = digest_pos % mod_val + add_val;

    return digest_pos;
}
