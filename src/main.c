#include "../include/evaluator.h"
#include "../include/tokenizer.h"

#include "../include/app.h"
#include "../include/config.h"
#include "../include/canvas.h"
#include "../include/ui.h"

int main(void)
{
	Input inputs[MAX_EQUATIONS] = { 0 };
	RenderTexture2D plots_cache;
	Font font;

	if (!init_app(&font, &plots_cache, inputs))
		exit(-1);
	
	size_t      total = 1;
	size_t     active = 0;
	size_t next_color = 1;

	bool changed = true;
	View    view = { 0.0, 0.0, INITIAL_SCALE };

	while (!WindowShouldClose())
	{
		int         w = GetScreenWidth();
		int         h = GetScreenHeight();
		Vector2 mouse = GetMousePosition();
		bool on_input = mouse.x < (float)(w / INPUTBOX_REL);

		changed |= handle_panning(&view, on_input);
		changed |= handle_zooming(&view, mouse, w, h, on_input);
		changed |= handle_input_click(inputs, mouse, w, h, &total, &active, &next_color, on_input);
		changed |= handle_input_typing(inputs, active);
		changed |= handle_input_delete(inputs, active);
		changed |= handle_inputs_navigation(inputs, &active, total);
		changed |= handle_inputs_adding(inputs, &next_color, &active, &total);
		changed |= handle_camera_reset(&view);
	
		if (IsWindowResized())
		{
			if (!resize_plot_cache(&plots_cache, w, h))
				break;
			changed = true;
		}
		
		if (changed)
		{
			rerender_plots(inputs, plots_cache, view, font, total, w, h);
			changed = false;
		}

		render_frame(inputs, plots_cache, w, h, font, active, total);
	}

	free_exit(&total, inputs, &plots_cache, &font);
	CloseWindow();
	return 0;
}
