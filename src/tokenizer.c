#include "tokenizer.h"


static int trimw_prevalidate(char *raw_str)
{
	// Check whenever there are invalid sequences, e.g:
	// "alnum <space> alnum", or "cos (x )"
	// before getting rid of whitespaces
	// Note: "- x" is still allowed
	char* start = raw_str;
	bool is_letter;
	bool was_letter = false;
	bool was_space = false;
	while (*raw_str)
	{
		unsigned char x = (unsigned char)*raw_str;

		if (isalnum(x) || is_operand(x))
		{
			is_letter = isalnum(x);

			if (was_space && was_letter && is_letter)
				return -2;

			was_letter = is_letter;
		}
		else if (!isspace(x))
			was_letter = false;
		was_space = isspace(x);
		raw_str++;
	}

	// trim whitespaces
	raw_str = start;
	char* dst = raw_str;
	while (*raw_str)
	{
		if (!isspace((unsigned char)*raw_str))
			*dst++ = *raw_str;
		raw_str++;
	}

	*dst = '\0';
	return 0;
}

long validate(char *raw_str)
{
	if (trimw_prevalidate(raw_str) == -2)
		return -3;

	const long length = (long)strlen(raw_str);
	if (length == 0)
		return -2;
	else if (length > MSTRLEN)
		return -4;
	else if (length == 1)
		return (isdigit(raw_str[0]) || raw_str[0] == 'x') ? -1 : 0;

	int bracket_depth = 0;
	bool unclosed_sign = false;

	for (long i = 0; i < length; ++i)
	{
		unsigned char curr = (unsigned char)raw_str[i];
		unsigned char next = (i + 1 < length) ? (unsigned char)raw_str[i + 1] : '\0';

		if (isalpha(curr))
		{
			unclosed_sign = false;
			if (curr == 'x')
			{
				if (isdigit(next) || next == '.')
					return i;
				continue;
			}
			
			// trigonometry
			size_t left = length - i;
			if      (left >= 4 && strncmp(raw_str + i, "sin(", 4) == 0) { ++bracket_depth; i += 3; }
			else if (left >= 4 && strncmp(raw_str + i, "cos(", 4) == 0) { ++bracket_depth; i += 3; }
			else if (left >= 4 && strncmp(raw_str + i, "tan(", 4) == 0) { ++bracket_depth; i += 3; }
			else if (left >= 4 && strncmp(raw_str + i, "cot(", 4) == 0) { ++bracket_depth; i += 3; }
			else
				return i;

			// blang trig_func() parens are not allowed
			if (length - i >= 1 && raw_str[i + 1] == ')')
				return i;
		}

		else if (isdigit(curr))
		{
			if (next && !isalnum(next) && !is_operand(next) 
				&& next != '.' && next != '(' && next != ')')
				return i;
			unclosed_sign = false;
		}

		else if (curr == '(')
		{
			if (!isalnum(next) && next != '(' && next != '-')
				return i + 1;
			++bracket_depth;
		}

		else if (curr == ')')
		{
			if (bracket_depth-- <= 0)
				return i;
		}

		else if (curr == '+' || curr == '-' || curr == '*' || curr == '/')
		{
			// minus sign is alowed at the very beginning
			// or after opening bracket
			if ((curr != '-') &&
				(i == 0 || (i > 0 && raw_str[i - 1] == '(')))
				return i;
			if (unclosed_sign || i == length - 1) 
				return i;
			else
				unclosed_sign = true;
		}

		else if (curr == '.')
		{
			if (!isdigit(next))
				return i;
			unclosed_sign = false;
		}

		else if (curr == '^')
		{
			if (i == 0)
				return i;
			if (!isalnum(next) && next != '.' && next != '(')
				return i;
			unclosed_sign = false;
		}

		else
			return i;
	}

	return bracket_depth == 0 ? -1 : -3;
}


bool is_operand(unsigned char ch)
{
	return ch == '^' || ch == '*' || ch == '/'
		|| ch == '+' || ch == '-';
}


static inline void chr_cpy(char **arr, size_t idx, unsigned char src)
{
	arr[idx][0] = src;
	arr[idx][1] = '\0';
}


TokenData tokenize(char *str)
{
	const size_t strl = strlen(str);
	char **tokens = malloc(sizeof(char*) * strl * 2);
	if (!tokens)
	{
		return (TokenData){ 0 };
	}

	char *memarea = malloc(sizeof(char) * (strl + 1) * (strl * 2));
	if (!memarea)
	{
		free(tokens);
		return (TokenData){ 0 };
	}

	for (size_t i = 0; i < strl * 2; ++i)
		tokens[i] = memarea + i * (strl + 1);
	
	// tokenizing
	size_t ti = 0;
	for (
		size_t i = 0;
		i < strl;
		++i
		)
	{
		unsigned char curr = (unsigned char)str[i];
		unsigned char next = (unsigned char)str[i + 1];

		// "-x/-func" at the very begining or after "("
		bool unary_minus = (curr == '-') && (i == 0 || str[i - 1] == '(');
		if (unary_minus && !isdigit(next) && next != '.')
		{
			strcpy(tokens[ti++], "0");
			strcpy(tokens[ti++], "-");
			continue;
		}
		
		// trigonometric function
		else if (curr == 's' || curr == 'c' || curr == 't')
		{
			tokens[ti][0] = curr;
			tokens[ti][1] = next;
			tokens[ti][2] = str[i + 2];
			tokens[ti++][3] = '\0';
			i += 2;
		}

		// short multiplication form expanding pt.2
		// "xx" -> "x * x", "xfunc(..." -> "x * func(..."
		// "x(..." -> "x * (..." and also ...
		else if ((isalpha(curr) || curr == ')')
			&& (isalpha(next) || next == '(')
			&& i < strl - 1)
		{
			chr_cpy(tokens, ti++, curr);
			chr_cpy(tokens, ti++, '*');
		}
		// ... ")(" -> ") * ("
		else if (curr == ')' && next == '(')
		{
			chr_cpy(tokens, ti++, ')');
			chr_cpy(tokens, ti++, '*');
		}

		// single operand, brackets or regular arg letter
		// (also not a part of -<...> at the beginning)
		else if (curr == 'x' || curr == '(' || curr == ')'
			|| (i != 0 && is_operand(curr) && !unary_minus))
			chr_cpy(tokens, ti++, curr);

		// integer or fraction
		else if (isdigit(curr) || curr == '.')
		{
			size_t bi = 0;

			// unary minus pre dogit, like "(-1)"
			if (i > 0 && str[i - 1] == '-'
				&& (i == 1 || str[i - 2] == '('))
				tokens[ti][bi++] = '-';

			while (i < strl && (isdigit(str[i]) || str[i] == '.'))
				tokens[ti][bi++] = str[i++];

			tokens[ti++][bi] = '\0';

			--i;

			// add "*" after digit/bracket if there will be <NUM>X/<NUM>(
			if (i < strl && (isalpha(str[i + 1]) || str[i + 1] == '('))
				strcpy(tokens[ti++], "*");
		}
	}

	return (TokenData) {
		.tokens = tokens,
		.memarena = memarea,
		.size = ti
	};
}


void free_tarray(TokenData _tokens)
{
	free(_tokens.memarena);
	free(_tokens.tokens);
}
