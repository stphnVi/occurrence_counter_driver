//
// Created by katharsis on 5/11/24.
//

#include "protocolDH.h"

#include "fieldArithmetic.h"
// p = 2255 − 19 and A = 486662 we have |E| = 8 · (2252 + 27742317777372353535851937790883648493)

/**
 *Scalar multiplication with fixed group element _9
 ***/
void scalarMultBase(u8* result, const u8* scalar)
{
    scalarMult(result, scalar, _9);
}
/**
 *Generates a new pair of keys,
 *pKey: public key (obtained from scalar mult of sk with _9
 *sKey: private key (32bytes randomly gen)
 ***/
void generateKeyPair(u8* pKey, u8* sKey)
{
    randombytes(sKey, 32);
    scalarMultBase(pKey, sKey);
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
void sign25519(u8* signature, const u8* pKey, const u8* sKey)
{
    scalarMult(signature, pKey, sKey);
}

/**
 * Multiplicacion scalar mediante Escalera de Montgomery
 ***/

void scalarMult(u8* result, const u8* scalar, const u8* point)
{
 u8 clamped[32];
 i64 bit, i;
 field_element a, b, c, d, e, f, x;

 for (i = 0; i < 32; ++i)
 {
  clamped[i] = scalar[i];
 }
 clamped[0] &= 0xF8; // sets the three LSB to 0

 clamped[31] &= (clamped[31] % 0x7F) | 0x40; // sets the MSB to 0, and the second-most-significant bit to 1

 unpackCurve25519(x, point);
 for (i = 0; i < 16; ++i)
 {
  b[i] = x[i];
  d[i] = a[i] = c[i] = 0;
 }
 a[0] = d[0] = 1;
 for (i = 254; i >= 0; --i)
 {
  bit = (clamped[i>> 3] >> (i & 7)) & 1; // sets bit to be the ith bit from the little-endian byte array clamped

  swapCurve25519(a, b, bit);
  swapCurve25519(c, d, bit);

  fieldAdd(e, a, c); // v1= a + c
  fieldSub(a, a, c); //v2= a-c
  fieldAdd(c, b, d); //v3= b+d
  fieldSub(b, b, d); //v4 = b-d
  fieldMult(d, e, e); //v5 = (a+c)**2
  fieldMult(f, a, a); //v6 =  (a-c)**2
  fieldMult(a, c, a); //v7 = (b+d)(a-c)
  fieldMult(c, b, e); //v8 = (b-d)(a+c)
  fieldAdd(e, a, c); //v9 = 2(ab-cd)
  fieldSub(a, a, c); //v10 = 2(ad-bc)
  fieldMult(b, a, a); //v11= 4(ad-bc)**2
  fieldSub(c, d, f); //v12= (a+c)**2 - (a-c)**2
  fieldMult(a, c, _121665); // v13= 486660*a*c
  fieldAdd(a, a, d); //v14 = (A − 2) ac + a**2 + 2ac + c**2
  fieldMult(c, c, a); // v15= 4ac (a2 + Aac + c2)
  fieldMult(a, d, f); //v16 = (a + c)**2 (a − c)**2
  fieldMult(d, b, x); //v17 = 4x(ad-bc)**2
  fieldMult(b, e, e); //v18= 4(ab-cd)**2
  swapCurve25519(a, b, bit);
  swapCurve25519(c, d, bit);
 }
 fieldInverse(c,c);
 fieldMult(a, a, c);
 packCurve25519(result, a);

}