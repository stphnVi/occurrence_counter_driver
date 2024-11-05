//
// Created by katharsis on 5/11/24.
//

#ifndef FIELDARITHMETIC_H
#define FIELDARITHMETIC_H

typedef unsigned char u8;
typedef long long i64; //signed 64bits for easier calculation
typedef i64 field_element[16]; //16 elements

//Little endian

//Addition, substraction and multiplication of modulo P.

static void unpackCurve25519(field_element result, const u8* in);
static void carryCurve25519(field_element element);

static void fieldAdd(field_element result, const field_element a, const field_element b);

static void fieldSub(field_element result, const field_element a, const field_element b);

static void fieldMult(field_element result, const field_element a, const field_element b);

#endif //FIELDARITHMETIC_H
