#pragma once

#include "config.h"
#include "canvas.h"
#include "ui.h"

#include "rlgl.h"
#include "raylib.h"

#include <stddef.h>
#include <stdbool.h>

/*
	high level stuff which i didnt find place to insert in other *.h
*/

bool init_app(Font *font, RenderTexture2D *plots_cache, Input *inputs);

void free_plots(size_t *size, Input *inputs);

bool resize_plot_cache(RenderTexture2D *plots_cache, int w, int h);

void rerender_plots(Input *inputs, RenderTexture2D plots_cache,
	View view, Font font, size_t total, int w, int h, bool light);
void render_frame(Input *inputs, RenderTexture2D plots_cache,
	int w, int h, bool light, Font font, size_t active, size_t total);

void free_exit(size_t *total, Input *inputs, RenderTexture2D *plots_cache, Font *font);
