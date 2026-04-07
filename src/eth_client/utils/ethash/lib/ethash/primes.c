// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.
#include "primes.h"
#include <stdbool.h>

/// Checks if the number is prime. Requires the number to be > 2 and odd.
static inline bool is_odd_prime(int number)
{
    // Check factors up to sqrt(number) by doing comparison d*d <= number with 64-bit precision.
    for (int d = 3; (int64_t)d * (int64_t)d <= (int64_t)number; d += 2)
    {
        if (number % d == 0)
            return false;
    }

    return true;
}

int ethash_find_largest_prime(int upper_bound)
{
    int n = upper_bound;

    if (n < 2)
        return 0;

    if (n == 2)
        return 2;

    // Skip even numbers.
    if (n % 2 == 0)
        --n;

    // Test descending odd numbers.
    while (!is_odd_prime(n))
        n -= 2;

    return n;
}
