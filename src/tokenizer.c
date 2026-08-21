#include "tokenizer.h"


static int trimw_prevalidate(char *raw_str)
{
	// Check whenever there are invalid sequences, e.g:
	// "x + 2 3", "x x" or "x    2"
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

	const size_t length = strlen(raw_str);
	if (length == 0)
		return -2;
	else if (length > MSTRLEN)
		return -4;
	else if (length == 1)
		return isalnum(raw_str[0]) ? -1 : 0;

	char arg;
	int bracket_depth = 0;
	bool argmet = false;
	bool unclosed_sign = false;

	for (long i = 0; i < (long)length; ++i)
	{
		unsigned char curr = (unsigned char)raw_str[i];
		unsigned char next = (i + 1 < (long)length) ? (unsigned char)raw_str[i + 1] : '\0';

		if (isalpha(curr))
		{
			if (!argmet) { argmet = true; arg = curr; }
			else if (arg != curr) // different arg letter met (only single letter allowed)
				return i;
			if (isdigit(next) || next == '.')
				return i;
			unclosed_sign = false;
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
			if (bracket_depth-- <= 0 || isalnum(next))
				return i;
		}

		else if (curr == '+' || curr == '-' || curr == '*' || curr == '/')
		{
			// only minus sign is alowed at the very beginning
			// or after opening bracket
			if ((curr != '-') &&
				(i == 0 || (i > 0 && raw_str[i - 1] == '(')))
				return i;
			if (unclosed_sign || i == (long)length - 1) 
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


inline bool is_operand(unsigned char ch)
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

		// "-x" at the very begining or after "("
		if (isalpha(curr) && i > 0 && str[i - 1] == '-' && (i == 1 || str[i - 2] == '('))
		{
			strcpy(tokens[ti++], "0");
			strcpy(tokens[ti++], "-");
			chr_cpy(tokens, ti++, curr);
		}
		
		// short multiplication form expanding
		// "xx" -> "x * x",
		// "x(..." -> "x * (..." and
		else if (isalpha(curr) && (isalpha(next) || next == '(') && i < strl - 1)
		{
			chr_cpy(tokens, ti++, curr);
			chr_cpy(tokens, ti++, '*');
		}
		// ")(" -> ") * ("
		else if (curr == ')' && next == '(')
		{
			chr_cpy(tokens, ti++, ')');
			chr_cpy(tokens, ti++, '*');
		}

		// single operand, brackets or regular arg letter
		// (also not a part of -<NUM> in the beginning)
		else if (isalpha(curr) || curr == '(' || curr == ')'
			|| (i != 0 && is_operand(curr)))
			chr_cpy(tokens, ti++, curr);

		// integer or fraction
		else if (isdigit(curr) || curr == '.')
		{
			size_t bi = 0;

			// if it is first number and is negative
			if (i == 1 && str[i - 1] == '-')
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
