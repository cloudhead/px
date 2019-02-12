//
// hash.c
// hash functions
//
// (c) 2014, Alexis Sellier
//
#include "hash.h"

//
// Fowler-Noll-Vo 32-bit hash function (FNV-1a)
//
unsigned long hash(const char *input, unsigned long len)
{
	static const unsigned long FNV_PRIME = 16777619;
	static const unsigned long FNV_BASIS = 2166136261;

	unsigned long hash = FNV_BASIS;

	for (unsigned long i = 0; i < len; i ++) {
		hash  ^=  (unsigned long)input[i];
		hash  *=  FNV_PRIME;
	}
	return hash;
}
