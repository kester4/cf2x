#include "evaluator.h"
#include "tokenizer.h"

static  unsigned int precedence(char op)
{
	switch (op)
	{
	case 's': return 4;
	case 'c': return 4;
	case 't': return 4;
	case '^': return 3;
	case '*': return 2;
	case '/': return 2;
	case '+': return 1;
	case '-': return 1;
	}
	return 0; // no warning
}

TokenData shunting_yard(TokenData _tokens)
{
	char **tokens = _tokens.tokens;
	const size_t length = _tokens.size;

	char **output = malloc(sizeof(char *) * length);
	if (!output)
	{
		free_tarray(_tokens);
		return (TokenData){ 0 };
	}

	char **stack = malloc(sizeof(char *) * length);
	if (!stack)
	{
		free(output);
		free_tarray(_tokens);
		return (TokenData){ 0 };
	}

	size_t oi = 0; // index for stack and output queue
	size_t si = 0;

	for (size_t i = 0; i < length; ++i)
	{
		unsigned char t = (unsigned char)tokens[i][0];

		// token is a number or an argument
		if (t == 'x' || isdigit(t)
			|| (t == '-' && isdigit((unsigned char)tokens[i][1]))
			|| t == '.')
		{
			output[oi++] = tokens[i];
			continue;
		}
		
		// token is trigonometry func or a (
		else if (t == '(' || strchr("sct", t))
		{
			stack[si++] = tokens[i];
			continue;
		}

		// token is a right paren
		else if (t == ')')
		{
			while (*stack[si - 1] != '(')
				output[oi++] = stack[--si];
			--si; // discard '('

			// add trigonometry to ouput too if any
			if (si && strchr("sct", stack[si][0]))
				output[oi++] = stack[--si];
			continue;
		}
	
		// token is +, -, *, ^ or /
		while (si
			&& precedence(*stack[si - 1]) >= precedence(t) 
			&& !(*stack[si - 1] == '^' && t == '^')) // equal precedence does not pop
		{
			output[oi++] = stack[--si];
		}
		stack[si++] = tokens[i];
	}
	

	while (si && *stack[si - 1] != '(')
		output[oi++] = stack[--si];

	free(stack);
	free(_tokens.tokens);

	return (TokenData) {
		.tokens = output,
		.memarena = _tokens.memarena,
		.size = oi
	};
}

Instr *prebake(TokenData _pftokens)
{
	char **tokens = _pftokens.tokens;
	const size_t length = _pftokens.size;

	Instr *program = malloc(sizeof(Instr) * length);
	if (!program)
	{
		free_tarray(_pftokens);
		return NULL;
	}

	size_t pi = 0;

	for (size_t i = 0; i < length; ++i)
	{
		unsigned char t = (unsigned char)tokens[i][0];
		
		// argument
		if (t == 'x')
			program[pi] = (Instr){ .type = OP_VAR, .value = (double)(int)t};

		// digit (incl. fractions)
		else if ((t == '-' && isdigit((unsigned char)tokens[i][1]))
			|| t == '.'
			|| isdigit((unsigned char)t))
		{
			errno = 0;
			double v = strtod(tokens[i], NULL);
			if (errno == ERANGE)
			{
				free(program);
				free_tarray(_pftokens);
				return NULL;
			}
			program[pi] = (Instr){ .type = OP_NUM, .value = v };
		}

		// trigonometry
		else if (strcmp(tokens[i], "sin") == 0)
			program[pi] = (Instr){ .type = OP_SIN };
		else if (strcmp(tokens[i], "cos") == 0)
			program[pi] = (Instr){ .type = OP_COS };
		else if (strcmp(tokens[i], "tan") == 0)
			program[pi] = (Instr){ .type = OP_TAN };
		else if (strcmp(tokens[i], "cot") == 0)
			program[pi] = (Instr){ .type = OP_COT };

		// operands
		else if (t == '^') program[pi] = (Instr){ .type = OP_POW };
		else if (t == '*') program[pi] = (Instr){ .type = OP_MUL };
		else if (t == '/') program[pi] = (Instr){ .type = OP_DIV };
		else if (t == '+') program[pi] = (Instr){ .type = OP_ADD };
		else if (t == '-') program[pi] = (Instr){ .type = OP_SUB };

		++pi;
	}

	free_tarray(_pftokens);
	return program;
}

double evaluate(Instr *program, ValueStack *stack, double arg)
{
	// stack->msize == elements in *program
	double *s = stack->values;
	size_t sp = 0;

	for (size_t i = 0; i < stack->msize; i++)
	{
		switch (program[i].type) {
		case OP_VAR:
			s[sp++] = arg;
			break;

		case OP_NUM:
			s[sp++] = program[i].value;
			break;

		case OP_SIN:
			s[sp - 1] = sin(s[sp - 1]);
			break;

		case OP_COS:
			s[sp - 1] = cos(s[sp - 1]);
			break;

		case OP_TAN:
			s[sp - 1] = tan(s[sp - 1]);
			break;

		case OP_COT:
			s[sp - 1] = 1 / tan(s[sp - 1]);
			break;

		case OP_POW: {
			double exponent = s[--sp];
			double base = s[--sp];

			if (exponent == 2.0) {
				s[sp++] = base * base;
			}
			else if (exponent == 3.0) {
				s[sp++] = base * base * base;
			}
			else if (exponent == 0.5) {
				s[sp++] = sqrt(base);
			}
			else if (exponent == -1.0) {
				s[sp++] = 1.0 / base;
			}
			else {
				s[sp++] = pow(base, exponent);
			}
			break;
		}

		case OP_MUL: {
			double b = s[--sp];
			double a = s[--sp];
			s[sp++] = a * b;
			break;
		}

		case OP_DIV: {
			double b = s[--sp];
			double a = s[--sp];
			s[sp++] = a / b;
			break;
		}

		case OP_ADD: {
			double b = s[--sp];
			double a = s[--sp];
			s[sp++] = a + b;
			break;
		}

		case OP_SUB: {
			double b = s[--sp];
			double a = s[--sp];
			s[sp++] = a - b;
			break;
		}
		}
	}

	return s[sp - 1];
}
