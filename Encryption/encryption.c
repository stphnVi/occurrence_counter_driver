#include "encryption.h"
//ya no se usa
void randombytes(unsigned char *buf, unsigned long long length)
{
}


/*Funcion para imprimir la llave*/
void printkey(unsigned char *buf)
{
  for (int i = 0;i < 32;++i) {
    if (i > 0) printf(","); else printf(" ");
    printf("0x%02x",(unsigned int) buf[i]);
    if (i % 8 == 7) printf("\n");
  }
}




/***
 * CURVE 25519 UTILITIES
 ***/

/*Converts a number from the byte array
 *representation (Z_p) to the field_element representation
 *result E [0, 2^255-1]
 */
static void unpackCurve25519(field_elem out, const u8 *in)
{
  int i;
  for (i = 0; i < 16; ++i) out[i] = in[2*i] + ((i64) in[2*i + 1] << 8);
  out[15] &= 0x7fff;
}
/*
 * Brings all the field_element values within the range of [0, 2^16 − 1]
 */
static void carryCurve25519(field_elem elem)
{
  int i;
  i64 carry;
  for (i = 0; i < 16; ++i) {
    carry = elem[i] >> 16;
    elem[i] -= carry << 16;
    if (i < 15) elem[i + 1] += carry; else elem[0] += 38 * carry;
  }
}


/*
 *If bit is 1: swaps the content of p and q
* If bit is 0: does nothing
*
* bit could be secret
*/
static void swapCurve25519(field_elem p, field_elem q, int bit)
{
  i64 t, i, c = ~(bit - 1);
  for (i = 0; i < 16; ++i) {
    t = c & (p[i] ^ q[i]);
    p[i] ^= t;
    q[i] ^= t;
  }
}
/*
 * Converts from field_element representation back to byte array
 *
 */
static void packCurve25519(u8 *result, const field_elem in)
{
  int i, j, carry;
  field_elem m, t;
  for (i = 0; i < 16; ++i) t[i] = in[i];
  carryCurve25519(t); carryCurve25519(t); carryCurve25519(t);

  //Calculate t − p and t − 2p, and then return accordingly either {t, t − p, t − 2p}
  for (j = 0; j < 2; ++j) {
    m[0] = t[0] - 0xffed;
    for(i = 1; i < 15; i++) {
      m[i] = t[i] - 0xffff - ((m[i-1] >> 16) & 1);
      m[i-1] &= 0xffff;
    }
    m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
    carry = (m[15] >> 16) & 1;
    m[14] &= 0xffff;
    swapCurve25519(t, m, 1 - carry);
  }
  for (i = 0; i < 16; ++i) {
    result[2*i] = t[i] & 0xff;
    result[2*i + 1] = t[i] >> 8;
  }
}
/***
 *FIELD ARITHMETIC OPERATIONS
 ***/

/* Adds each of the 16 elements separately
 * out = a + b
 */
static void fieldAdd(field_elem out, const field_elem a, const field_elem b)
{
  int i;
  for (i = 0; i < 16; ++i) out[i] = a[i] + b[i];
}

/* Subtracts each of the 16 elements separately
 * out = a - b
 */
static void fieldSub(field_elem out, const field_elem a, const field_elem b)
{
  int i;
  for (i = 0; i < 16; ++i) out[i] = a[i] - b[i];
}

/*
 * Multiplies two elements in the field_element form, the product is a
 * 31 element array of 64bit integers
 */
static void fieldMult(field_elem out, const field_elem a, const field_elem b) /* out = a * b */
{
  i64 i, j, product[31];
  for (i = 0; i < 31; ++i) product[i] = 0;
  for (i = 0; i < 16; ++i) {
    for (j = 0; j < 16; ++j) product[i+j] += a[i] * b[j];
  }
  for (i = 0; i < 15; ++i) product[i] += 38 * product[i+16];
  for (i = 0; i < 16; ++i) out[i] = product[i];
  carryCurve25519(out);
  carryCurve25519(out);
}
/*
 * (mod p) = 2^255 − 19 (Pequeño Teorema de Fermat)
 * Division b/a= inverse(a) * b
 */
static void fieldInverse(field_elem out, const field_elem in)
{
  field_elem c;
  int i;
  for (i = 0; i < 16; ++i) c[i] = in[i];
  for (i = 253; i >= 0; i--) {
    fieldMult(c,c,c);
    if (i != 2 && i != 4) fieldMult(c,c,in);
  }
  for (i = 0; i < 16; ++i) out[i] = c[i];
}

