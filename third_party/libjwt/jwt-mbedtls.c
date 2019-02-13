/* Copyright (C) 2015-2018 Ben Collins <ben@cyphre.com>
   This file is part of the JWT C Library
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <alloca.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/entropy_poll.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

#include <jwt.h>

#include "config.h"
#include "jwt-private.h"
#include "utils/entropy_utils.h"

#define SHA256_OUT_SIZE (32)
#define SHA384_OUT_SIZE (48)
#define SHA512_OUT_SIZE (64)

#define RSA_HASH_BUF_SIZE (256)
#define EC_MAX_SIG_SIZE (256)

int jwt_sign_sha_hmac(jwt_t *jwt, char **out, unsigned int *len, const char *str)
{
    int               out_size;
    mbedtls_md_type_t md_type;

    switch (jwt->alg)
    {
    /* HMAC */
    case JWT_ALG_HS256:
        out_size = SHA256_OUT_SIZE;
        md_type  = MBEDTLS_MD_SHA256;
        break;
    case JWT_ALG_HS384:
        out_size = SHA384_OUT_SIZE;
        md_type  = MBEDTLS_MD_SHA384;
        break;
    case JWT_ALG_HS512:
        out_size = SHA512_OUT_SIZE;
        md_type  = MBEDTLS_MD_SHA512;
        break;
    default:
        return EINVAL;
    }

    *out = malloc(out_size);
    if (*out == NULL)
        return ENOMEM;

    mbedtls_md_hmac(mbedtls_md_info_from_type(md_type), jwt->key, jwt->key_len, (const unsigned char *)str, strlen(str),
                    (unsigned char *)*out);
    *len = out_size;

    return 0;
}

int jwt_verify_sha_hmac(jwt_t *jwt, const char *head, const char *sig)
{
    char *       sig_check, *buf = NULL;
    unsigned int len;
    int          ret = EINVAL;

    if (!jwt_sign_sha_hmac(jwt, &sig_check, &len, head))
    {
        size_t buf_len = len * 2;
        size_t base64_len;
        buf = alloca(len * 2);
        mbedtls_base64_encode((unsigned char *)buf, base64_len, &base64_len, (unsigned char *)sig_check, len);
        jwt_base64uri_encode(buf);

        if (!strcmp(sig, buf))
            ret = 0;

        free(sig_check);
    }

    return ret;
}

static int decode_der_to_rs(const unsigned char *sig, unsigned char **rs, unsigned int *len)
{
    // a x509 encoded EC signature has format:
    // | 0x30 | len(seq) | 0x02 | len(r) | r...... | 0x02 | len(s) | s..... |
    // refer to this structure for the meaning of follwing numbers
    uint8_t              r_size = sig[3];
    uint8_t              s_size = sig[5 + r_size];
    const unsigned char *sig_r  = &sig[4];
    const unsigned char *sig_s  = &sig[6 + r_size];
    uint8_t              adj;
    uint8_t              r_padding = 0, r_out_padding = 0;
    uint8_t              s_padding = 0, s_out_padding = 0;

    /* Check r and s size */
    if (JWT_ALG_ES256)
        adj = 32;
    if (JWT_ALG_ES384)
        adj = 48;
    if (JWT_ALG_ES512)
        adj = 66;

    if (r_size > adj)
        r_padding = r_size - adj;
    else if (r_size < adj)
        r_out_padding = adj - r_size;

    if (s_size > adj)
        s_padding = s_size - adj;
    else if (s_size < adj)
        s_out_padding = adj - s_size;

    *len = adj << 1;

    *rs = malloc(*len);
    if (*rs == NULL)
    {
        return ENOMEM;
    }
    memset(*rs, 0, *len);

    memcpy(*rs + r_out_padding, sig_r + r_padding, r_size - r_padding);
    memcpy(*rs + (r_size - r_padding + r_out_padding) + s_out_padding, sig_s + s_padding, (s_size - s_padding));

    return 0;
}

