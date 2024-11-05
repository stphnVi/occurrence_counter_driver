//
// Created by katharsis on 1/11/24.
//

#include "fieldArithmetic.h"

/***
 * CURVE 25519 UTILITIES
 ***/


/*
 * Converts from field_element representation back to byte array
 *
 */
static void packCurve25519(u8* result, const field_element in)
{
    int i;
    field_element m, t;

    for (i=0; i<16; ++i)
    {
        carryCurve25519(t);
        carryCurve25519(t);
        carryCurve25519(t);
    }
    //Calculate t − p and t − 2p, and then return accordingly either {t, t − p, t − 2p}
    for (int j = 0; j<2; ++j)
    {
        m[0] = t[0]- 0xFFED; //substracts LSB 16bits of p
        for (i=1; i<15; i++)
        {
            m[i] = t[i]- 0xFFFF - ((m[i-1]>>16) & 1); //0xFFFF midle bits of p
            m[i-1] &= 0xFFFF; //ensures it is within the range [0, 2^16 − 1]
        }
        m[15]=t[15] - 0x7FFF - ((m[14]>>16) & 1); //0x7FFF MSB 16bits of p
        int carry = (m[15] >> 16) & 1;
        m[14] &= 0xFFFF;
        swapCurve25519(t,m, 1- carry);
    }
    //Each of the 16-bit elements is split into
    //two bytes and assigned to two adjacent elements of the byte array
    for (i=0; i<16; ++i)
    {
        //Copy into result
        result[2*i] = t[i] & 0xFF;
        result[2*i+1] = t[i] >> 8;
    }
}



/*Converts a number from the byte array
 *representation (Z_p) to the field_element representation
 *result E [0, 2^255-1]
 */
static void unpackCurve25519(field_element result, const u8* in)
{
    //Merge two adjacent bytes into a 16bit value
    //Shifts the 2i+1 byte 8 bits and adds it to 2*i byte
    for (int i = 0; i < 16; ++i)
    {
        result[i]= in[2*i] + ((i64) in[2*i +1] <<8);
    }
    result[15]&= 0x7FFF; //Forces MSB (255th bit) to be 0 as numbers < 2^255
}

/*
 * Brings all the field_element values within the range of [0, 2^16 − 1]
 */
static void carryCurve25519(field_element element)
{
    for (int i = 0; i < 16; ++i)
    {
        //Select bits greater than the lower 16 bits
        i64 carry= element[i] >> 16;

        //substracts the carry bits from element[i] rounding it within the range
        element[i] -= carry <<16;
        //add carry bits to the next element
        if (i < 15)
        {
            element[i+1] += carry;
        }
        //for the last element: modulo 2p reduction
        else
            element[0] += 38 * carry;
    }
}

/*
 *If bit is 1: swaps the content of p and q
* If bit is 0: does nothing
*
* bit could be secret
*/

static void swapCurve25519(field_element p, field_element q, int bit)
{

    i64 c = ~(bit-1); //if bit==0 => 0 | if  bit==1 => 0xFFFFFF...
    for (i64 i = 0; i < 16; ++i)
    {
        i64 t = c & (p[i] ^ q[i]); //if bit==0 => 0 | if bit==1 => p[i] ^ q[i]
        //doesnt have effect if bit==0
        p[i] ^= t; //p[i] = p[i] ^ (p[i] ^ q[i]) = q[i] if bit==0
        q[i] ^= t;
    }

}



/***
 *FIELD ARITHMETIC OPERATIONS
 ***/

/* Adds each of the 16 elements separately
 * result = a + b
 */
static void fieldAdd(field_element result, const field_element a, const field_element b)
{
    for (int i = 0; i < 16; ++i)
    {
        result[i]= a[i] + b[i];//two field elements
    }
}

/* Subtracts each of the 16 elements separately
 * result = a - b
 */
static void fieldSub(field_element result, const field_element a, const field_element b)
{
    for (int i = 0; i < 16; ++i)
    {
        result[i]= a[i] - b[i];  //two field elements
    }
}

/*
 * Multiplies two elements in the field_element form, the product is a
 * 31 element array of 64bit integers
 */
static void fieldMult(field_element result, const field_element a, const field_element b)
{
    i64 product[31], i, j; //product is a 31-element array of i64

    for (i=0; i < 31; ++i)
    {
        product[i]= 0;
    }
    for (i=0; i < 16; ++i)
    {
        for (j=0; j < 16; ++j)
            product[i+j]+= a[i]*b[j];
    }

    //Reduce modulo 2p = 2*(2^255-19) = 2^256-38
    for (i=0; i < 15; ++i){
        //we ignore product[16] to product[30].
        product[i] += 38* product[i+16];
    }
    //copies the product into the result variable
    for (i=0; i < 16; ++i){
        result[i]= product[i];
    }

    //Call two times to make sure the elements are within the range [0, 2^16 − 1]
    carryCurve25519(result);
    carryCurve25519(result);
}
/*
 * (mod p) = 2^255 − 19 (Pequeño Teorema de Fermat)
 * Division b/a= inverse(a) * b
 */
static void fieldInverse(field_element result,  const field_element in)
{
    field_element c;

    //Init c with input value
    for (int i = 0; i < 16; ++i)
        c[i] = in[i];

    //Square and multiply method: count down from 253 bit
    for (int i = 253; i >=0; i--)
    {
        fieldMult(c, c, c); //a**2i = a**i * a**i
        //skips Bits 2 and 4 as are always 0
        if (i!= 2 && i !=4)
            fieldMult(c, c, in);//a**(2i+1) = a * a**i * a**i
    }
    //Copy c to result
    for (int i = 0; i < 16; ++i)
        result[i] = c[i];
}