/***
 * X25519: DiffieHellman
 * /


/**
 * Multiplicacion scalar mediante Escalera de Montgomery
 ***/

void scalarmult(u8 *out, const u8 *scalar, const u8 *point)
{
  u8 clamped[32];
  i64 bit, i;
  field_elem a, b, c, d, e, f, x;
  for (i = 0; i < 32; ++i) clamped[i] = scalar[i];
  clamped[0] &= 0xf8;
  clamped[31] = (clamped[31] & 0x7f) | 0x40;
  unpackCurve25519(x, point);
  for (i = 0; i < 16; ++i) {
    b[i] = x[i];
    d[i] = a[i] = c[i] = 0;
  }
  a[0] = d[0] = 1;
  for (i = 254; i >= 0; --i) {
    bit = (clamped[i >> 3] >> (i & 7)) & 1;
    swapCurve25519(a, b, bit);
    swapCurve25519(c, d, bit);
    fieldAdd(e, a, c);
    fieldSub(a, a, c);
    fieldAdd(c, b, d);
    fieldSub(b, b, d);
    fieldMult(d, e, e);
    fieldMult(f, a, a);
    fieldMult(a, c, a);
    fieldMult(c, b, e);
    fieldAdd(e, a, c);
    fieldSub(a, a, c);
    fieldMult(b, a, a);
    fieldSub(c, d, f);
    fieldMult(a, c, _121665);
    fieldAdd(a, a, d);
    fieldMult(c, c, a);
    fieldMult(a, d, f);
    fieldMult(d, b, x);
    fieldMult(b, e, e);
    swapCurve25519(a, b, bit);
    swapCurve25519(c, d, bit);
  }
  fieldInverse(c, c);
  fieldMult(a, a, c);
  packCurve25519(out, a);
}
/**
 *Scalar multiplication with fixed group element _9
 ***/
void scalarmult_base(u8 *out, const u8 *scalar)
{
  scalarmult(out, scalar, _9);
}
/**
 *Generates a new pair of keys,
 *pKey: public key (obtained from scalar mult of sk with _9
 *sKey: private key (32bytes randomly gen)
 ***/
void generate_keypair(u8 *pk, u8 *sk)
{
  randombytes(sk, 32);
  scalarmult_base(pk, sk);
}

/**
 * If called by sender:
         pKey: recipient public key
         sKey: random integer gen by sender

   If called by recipient:
         pKey: group element sent with the message
         sKey: recipient private key
   For both:
        signature: shared secret

Signature can be used to encrypt a message
 ***/
void x25519(u8 *out, const u8 *pk, const u8 *sk)
{
  scalarmult(out, sk, pk);
}


//Diffie Hellman Key Exchange
void generate_shared_secret(unsigned char *shared_secret, const unsigned char *private_key, const unsigned char *public_key) {
  x25519(shared_secret, public_key, private_key);
}


//Function to derive a symmetric key using the shared secret (e.g., using HKDF or another KDF)
void derive_symmetric_key(unsigned char *symmetric_key, const unsigned char *shared_secret) {
  memcpy(symmetric_key, shared_secret, 32);
}

//Encryption
void encrypt_message(const unsigned char *key, const unsigned char *plaintext, size_t plaintext_len,
                    unsigned char *iv, unsigned char *tag,
                    unsigned char *ciphertext, int *ciphertext_len) {
    EVP_CIPHER_CTX *ctx;
    int len;

    // Generate random IV
    for(int i = 0; i < 12; i++) {  // 12 bytes for GCM IV
        iv[i] = i; 
    }

    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);

    // Encrypt the message
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    *ciphertext_len = len;

    // Finalize encryption
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    *ciphertext_len += len;

    // Get the tag
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);

    EVP_CIPHER_CTX_free(ctx);
}

//Decryption
int decrypt_message(const unsigned char *key, const unsigned char *iv,
                   const unsigned char *tag, const unsigned char *ciphertext,
                   int ciphertext_len, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);

    // Decrypt the message
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;

    // Set expected tag value
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)tag);

    // Finalize decryption: verify tag
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0) {
        // Add null terminator
        plaintext[plaintext_len] = '\0';
        return 1; // Success
    }
    return 0; // Verification failed
}

