#include "ivshmem_crypto.h"


#define KEY_LEN    16   // AES-128
#define IV_LEN     12   // recommended for GCM
#define TAG_LEN    16

// Encrypt plaintext -> ciphertext (+ tag)
int aes_gcm_encrypt(const uint8_t *plaintext, int plaintext_len,
                    const uint8_t *aad, int aad_len,
                    const uint8_t *key,
                    const uint8_t *iv, int iv_len,
                    uint8_t *ciphertext,
                    uint8_t *tag)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, ciphertext_len;

    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);

    // AAD (optional)
    if (aad && aad_len)
        EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len);

    // Encrypt plaintext
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;

    // Finalize
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    // Get tag
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

// Decrypt ciphertext + tag -> plaintext
int aes_gcm_decrypt(const uint8_t *ciphertext, int ciphertext_len,
                    const uint8_t *aad, int aad_len,
                    const uint8_t *tag,
                    const uint8_t *key,
                    const uint8_t *iv, int iv_len,
                    uint8_t *plaintext)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, ret, plaintext_len;

    EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);

    // AAD (must be same as during encryption)
    if (aad && aad_len)
        EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len);

    // Decrypt ciphertext
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;

    // Set expected tag value
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, (void *)tag);

    // Finalize: returns 0 if tag verification fails
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0) {
        plaintext_len += len;
        return plaintext_len;
    } else {
        // verification failed
        return -1;
    }
}

