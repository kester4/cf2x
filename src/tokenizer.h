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


// Returns -1 if equation is valid,
//         -2 if too small
//		   -3 if invalid space sequences were presented,
//         -4 if too long (exceeds MSTRLEN)
// firstly met 'wrong' operator/operand index otherwise
long validate(char *raw_str);


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