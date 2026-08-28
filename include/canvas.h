#pragma once

#include "config.h"
#include "evaluator.h"
#include "raylib.h"

/*
	things related to plots, axis and camera:
	plotting itself, grid drawing, camera pan/zoom
*/

typedef struct Input Input;   // forward declaration

typedef struct
{
	double x_offset;
	double y_offset;
	double scale;
} View;

typedef struct
{
	double  x;
	double  y;
	Vector2 s; // screen coords
	bool    valid;
} Sample;

typedef struct
{
	Instr *plot_program;
	size_t plotp_length;
	double *values;
} Plot;

void render_grid(View v, Font f, int w, int h, bool light);

void render_plot(Plot p, View v, int w, int h, Color color, bool is_periodic);
void update_plot(Input *input, bool is_periodic);
void free_plot(Plot *p);

bool handle_panning(View *view, bool on_input);
bool handle_zooming(View *view, Vector2 mouse, int w, int h, bool on_input);
bool handle_camera_reset(View *view);
