#include "../include/canvas.h"
#include "../include/ui.h"

static Vector2 screen_from_world(View v, double x, double y, int w, int h)
{
	return (Vector2)
	{
		(float)(w * 0.5 + (x - v.x_offset) * v.scale),
			(float)(h * 0.5 - (y - v.y_offset) * v.scale)
	};
}

static Vector2 world_from_screen(View v, float x, float y, int w, int h)
{
	return (Vector2)
	{
		(float)(v.x_offset + (x - w * 0.5) / v.scale),
			(float)(v.y_offset + (0.5 * h - y) / v.scale)
	};
}

static Plot plot(char *valid_input)
{
	TokenData tokens = tokenize(valid_input);
	if (!tokens.tokens)
		return (Plot) { 0 };

	TokenData postfix = shunting_yard(tokens);
	if (!postfix.tokens)
		return (Plot) { 0 };

	double *values = malloc(sizeof(double) * postfix.size);
	if (!values)
	{
		free_tarray(postfix);
		return (Plot) { 0 };
	}

	Instr *program = prebake(postfix);
	if (!program)
	{
		free(values);
		free_tarray(postfix);
		return (Plot) { 0 };
	}

	return (Plot) {
		.plot_program = program,
		.plotp_length = postfix.size,
		.values = values,
		.denom_spans = denominator_spans(program, postfix.size)
	};
}

static char *format(char *axis_label, double scale, double number, double step)
{
	int past_e = (int)floor(log10(fabs(step)));
	if (fabs(number) >= MAX_SHORTVAL && past_e >= 5 && scale < INITIAL_SCALE / 5)
	{
		double pre_e = number / powf(10.0f, (float)past_e);
		snprintf(axis_label, 50, "%.1fE%d", pre_e, past_e);
		return axis_label;
	}

	int dec = (step >= 1.0) ? 0 : (int)ceil(-log10(step));
	snprintf(axis_label, 50, "%.*f", dec, number);

	return axis_label;
}

void render_grid(View v, Font f, int w, int h, bool light)
{
	double spacing = GRID_SPACING * SSAA / v.scale;
	double   order = floor(log10(spacing));
	double    norm = spacing / pow(10.0, order); // from 1 to 10
	double    mult = (norm < 1.5) ? 1.0 : (norm < 3.0) ? 2.0 : (norm < 7.0) ? 5.0 : 10.0;
	double   major = mult * pow(10.0, order);
	double   minor = major / MINORL_COUNT;

	float scmin_step = minor * v.scale;

	Vector2      center = screen_from_world(v, 0.0, 0.0, w, h);
	Vector2   top_right = world_from_screen(v, 0.0f, 0.0f, w, h);
	Vector2 bottom_left = world_from_screen(v, (float)w, (float)h, w, h);
	Vector2     current, next, tsz;

	// major + minor grids VERTICAL
	double x_start = floor(top_right.x / major) * major;
	current = screen_from_world(v, x_start, 0, w, h);

	static char axis_label[50];
	for (double xi = x_start; xi <= bottom_left.x; xi += major)
	{
		next = screen_from_world(v, xi + major, 0, w, h);

		// minor
		for (float xj = current.x + scmin_step; xj < next.x; xj += scmin_step)
			DrawLineEx((Vector2) { xj, 0 }, (Vector2) { xj, h }, MINORL_THICK * SSAA,
				(light ? MNGRID_LIGHT : MNGRID_DARK));

		// major (drawn over minor)
		DrawLineEx((Vector2) { current.x, 0 }, (Vector2) { current.x, h }, MAJORL_THICK * SSAA,
			(light ? MJGRID_LIGHT : MJGRID_DARK));

		// don't print zero at (0; 0)
		if (fabs(xi) > 1e-12)
		{
			format(axis_label, v.scale, xi, major);
			tsz = MeasureTextEx(f, axis_label, FONT_SIZE * SSAA, 0.0f);
			DrawTextEx(f, axis_label,
				(Vector2) { current.x - tsz.x * 0.5f, current.y + tsz.y * 0.1f},
				FONT_SIZE * SSAA, 0.0f, (light ? TEXT_LIGHT : TEXT_DARK));
		}
			
		current = next;
	}

	// major + minor grids HORIZONTAL
	double y_start = floor(bottom_left.y / major) * major;
	current = screen_from_world(v, 0, y_start, w, h);

	for (double yi = y_start; yi <= top_right.y; yi += major)
	{
		next = screen_from_world(v, 0, yi + major, w, h);

		for (float yj = current.y + scmin_step; yj > next.y; yj -= scmin_step)
			DrawLineEx((Vector2) { 0, yj }, (Vector2) { w, yj }, MINORL_THICK * SSAA,
				(light ? MNGRID_LIGHT : MNGRID_DARK));

		DrawLineEx((Vector2) { 0, current.y }, (Vector2) { w, current.y }, MAJORL_THICK * SSAA,
			(light ? MJGRID_LIGHT : MJGRID_DARK));

		if (fabs(yi) > 1e-12)
		{
			format(axis_label, v.scale, yi, major);
			tsz = MeasureTextEx(f, axis_label, FONT_SIZE * SSAA, 0.0f);
			DrawTextEx(f, axis_label,
				(Vector2) { center.x + tsz.y * 0.15f, current.y - tsz.y * 0.4f },
				FONT_SIZE * SSAA, 0.0f, (light ? TEXT_LIGHT : TEXT_DARK));
		}

		current = next;
	}

	// main axes
	DrawLineEx((Vector2) { 0, center.y }, (Vector2) { w, center.y },
		AXIS_THICK * SSAA, (light ? AXIS_LIGHT : AXIS_DARK));
	DrawLineEx((Vector2) { center.x, 0 }, (Vector2) { center.x, h },
		AXIS_THICK * SSAA, (light ? AXIS_LIGHT : AXIS_DARK));
}

