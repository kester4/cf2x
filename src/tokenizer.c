#include "../include/tokenizer.h"


static bool trimw_prevalidate(char *dst, char *src)
{
	// Check whenever there are invalid sequences,
	// such as "alnum <space> alnum", or "cos (x )"
	// before getting rid of whitespaces and
	// writting trimmed string to dst buffer
	// 
	// Note: "- x" is still allowed /// TODO
	// "<letter> = ...", "<letter>(x) = ..." is allowed too
	// well, except for "e = ..." and "x = ..."

	bool is_letter;
	bool was_letter = false;
	bool was_space = false;

	for (char *start = src; *start; ++start)
	{
		unsigned char x = (unsigned char)*start;

		if (isalnum(x) || is_operand(x))
		{
			is_letter = isalnum(x);

			if (was_space && was_letter && is_letter)
				return false;

			was_letter = is_letter;
		}
		else if (!isspace(x))
			was_letter = false;
		was_space = isspace(x);
	}

	// trim whitespaces
	for (char *start = src; *start; ++start)
	{
		if (!isspace((unsigned char)*start))
			*dst++ = *start;
	}

	*dst = '\0';
	return true;
}

bool validate(char *dst, char *src)
{
	if (!trimw_prevalidate(dst, src))
		return false;

	const size_t length = strlen(dst);
	if (length == 0 || length > MSTRLEN)
		return false;
	else if (length == 1)
		return (isdigit(dst[0]) || dst[0] == 'x' || dst[0] == 'e');

	int bracket_depth = 0;
	bool unclosed_sign = false;

	for (size_t i = 0; i < length; ++i)
	{
		unsigned char curr = (unsigned char)dst[i];
		unsigned char next = (i + 1 < length) ? (unsigned char)dst[i + 1] : '\0';

		if (isalpha(curr))
		{
			unclosed_sign = false;
			size_t left = length - i;

			// exponent
			if      (left >= 4 && strncmp(dst + i, "exp(", 4) == 0) { ++bracket_depth; i += 3; }

			// argument or e constant
			else if (curr == 'x' || curr == 'e')
			{
				if (isdigit(next) || next == '.')
					return false;
				continue;
			}

			// logarithms
			else if (left >= 4 && strncmp(dst + i, "log(", 4) == 0) { ++bracket_depth; i += 3; }
			else if (left >= 3 && strncmp(dst + i, "ln(", 3) == 0)  { ++bracket_depth; i += 2; }

			// trigonometry
			else if (left >= 4 && strncmp(dst + i, "sin(", 4) == 0) { ++bracket_depth; i += 3; }
			else if (left >= 4 && strncmp(dst + i, "cos(", 4) == 0) { ++bracket_depth; i += 3; }
			else if (left >= 4 && strncmp(dst + i, "tan(", 4) == 0) { ++bracket_depth; i += 3; }
			else if (left >= 4 && strncmp(dst + i, "cot(", 4) == 0) { ++bracket_depth; i += 3; }

			// absolute value
			else if (left >= 4 && strncmp(dst + i, "abs(", 4) == 0) { ++bracket_depth; i += 3; }
				
			else
				return false;

			// blank func() parens are not allowed
			if (length - i >= 1 && dst[i + 1] == ')')
				return false;
		}

		else if (isdigit(curr))
		{
			if (next && !isalnum(next) && !is_operand(next) 
				&& next != '.' && next != '(' && next != ')')
				return false;
			unclosed_sign = false;
		}

		else if (curr == '(')
		{
			if (!isalnum(next) && next != '(' && next != '-')
				return false;
			++bracket_depth;
			unclosed_sign = false;
		}

		else if (curr == ')')
		{
			if (bracket_depth-- <= 0 || isdigit(next))
				return false;
		}

		else if (curr == '+' || curr == '-' || curr == '*' || curr == '/')
		{
			// minus sign is alowed at the very beginning
			// or after opening bracket
			if ((curr != '-') &&
				(i == 0 || (i > 0 && dst[i - 1] == '(')))
				return false;
			if (unclosed_sign || i == length - 1 || next == ')')
				return false;
			else
				unclosed_sign = true;
		}

		else if (curr == '.')
		{
			if (!isdigit(next))
				return false;
			unclosed_sign = false;
		}

		else if (curr == '^')
		{
			if (i == 0 || dst[i - 1] == '(' || unclosed_sign)
				return false;
			if (!isalnum(next) && next != '.' && next != '(')
				return false;
			unclosed_sign = false;
		}

		else
			return false;
	}

	return (bracket_depth == 0);
}


bool is_operand(unsigned char ch)
{
	return ch == '^' || ch == '*' || ch == '/'
		|| ch == '+' || ch == '-';
}


static  void chr_cpy(char **arr, size_t idx, unsigned char src)
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

		// "-x/-func/-e" at the very begining or after "("
		bool unary_minus = (curr == '-') && (i == 0 || str[i - 1] == '(');
		if (unary_minus && !isdigit(next) && next != '.')
		{
			strcpy(tokens[ti++], "0");
			strcpy(tokens[ti++], "-");
			continue;
		}
		
		// natural logarithm
		else if (curr == 'l' && next == 'n')
		{
			strcpy(tokens[ti++], "ln");
			++i;
		}

		// trigonometric function, logarithm, abs or exp()
		else if (curr == 's' || curr == 'c' || curr == 't' || curr == 'a'
			|| curr == 'l' || (curr == 'e' && next == 'x' && str[i + 2] == 'p'))
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
			chr_cpy(tokens, ti++, (curr != 'e' ? curr : 'E'));
			chr_cpy(tokens, ti++, '*');
		}
		// ... ")(" -> ") * ("
		else if (curr == ')' && next == '(')
		{
			chr_cpy(tokens, ti++, ')');
			chr_cpy(tokens, ti++, '*');
		}

		// single operand, brackets, x arg or e-constant
		// (also not a part of -<...> at the beginning)
		else if (curr == 'x' || curr == 'e' || curr == '(' || curr == ')'
			|| (i != 0 && is_operand(curr) && !unary_minus))
			chr_cpy(tokens, ti++, (curr != 'e' ? curr : 'E'));

		// integer or fraction
		else if (isdigit(curr) || curr == '.')
		{
			size_t bi = 0;

			// unary minus pre digit, like "(-1)"
			if (i > 0 && str[i - 1] == '-'
				&& (i == 1 || str[i - 2] == '('))
				tokens[ti][bi++] = '-';

			while (i < strl && (isdigit(str[i]) || str[i] == '.'))
				tokens[ti][bi++] = str[i++];

			tokens[ti++][bi] = '\0';

			--i;

			// add "*" after digit/bracket if there will be <NUM>X / <NUM>(
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
