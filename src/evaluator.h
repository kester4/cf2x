#pragma once

#ifdef _WIN32
	#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#endif
#endif

#include "tokenizer.h"
#include <math.h>
#include <float.h>

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
	OP_COTAN
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


//
TokenData shunting_yard(TokenData _tokens);


//
Instr *prebake(TokenData _pftokens);


//
double evaluate(Instr *program, ValueStack *stack, double arg);