static Sample sample(Instr *prog, ValueStack *vstack, View v, int w, int h, double x)
{
	double y = evaluate(prog, vstack, x);
	bool valid = isfinite(y);

	return (Sample) {
		.x = x,
			.y = y,
			.s = (Vector2){ (x - v.x_offset) * v.scale + w * 0.5, valid ? h * 0.5 - (y - v.y_offset) * v.scale : 0.0f },
			.valid = valid
	};
}

static bool on_screen(Sample prev, Sample curr, int h)
{
	if (!prev.valid || !curr.valid)
		return false;

	float border = 4.0f * h;
	return !((prev.s.y > border || -prev.s.y > border) &&
		(curr.s.y > border || -curr.s.y > border));
}

static void refine_plot(Instr *prog, ValueStack *vstack, View v, int w, int h, Sample prev, Sample curr, int depth, Color color)
{
	if (!prev.valid && !curr.valid)
		return;

	float px_width = fabsf(curr.s.x - prev.s.x);
	if (px_width <= MAX_LEAF_PX && prev.valid && curr.valid && !on_screen(prev, curr, h))
		return;

	if (depth >= MAX_RECDEPTH)
	{
		if (on_screen(prev, curr, h))
			DrawLineEx((Vector2) { prev.s.x, prev.s.y }, (Vector2) { curr.s.x, curr.s.y }, GRAPH_THICK * SSAA, color);
		return;
	}
	double xm = (prev.x + curr.x) * 0.5;
	Sample mid = sample(prog, vstack, v, w, h, xm);

	bool finite = isfinite(prev.s.y) && isfinite(mid.s.y) && isfinite(curr.s.y);

	bool split = (!prev.valid || !mid.valid || !curr.valid) ||
		(finite && fabsf(mid.s.y - (prev.s.y + curr.s.y) * 0.5f) > TOLERANCE_PX) ||
		!finite || px_width > MAX_LEAF_PX;

	if (on_screen(prev, curr, h) && !split)
	{
		DrawLineEx((Vector2) { prev.s.x, prev.s.y }, (Vector2) { curr.s.x, curr.s.y }, GRAPH_THICK * SSAA, color);
		return;
	}

	refine_plot(prog, vstack, v, w, h, prev, mid, depth + 1, color);
	refine_plot(prog, vstack, v, w, h, mid, curr, depth + 1, color);
}

