#ifndef UTILS_H
#define UTILS_H

#define SWAP(x, y, T) do { T SWAP = x; x = y; y = SWAP; } while (0)
#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)
#define CLAMP(value, low, high) (value < low ? low : (value > high ? high : value))

float Q_rsqrt(float number);

#endif
