/* ***********************************************
 *   Implementation of the 24-bit integer.
 *
 *   Creator: Matthieu 'Rubisetcie' Carteron
 * ***********************************************/

#ifndef INT24_H_INCLUDED
#define INT24_H_INCLUDED

/* The 24-bit integer structure */
typedef struct int24{
    unsigned char b[3];
} int24;

/* Converts int24 to standard int */
int toInt32(const int24 input);

/* Converts standard int to int24 */
int24 toInt24(const int input);

/* Returns the minimum value between a and b */
int min24(const int24 a, const int24 b);

/* Returns the maximum value between a and b */
int max24(const int24 a, const int24 b);

#endif