void render_plot(Plot p, View v, int w, int h, Color color, bool is_periodic)
{
	ValueStack vstack = (ValueStack){
		.msize = p.plotp_length,
		.values = p.values
	};
	double x_start = v.x_offset - (0.5 * w) / v.scale;
	double   x_end = v.x_offset + (0.5 * w) / v.scale;

	if (is_periodic)
	{
		const int N = (int)ceil(w / PERIODIC_FUNC);
		double step = (x_end - x_start) / N;

		Sample left = sample(p.plot_program, &vstack, v, w, h, x_start);
		for (int i = 1; i < N; ++i)
		{
			x_start += step;

			Sample right = sample(p.plot_program, &vstack, v, w, h, x_start);
			refine_plot(p.plot_program, &vstack, v, w, h, left, right, 0, color);

			left = right;
		}
		return;
	}

	double poles[64];
	int    npoles = 0;
	for (int i = 0; i < p.denom_spans.count && npoles < 64; i++)
	{
		npoles += poles_in_span(p.plot_program, &vstack,
			p.denom_spans.spans[i], x_start, x_end, poles + npoles, 64 - npoles);
	}
		
	double lo = x_start;
	for (int i = 0; i < npoles; i++)
	{
		double hi = nextafter(poles[i], -INFINITY);

		// pre pole
		Sample nearHi = sample(p.plot_program, &vstack, v, w, h, hi);
		if (nearHi.valid)
		{
			float border = 4.0f * h;
			if (fabsf(nearHi.s.y - h * 0.5f) < border)
			{
				Sample stub = nearHi;
				stub.x = hi;
				stub.s.y = (nearHi.y < 0) ? border : -border;
				DrawLineEx((Vector2) { nearHi.s.x, nearHi.s.y }, (Vector2) { stub.s.x, stub.s.y },
					GRAPH_THICK * SSAA, color);
			}
		}

		if (hi > lo)
		{
			refine_plot(p.plot_program, &vstack, v, w, h,
				sample(p.plot_program, &vstack, v, w, h, lo),
				sample(p.plot_program, &vstack, v, w, h, hi), 0, color);
		}
		lo = nextafter(poles[i], INFINITY);

		// post pole
		Sample near = sample(p.plot_program, &vstack, v, w, h, lo);
		if (near.valid)
		{
			float border = 4.0f * h;
			if (fabsf(near.s.y - h * 0.5f) < border)
			{
				Sample stub = near;
				stub.x = lo;
				stub.s.y = (near.y < 0) ? border : -border;
				DrawLineEx((Vector2) { near.s.x, near.s.y }, (Vector2) { stub.s.x, stub.s.y },
					GRAPH_THICK * SSAA, color);
			}
		}
	}

	refine_plot(p.plot_program, &vstack, v, w, h,
		sample(p.plot_program, &vstack, v, w, h, lo),
		sample(p.plot_program, &vstack, v, w, h, x_end),
		0, color);
}

void free_plot(Plot *p)
{
	free(p->plot_program);
	free(p->values);
	*p = (Plot){ 0 };
}

void update_plot(Input *input, bool is_periodic)
{
	if (validate(input->text, input->origin))
	{
		Plot new = plot(input->text);
		if (new.values)
		{
			free_plot(&input->plot); // previous
			input->plot = new;
			input->valid = true;
			input->periodic = is_periodic;
			return;
		}
	}
	// invalid input or malloc failure in plot()
	input->valid = false;
	input->periodic = false;
}

bool handle_panning(View *view, bool on_input)
{
	if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || on_input)
		return false;

	Vector2 delta = GetMouseDelta();
	view->x_offset -= delta.x / view->scale;
	view->y_offset += delta.y / view->scale;

	return (delta.x || delta.y);
}

bool handle_zooming(View *view, Vector2 mouse, int w, int h, bool on_input)
{
	float wheel = GetMouseWheelMove();
	if (wheel == 0.0f || on_input)
		return false;

	double factor = (wheel > 0) ? ZOOM_FACTOR : (1.0 / ZOOM_FACTOR);
	int   input_w = (int)(w / INPUTBOX_REL);

	Vector2 world_before = world_from_screen(*view, mouse.x - input_w, mouse.y, w - input_w, h);
	view->scale = CLAMP(view->scale * factor, 1e-4, 1e5);
	Vector2 world_after = world_from_screen(*view, mouse.x - input_w, mouse.y, w - input_w, h);

	view->x_offset += (world_before.x - world_after.x);
	view->y_offset += (world_before.y - world_after.y);

	return true;
}

bool handle_camera_reset(View *view)
{
	if (!IsKeyPressed(KEY_R))
		return false;

	if (IsKeyDown(KEY_LEFT_CONTROL))
		view->scale = INITIAL_SCALE;
	view->x_offset = 0.0;
	view->y_offset = 0.0;

	return true;
}