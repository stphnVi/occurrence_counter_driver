//
// Created by katharsis on 5/11/24.
//

#ifndef PROTOCOLDH_H
#define PROTOCOLDH_H

typedef unsigned char u8;
typedef unsigned long long u64;
typedef long long i64;
typedef i64 field_element[16];
static const field_element _121665 = {0xDB41, 1};
static const u8 _9[32]= {9};//Punto de la curva cuyo x+9 (orden de gran primo)
extern void randombytes(u8* out, const u8* scalar);



void scalarMultBase(u8* result, const u8* scalar);
void scalarMult(u8* result, const u8* scalar, const u8* point);
void generateKeyPair(u8* pKey, u8* sKey);
void sign25519(u8* signature, const u8* pKey, const u8* sKey);


#endif //PROTOCOLDH_H
