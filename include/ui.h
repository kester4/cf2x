#pragma once

#include "config.h"
#include "canvas.h"
#include "raylib.h"

/*
	functions related to the input panel:
	editing, updating plots with inputs
	and panel's rendering itself
*/

typedef struct Input
{
	Plot   plot;
	char *text;
	char *origin;    // user input
	size_t capacity; // malloc'd size of text
	size_t caret;    // caret pos
	bool   periodic;
	bool   valid;
	float  scroll;
	Color  color;
} Input;

void render_menu(int w, int h, Font f, Input *inputs, size_t size, size_t active);

bool handle_input_typing(Input *inputs, size_t active);
bool handle_input_delete(Input *inputs, size_t active);
bool handle_inputs_navigation(Input *inputs, size_t *active, size_t total);
bool handle_inputs_adding(Input *inputs, size_t *next_color, size_t *active, size_t *total);
bool handle_input_click(Input *inputs, Vector2 mouse, int w, int h, size_t *total, size_t *active, size_t *next_color, bool on_input);
