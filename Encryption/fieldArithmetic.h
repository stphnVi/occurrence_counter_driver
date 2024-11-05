//
// Created by katharsis on 5/11/24.
//

#ifndef FIELDARITHMETIC_H
#define FIELDARITHMETIC_H

typedef unsigned char u8;
typedef long long i64; //signed 64bits for easier calculation
typedef i64 field_element[16]; //16 elements

//Little endian


static void unpackCurve25519(field_element result, const u8* in);
static void packCurve25519(u8* result, const field_element in);
static void carryCurve25519(field_element element);
static void swapCurve25519(field_element p, field_element q, int bit);

//Addition, substraction and multiplication of modulo P.
static void fieldAdd(field_element result, const field_element a, const field_element b);
static void fieldSub(field_element result, const field_element a, const field_element b);
static void fieldMult(field_element result, const field_element a, const field_element b);
static void fieldInverse(field_element result, const field_element in);

#endif //FIELDARITHMETIC_H
