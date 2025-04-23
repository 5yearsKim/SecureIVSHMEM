// crypto.h
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define KEY_LEN      16                   /* AES-128 */
#define IV_LEN       12                   /* GCM recommended */
#define TAG_LEN      16

// returns ciphertext length (or –1 on error)
int aes_gcm_encrypt(const uint8_t *plaintext, int plaintext_len,
                    const uint8_t *aad, int aad_len,
                    const uint8_t *key,
                    const uint8_t *iv, int iv_len,
                    uint8_t *ciphertext,
                    uint8_t *tag);

// returns plaintext length, or –1 if tag mismatch
int aes_gcm_decrypt(const uint8_t *ciphertext, int ciphertext_len,
                    const uint8_t *aad, int aad_len,
                    const uint8_t *tag,
                    const uint8_t *key,
                    const uint8_t *iv, int iv_len,
                    uint8_t *plaintext);