static int encode_rs_to_der(unsigned char *      sig,
                            size_t *             sig_len,
                            const unsigned char *r,
                            size_t               r_size,
                            const unsigned char *s,
                            size_t               s_size)
{
    // a x509 encoded EC signature has format:
    // | 0x30 | len(seq) | 0x02 | len(r) | r...... | 0x02 | len(s) | s..... |

    // remove leading zeros except for the leading 0 for "negative" first byte
    int r_data_offset = 0, s_data_offset = 0;
    while ((*r) == 0)
    {
        r++;
        r_size--;
    }
    if ((*r) & 0x80)
    {
        r_data_offset++;
        r_size++;
    }
    while ((*s) == 0)
    {
        s++;
        s_size--;
    }
    if ((*s) & 0x80)
    {
        s_data_offset++;
        s_size++;
    }

    sig[0] = 0x30;
    sig[1] = r_size + s_size + 4;
    sig[2] = 0x02;
    sig[3] = r_size;

    memcpy(&sig[4 + r_data_offset], r, r_size);
    sig[r_size + 4] = 0x02;
    sig[r_size + 5] = s_size;
    memcpy(&sig[r_size + 6 + s_data_offset], s, s_size);
    *sig_len = r_size + s_size + 6;
    return 0;
}

int jwt_sign_sha_pem(jwt_t *jwt, char **out, unsigned int *len, const char *str)
{
    int                      ret;
    mbedtls_pk_context       pk;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_type_t        pk_type;
    mbedtls_md_type_t        md_type;
    unsigned char            hash[RSA_HASH_BUF_SIZE];
    unsigned char            out_buf[MBEDTLS_MPI_MAX_SIZE];
    size_t                   out_size;
    const unsigned char *    pers     = (const unsigned char *)"jwt";
    size_t                   pers_len = 3;

    mbedtls_entropy_init(&entropy);
    mbedtls_entropy_add_source(&entropy, otrMbedtlsEntropyPoll, NULL, MBEDTLS_ENTROPY_MIN_PLATFORM,
                               MBEDTLS_ENTROPY_SOURCE_STRONG);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_pk_init(&pk);

    switch (jwt->alg)
    {
    /* RSA */
    case JWT_ALG_RS256:
        md_type = MBEDTLS_MD_SHA256;
        pk_type = MBEDTLS_PK_RSA;
        break;
    case JWT_ALG_RS384:
        md_type = MBEDTLS_MD_SHA384;
        pk_type = MBEDTLS_PK_RSA;
        break;
    case JWT_ALG_RS512:
        md_type = MBEDTLS_MD_SHA512;
        pk_type = MBEDTLS_PK_RSA;
        break;

    /* ECC */
    case JWT_ALG_ES256:
        md_type = MBEDTLS_MD_SHA256;
        pk_type = MBEDTLS_PK_ECKEY;
        break;
    case JWT_ALG_ES384:
        md_type = MBEDTLS_MD_SHA384;
        pk_type = MBEDTLS_PK_ECKEY;
        break;
    case JWT_ALG_ES512:
        md_type = MBEDTLS_MD_SHA512;
        pk_type = MBEDTLS_PK_ECKEY;
        break;

    default:
        ret = EINVAL;
        goto exit;
    }

    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, pers, pers_len) != 0)
    {
        ret = EINVAL;
        goto exit;
    }

    if (mbedtls_pk_parse_key(&pk, jwt->key, strlen(jwt->key) + 1, NULL, 0) != 0)
    {
        ret = EINVAL;
        goto exit;
    }

    if (pk_type != mbedtls_pk_get_type(&pk))
    {
        ret = EINVAL;
        goto exit;
    }

    if (mbedtls_md(mbedtls_md_info_from_type(md_type), (const unsigned char *)str, strlen(str), hash) != 0)
    {
        ret = EINVAL;
        goto exit;
    }

    if (mbedtls_pk_sign(&pk, md_type, hash, 0, out_buf, &out_size, mbedtls_ctr_drbg_random, &ctr_drbg) != 0)
    {
        ret = EINVAL;
        goto exit;
    }

    if (pk_type == MBEDTLS_PK_RSA)
    {
        *out = malloc(out_size);
        memcpy(*out, out_buf, out_size);
        *len = out_size;
        ret  = 0;
    }
    else
    {
        ret = decode_der_to_rs(out_buf, (unsigned char **)out, len);
    }
