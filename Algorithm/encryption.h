
#ifndef CURVE25519_H
#define CURVE25519_H

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>

// Type definitions
typedef unsigned char u8;
typedef long long i64;
typedef i64 field_elem[16];

// Constants
static const u8 _9[32] = {9};
static const field_elem _121665 = {0xDB41, 1};

// Functions

// Diffie-Hellman key exchange
void generate_shared_secret(unsigned char *shared_secret, const unsigned char *private_key, const unsigned char *public_key);

// Key derivation function
void derive_symmetric_key(unsigned char *symmetric_key, const unsigned char *shared_secret);

// Encryption and decryption functions
void encrypt_message(const unsigned char *key, const unsigned char *plaintext, size_t plaintext_len, unsigned char *iv, unsigned char *tag, unsigned char *ciphertext, int *ciphertext_len);
int decrypt_message(const unsigned char *key, const unsigned char *iv, const unsigned char *tag, const unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext);

#endif // CURVE25519_H
