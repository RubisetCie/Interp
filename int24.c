/* ***********************************************
 *   Implementation of the 24-bit integer.
 *
 *   Creator: Matthieu 'Rubisetcie' Carteron
 * ***********************************************/

#include "int24.h"

/* Converts int24 to standard int */
int toInt32(const int24 input)
{
    /* Handle the sign */
    if (input.b[2] & 0x80)
    {
        return (0xff << 24) | (input.b[2] << 16)
                            | (input.b[1] << 8)
                            |  input.b[0];
    }
    else
    {
        return (input.b[2] << 16)
             | (input.b[1] << 8)
             |  input.b[0];
    }
}

/* Converts standard int to int24 */
int24 toInt24(const int input)
{
    int24 value;

    value.b[0] = ((unsigned char*)&input)[0];
    value.b[1] = ((unsigned char*)&input)[1];
    value.b[2] = ((unsigned char*)&input)[2];

    return value;
}

/* Returns the minimum value between a and b */
int min24(const int24 a, const int24 b)
{
    const int valuea = toInt32(a);
    const int valueb = toInt32(b);

    return valuea < valueb ? valuea : valueb;
}

/* Returns the maximum value between a and b */
int max24(const int24 a, const int24 b)
{
    const int valuea = toInt32(a);
    const int valueb = toInt32(b);

    return valuea > valueb ? valuea : valueb;
}