exit:
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_pk_free(&pk);
    return ret;
}

int jwt_verify_sha_pem(jwt_t *jwt, const char *head, const char *sig_b64)
{
    int                ret;
    unsigned char *    sig = NULL;
    int                sig_len;
    mbedtls_pk_context pk;
    mbedtls_pk_type_t  pk_type;
    mbedtls_md_type_t  md_type;
    unsigned char      hash[RSA_HASH_BUF_SIZE];

    mbedtls_pk_init(&pk);

    switch (jwt->alg)
    {
    /* RSA */
    case JWT_ALG_RS256:
        md_type = MBEDTLS_MD_SHA256;
        pk_type = MBEDTLS_PK_RSA;
        break;
    case JWT_ALG_RS384:
        md_type = MBEDTLS_MD_SHA384;
        pk_type = MBEDTLS_PK_RSA;
        break;
    case JWT_ALG_RS512:
        md_type = MBEDTLS_MD_SHA512;
        pk_type = MBEDTLS_PK_RSA;
        break;

    /* ECC */
    case JWT_ALG_ES256:
        md_type = MBEDTLS_MD_SHA256;
        pk_type = MBEDTLS_PK_ECKEY;
        break;
    case JWT_ALG_ES384:
        md_type = MBEDTLS_MD_SHA384;
        pk_type = MBEDTLS_PK_ECKEY;
        break;
    case JWT_ALG_ES512:
        md_type = MBEDTLS_MD_SHA512;
        pk_type = MBEDTLS_PK_ECKEY;
        break;

    default:
        ret = EINVAL;
        goto exit;
    }

    sig = (unsigned char *)jwt_b64_decode(sig_b64, &sig_len);

    if (sig == NULL)
    {
        ret = EINVAL;
        goto exit;
    }

    if (mbedtls_pk_parse_public_key(&pk, jwt->key, strlen(jwt->key) + 1) != 0)
    {
        ret = EINVAL;
        goto exit;
    }

    if (mbedtls_md(mbedtls_md_info_from_type(md_type), (const unsigned char *)head, strlen(head), hash) != 0)
    {
        ret = EINVAL;
        goto exit_freesig;
    }

    if (pk_type == MBEDTLS_PK_RSA)
    {
        if (mbedtls_pk_verify(&pk, md_type, hash, 0, sig, sig_len) != 0)
        {
            ret = EINVAL;
            goto exit_freesig;
        }
    }
    else
    {
        unsigned char  ec_sig_buf[EC_MAX_SIG_SIZE];
        size_t         ec_sig_len, r_size, s_size;
        unsigned char *r;
        unsigned char *s;

        if (sig_len == 64)
        {
            r_size = 32;
            r      = sig;
            s_size = 32;
            s      = sig + 32;
        }
        else if (sig_len == 96)
        {
            r_size = 48;
            r      = sig;
            s_size = 48;
            s      = sig + 48;
        }
        else if (sig_len == 132)
        {
            r_size = 66;
            r      = sig;
            s_size = 66;
            s      = sig + 66;
        }
        else
        {
            ret = EINVAL;
            goto exit_freesig;
        }

        encode_rs_to_der(ec_sig_buf, &ec_sig_len, r, r_size, s, s_size);

        if (mbedtls_pk_verify(&pk, md_type, hash, 0, ec_sig_buf, ec_sig_len) != 0)
        {
            ret = EINVAL;
            goto exit_freesig;
        }
    }

    ret = 0;
exit_freesig:
    free(sig);
exit:
    return ret;
}
