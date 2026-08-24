#pragma once

#ifdef _WIN32
	#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#endif
#endif

#include "tokenizer.h"
#include <math.h>
#include <float.h>
#include <errno.h>

typedef enum
{
	OP_NUM,
	OP_VAR,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_POW,
	OP_SIN,
	OP_COS,
	OP_TAN,
	OP_COT
} OpType;


typedef struct
{
	OpType type;
	double value;
} Instr;


typedef struct
{
	double *values;
	size_t msize;
} ValueStack;


// Shunting-yard's infix token array to postfix
// token array, frees the infix array
//
// Returns newly malloc'd postfix TokenData or
//         empty TokenData on malloc failure
TokenData shunting_yard(TokenData _tokens);


// Converts postfix tokens into Instr array,
// strtol's numeric tokens to doubles so it is 
// done once not per every evaluate() call
//
// Returns newly malloc'd Instr array or
//         NULL if malloc fails or a number is
//         too long for a double
Instr *prebake(TokenData _pftokens);


// RPN's Instr array for the given arg value
double evaluate(Instr *program, ValueStack *stack, double arg);