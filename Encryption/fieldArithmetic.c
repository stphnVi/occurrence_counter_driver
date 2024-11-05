//
// Created by katharsis on 1/11/24.
//

#include "fieldArithmetic.h"



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
 *
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
