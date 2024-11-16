//
// Created by katharsis on 5/11/24.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include "encryption.h"

//Alice secret key
unsigned char alicesk[32] = {
 0x77,0x07,0x6d,0x0a,0x73,0x18,0xa5,0x7d
,0x3c,0x16,0xc1,0x72,0x51,0xb2,0x66,0x45
,0xdf,0x4c,0x2f,0x87,0xeb,0xc0,0x99,0x2a
,0xb1,0x77,0xfb,0xa5,0x1d,0xb9,0x2c,0x2a
};
//Alice public key
unsigned char alicepk[32] = {
 0x85,0x20,0xf0,0x09,0x89,0x30,0xa7,0x54
,0x74,0x8b,0x7d,0xdc,0xb4,0x3e,0xf7,0x5a
,0x0d,0xbf,0x3a,0x0d,0x26,0x38,0x1a,0xf4
,0xeb,0xa4,0xa9,0x8e,0xaa,0x9b,0x4e,0x6a
} ;

//Bob secret key
unsigned char bobsk[32] = {
 0x5d,0xab,0x08,0x7e,0x62,0x4a,0x8a,0x4b
,0x79,0xe1,0x7f,0x8b,0x83,0x80,0x0e,0xe6
,0x6f,0x3b,0xb1,0x29,0x26,0x18,0xb6,0xfd
,0x1c,0x2f,0x8b,0x27,0xff,0x88,0xe0,0xeb
} ;
//Bob public key
unsigned char bobpk[32] = {
 0xde,0x9e,0xdb,0x7d,0x7b,0x7d,0xc1,0xb4
,0xd3,0x5b,0x61,0xc2,0xec,0xe4,0x35,0x37
,0x3f,0x83,0x43,0xc8,0x5b,0x78,0x67,0x4d
,0xad,0xfc,0x7e,0x14,0x6f,0x88,0x2b,0x4f
} ;

// Helper function to read data from a file into a byte array
unsigned char* read_file(const char *filename, size_t *file_size) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *data = (unsigned char *)malloc(*file_size);
    if (!data) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    fread(data, 1, *file_size, file);
    fclose(file);
    return data;
}


int main() {
  unsigned char alice_shared_secret[32];
  unsigned char bob_shared_secret[32];

  // Generate shared secrets
  generate_shared_secret(alice_shared_secret, alicesk, bobpk);
  generate_shared_secret(bob_shared_secret, bobsk, alicepk);

  // Derive symmetric keys
  unsigned char alice_key[32], bob_key[32];
  derive_symmetric_key(alice_key, alice_shared_secret);
  derive_symmetric_key(bob_key, bob_shared_secret);

  
  // Read data from a file
  size_t file_size;
  unsigned char *file_data = read_file("/home/katharsis/Documents/GitHub/occurrence_counter_driver/Encryption/hola.txt", &file_size);
  if (!file_data) return 1;

  // Message to encrypt
  unsigned char ciphertext[1024];
  unsigned char iv[12];
  unsigned char tag[16];
  int ciphertext_len;
  
  //encrypt the file data (unsigned char* file_data:) 
  encrypt_message(alice_key, file_data, file_size, iv, tag, ciphertext, &ciphertext_len);
  printf("Data encrypted successfully.\n");

// Decrypt the data
    unsigned char decrypted[1024];
    if (decrypt_message(bob_key, iv, tag, ciphertext, ciphertext_len, decrypted)) {
        printf("Decrypted data: %s\n", decrypted);
    } else {
        printf("Decryption failed: authentication error\n");
    }

    free(file_data);
    return 0;
}
