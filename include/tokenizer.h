#pragma once

#ifdef _WIN32
	#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#endif
#endif

#define MSTRLEN 10000


#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct
{
	char **tokens;
	char *memarena;
	size_t size;
} TokenData;


// Checks if equation is valid,
//        not too small
//		  no invalid space sequences,
//        not too long (exceeds MSTRLEN)
bool validate(char *dst, char *src);


// Checks whenever ch is ^, *, /, + or -
bool is_operand(unsigned char ch);


// Returns pointer to a heap array of strings (char *),
// plus its size; For example:
// "-0.49x+1" -> {"-0.49", "*", "x", "+", "1"}
// 
// Assumes the string was correct, i.e. validate()
// returned -1
TokenData tokenize(char* str);


// Memory freeing for tokens array
void free_tarray(TokenData _tokens);