#include "evaluator.h"
#include "tokenizer.h"

#include "raylib.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// misc
#define  MAX_EQUATIONS (200)
#define           SSAA (2)        // set this to 1 if you get framebuffer warnings
#define  INPUTLEN_INIT (64)

// window parameters
#define  INITIAL_WIDTH (1280)
#define INITIAL_HEIGHT (900)
// ratios
#define   INPUTBOX_REL (5)        // plots canvas : input column (width)
#define INPUT_HEAD_REL (0.05f)    // input column head : full input column (height)
#define    INPUT_H_REL (0.067f)   // single input field : full inputs column (height)
// sizes
#define   INPUTB_THICK (2) 
#define INPUTB_PADDING (4)

// plotting canvas parameters
#define   GRID_SPACING (140)
#define   MINORL_COUNT (3)
#define   MINORL_THICK (1)
#define     AXIS_THICK (3)
#define    GRAPH_THICK (3)
#define   MAJORL_THICK (2)
#define  INITIAL_SCALE (30.0)
#define    ZOOM_FACTOR (1.1)
#define  PERIODIC_FUNC (8.0f)     // see render_plot)(

// static color pallete
#define     BGND_COLOR ((Color){ 245, 245, 245, 255 })
#define   MJGRID_COLOR ((Color){ 180, 180, 180, 255 })
#define   MNGRID_COLOR ((Color){ 200, 200, 200, 255 })
#define     AXIS_COLOR ((Color){ 80, 80, 80, 255 })
#define     TEXT_COLOR ((Color){ 30, 30, 30, 255 })
#define INPUTBOX_COLOR ((Color){ 250, 250, 250, 255 })
#define  INPUTUP_COLOR ((Color){ 215, 215, 215, 255})
#define    ERASE_COLOR (INPUTBOX_COLOR)

// labels formatting
#define   MAX_SHORTVAL (1e5)
#define      FONT_SIZE (20.0f)

// adaptive funciton sampling
#define   MAX_RECDEPTH (55)    
#define   TOLERANCE_PX (0.35f)

#define CLAMP(val, min, max) \
	((val) < (min) ? (min) : (val) > (max) ? (max) : (val))

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

typedef struct
{
	Plot   plot;
	char   *text;
	size_t capacity;
	bool   valid;
	bool   periodic;
} Input;

static const Color color_cycle[] = {
	{  31, 119, 180, 255 }, // blue
	{ 255, 127,  14, 255 }, // orange
	{  44, 160,  44, 255 }, // green
	{ 214,  39,  40, 255 }, // red
	{ 148, 103, 189, 255 }, // purple
	{ 140,  86,  75, 255 }, // brown
	{ 227, 119, 194, 255 }, // pink
	{ 127, 127, 127, 255 }, // gray
	{ 188, 189,  34, 255 }, // olive
	{  23, 190, 207, 255 }  // cyan
};

inline Vector2 screen_from_world(View v, double x, double y, int w, int h)
{
	return (Vector2)
	{
		(float)(w * 0.5 + (x - v.x_offset) * v.scale),
			(float)(h * 0.5 - (y - v.y_offset) * v.scale)
	};
}

inline Vector2 world_from_screen(View v, float x, float y, int w, int h)
{
	return (Vector2)
	{
		(float)(v.x_offset + (x - w * 0.5) / v.scale),
			(float)(v.y_offset + (0.5 * h - y) / v.scale)
	};
}

Plot plot(char *valid_input)
{
	TokenData tokens = tokenize(valid_input);
	TokenData postfix = shunting_yard(tokens);

	double *values = malloc(sizeof(double) * postfix.size);
	if (!values) {
		free_tarray(postfix);
		return (Plot) { 0 };
	}

	Instr *program = prebake(postfix);
	return (Plot) {
		.plot_program = program,
		.plotp_length = postfix.size,
		.values = values
	};
}

static char *format(double scale, double number, double step)
{
	static char axis_label[50];

	int past_e = (int)floor(log10(fabs(step)));
	if (fabs(number) >= MAX_SHORTVAL && past_e >= 5 && scale < INITIAL_SCALE / 5)
	{
		float pre_e = (float)number / powf(10.0f, (float)past_e);
		snprintf(axis_label, sizeof(axis_label), "%.1fE%d", pre_e, past_e);
		return axis_label;
	}

	int dec = (step >= 1.0) ? 0 : (int)ceil(-log10(step));
	snprintf(axis_label, sizeof(axis_label), "%.*f", dec, number);

	return axis_label;
}

void render_grid(View v, Font f, int w, int h)
{
	double spacing = GRID_SPACING * SSAA / v.scale;
	double   order = floor(log10(spacing));
	double    norm = spacing / pow(10.0, order); // from 1 to 10
	double    mult = (norm < 1.5) ? 1.5 : (norm < 3.0) ? 3.0 : (norm < 7.0) ? 4.0 : 8.0;
	double   major = mult * pow(10.0, order);
	double   minor = major / MINORL_COUNT;

	float scmin_step = minor * v.scale;

	Vector2      center = screen_from_world(v, 0.0, 0.0, w, h);
	Vector2   top_right = world_from_screen(v, 0.0f, 0.0f, w, h);
	Vector2 bottom_left = world_from_screen(v, (float)w, (float)h, w, h);
	Vector2     current, next;

	// major + minor grids VERTICAL
	double x_start = floor(top_right.x / major) * major;
	current = screen_from_world(v, x_start, 0, w, h);

	for (double xi = x_start; xi <= bottom_left.x; xi += major)
	{
		next = screen_from_world(v, xi + major, 0, w, h);

		// minor
		for (float xj = current.x + scmin_step; xj < next.x; xj += scmin_step)
			DrawLineEx((Vector2) { xj, 0 }, (Vector2) { xj, h }, MINORL_THICK * SSAA, MNGRID_COLOR);

		// major (drawn over minor)
		DrawLineEx((Vector2) { current.x, 0 }, (Vector2) { current.x, h }, MAJORL_THICK * SSAA, MJGRID_COLOR);

		// don't print zero at (0; 0)
		if (fabs(xi) > 1e-12)
			DrawTextEx(f, format(v.scale, xi, major),
				(Vector2) {current.x - 4.0f * SSAA, center.y + 2.0f * SSAA},
				FONT_SIZE * SSAA, 0.0f, TEXT_COLOR);
		current = next;
	}

	// major + minor grids HORIZONTAL
	double y_start = floor(bottom_left.y / major) * major;
	current = screen_from_world(v, 0, y_start, w, h);

	for (double yi = y_start; yi <= top_right.y; yi += major)
	{
		next = screen_from_world(v, 0, yi + major, w, h);

		for (float yj = current.y + scmin_step; yj > next.y; yj -= scmin_step)
			DrawLineEx((Vector2) { 0, yj }, (Vector2) { w, yj }, MINORL_THICK * SSAA, MNGRID_COLOR);

		DrawLineEx((Vector2) { 0, current.y }, (Vector2) { w, current.y }, MAJORL_THICK * SSAA, MJGRID_COLOR);

		if (fabs(yi) > 1e-12)
			DrawTextEx(f, format(v.scale, yi, major),
				(Vector2) {center.x + 4.0f * SSAA, current.y - 8.0f * SSAA},
				FONT_SIZE * SSAA, 0.0f, TEXT_COLOR);
		current = next;
	}

	// main axes
	DrawLineEx((Vector2) { 0, center.y }, (Vector2) { w, center.y }, AXIS_THICK * SSAA, AXIS_COLOR);
	DrawLineEx((Vector2) { center.x, 0 }, (Vector2) { center.x, h }, AXIS_THICK * SSAA, AXIS_COLOR);
}

inline Sample sample(Instr *prog, ValueStack *vstack, View v, int w, int h, double x)
{
	double y = evaluate(prog, vstack, x);
	bool valid = isfinite(y);

	return (Sample) {
		.x = x,
			.y = y,
			.s = (Vector2){ (x - v.x_offset) * v.scale + w * 0.5,
				valid ? h * 0.5 - (y - v.y_offset) * v.scale : 0.0f },
			.valid = valid
	};
}

bool on_screen(Sample prev, Sample curr, int h)
{
	if (!prev.valid || !curr.valid)
		return false;

	float border = 4.0f * h;
	return !((prev.s.y > border || -prev.s.y > border) &&
		(curr.s.y > border || -curr.s.y > border));
}

void refine_plot(Instr *prog, ValueStack *vstack, View v, int w, int h, Sample prev, Sample curr, int depth, Color color)
{
	// degenerate sample
	if (!prev.valid && !curr.valid)
		return;

	// we can't see them so y elaborate more?
	if (depth > MAX_RECDEPTH / 5 && !on_screen(prev, curr, h))
		return;

	if (depth >= MAX_RECDEPTH)
	{
		if (on_screen(prev, curr, h))
			DrawLineEx((Vector2) { prev.s.x, prev.s.y }, (Vector2) { curr.s.x, curr.s.y }, GRAPH_THICK *SSAA, color);
		return;
	}

	double xm = (prev.x + curr.x) * 0.5;
	Sample mid = sample(prog, vstack, v, w, h, xm);

	bool split = (!prev.valid || !mid.valid || !curr.valid) ||
		fabsf(mid.s.y - (prev.s.y + curr.s.y) * 0.5f) > TOLERANCE_PX ||
		((prev.y > 0.0f) != (curr.y > 0.0f)); // ? should this fix 1/x

	if (on_screen(prev, curr, h) && !split)
	{
		DrawLineEx((Vector2) { prev.s.x, prev.s.y }, (Vector2) { curr.s.x, curr.s.y }, GRAPH_THICK *SSAA, color);
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

	// initially only 2 points on different sides of the
	// screen are needed (!not for sin/cos) as we're
	// going to make more with adaptive sampling
	double x_start = v.x_offset - (0.5 * w) / v.scale;
	double   x_end = v.x_offset + (0.5 * w) / v.scale;

	if (!is_periodic)
	{
		refine_plot(
			p.plot_program, &vstack, v, w, h,
			sample(p.plot_program, &vstack, v, w, h, x_start),
			sample(p.plot_program, &vstack, v, w, h, x_end),
			0, color
		);
		return;
	}

	// for sin(x)/cos(x) we re gonna need more initial samples
	// now thier amount becomes (width / PERIODIC_FUNC)
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
}

inline Rectangle erase_button(int w, int h, int s)
{
	float box_w = (float)(w / INPUTBOX_REL) * s;
	float bar_h = INPUT_HEAD_REL * (float)h * s;

	return (Rectangle){box_w * 0.69f, bar_h * 0.25f, box_w * 0.26f, bar_h * 0.5f};
}

inline Rectangle input_box(int w, int h, int s, size_t i)
{
	float box_w = (float)(w / INPUTBOX_REL) * s;
	float box_h = (float)h * INPUT_H_REL * s;

	// {out.width * 0.15f, out.y + box_h * 0.25f, out.width * 0.8, out.height * 0.5f};
	return (Rectangle) { 0, INPUT_HEAD_REL * s * h + i * box_h, box_w, box_h };
}

void render_menu(int w, int h, Font f, Input *inputs, size_t size, size_t active)
{
	float   font_size;
	Vector2 text_size;

	Rectangle main_box = (Rectangle){ 0, 0, (int)w / INPUTBOX_REL * SSAA, SSAA * h};
	Rectangle    upper = (Rectangle){ 0, 0, (int)w / INPUTBOX_REL * SSAA, INPUT_HEAD_REL * SSAA * h};
	Rectangle    erase = erase_button(w, h, SSAA);

	DrawRectangleRec(main_box, INPUTBOX_COLOR);
	DrawRectangleRec(upper, INPUTUP_COLOR);
	DrawRectangleRounded(erase, 0.90f, 0, ERASE_COLOR);

	// clear all is always centered in erase box
	font_size = upper.height * 0.35f;
	text_size = MeasureTextEx(f, "Clear all", font_size, 0.0f);
	DrawTextEx(f, "Clear all", (Vector2)
		{erase.x + (erase.width - text_size.x) * 0.5f,
		 erase.y + (erase.height - text_size.y) * 0.5f},
		font_size, 0.0f, TEXT_COLOR);

	font_size *= 1.3f;
	for (size_t i = 0; i < size; ++i)
	{
		// strcpy(inputs[i].text, "debug");
		Rectangle ci = input_box(w, h, SSAA, i);

		// same for every user input (line 332)
		text_size = MeasureTextEx(f, inputs[i].text, font_size, 0.0f);
		Vector2 fp = (Vector2){ ci.width * 0.14f, ci.y + (ci.height - text_size.y) * 0.5f};

		DrawRectangleLinesEx(ci, INPUTB_THICK * SSAA, i == active ? DARKGRAY : LIGHTGRAY);
		DrawTextEx(f, inputs[i].text, fp, font_size, 0.0f, TEXT_COLOR);
		if (inputs[i].valid)
			DrawCircle(ci.width * 0.075f, ci.y + ci.height * 0.5f, ci.height * 0.16f, color_cycle[i % 10]);
		else
		{
			Rectangle warning = (Rectangle){ ci.width * 0.073f, ci.y + ci.height * 0.3f, ci.height * 0.1f, ci.width * 0.05f };
			DrawRectangleRec(warning, ORANGE);
			DrawCircle(warning.x + warning.width * 0.52f, warning.y + warning.height * 1.45f, warning.width * 0.6f, ORANGE);
		}
	}
}

bool init(Font *font, RenderTexture2D *plots_cache, Input *inputs)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "cf2x");
	SetTargetFPS(60);

	Image icon = LoadImage("assets/icon.png");
	if (!icon.data)
		printf("Failed to load icon\n");
	else
	{
		ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		SetWindowIcon(icon);
		UnloadImage(icon);
	}
	
	*font = LoadFont("assets/LiberationSans-Regular.ttf");
	*plots_cache = LoadRenderTexture(INITIAL_WIDTH * SSAA, INITIAL_HEIGHT * SSAA);
	if (!plots_cache->id || !font->texture.id)
	{
		printf("Assets not loaded...\n");
		return false;
	}
	SetTextureFilter(font->texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(plots_cache->texture, TEXTURE_FILTER_BILINEAR);

	for (size_t i = 0; i < MAX_EQUATIONS; ++i)
	{
		inputs[i].text = malloc(INPUTLEN_INIT);
		if (!inputs[i].text)
			return false;
		// yea i know ideally we should free past successful
		// mallocs on failure but app is going to be exit(-1) anyway
		inputs[i].text[0] = '\0';
		inputs[i].capacity = INPUTLEN_INIT;
	}

	return true;
}

void free_plot(Plot *p)
{
	free(p->plot_program);
	free(p->values);
	*p = (Plot){ 0 };
}

void free_plots(size_t *size, Input *inputs)
{
	if (!inputs || !size)
		return;

	for (size_t i = 0; i < *size; ++i)
	{
		free_plot(&inputs[i].plot);
		inputs[i].text[0] = '\0';
		inputs[i].valid = false;
	}
}

void update_plot(Input *input, bool is_periodic)
{
	if (validate(input->text) == -1)
	{
		Plot new = plot(input->text);
		if (new.values)
		{
			free_plot(&input->plot); // previous
			input->plot = new;
			input->valid = true;
			input->periodic = is_periodic;
		}
	}
	else
		input->valid = false;
}

int main(void)
{
	Font font;
	RenderTexture2D plots_cache;
	Input inputs[MAX_EQUATIONS] = { 0 };

	if (!init(&font, &plots_cache, inputs))
		exit(-1);
	
	size_t  total = 1;
	size_t active = 0;

	bool changed = true;
	View view = { 0.0, 0.0, INITIAL_SCALE };

	while (!WindowShouldClose())
	{
		int w = GetScreenWidth();
		int h = GetScreenHeight();

		Vector2 mouse = GetMousePosition();
		bool on_input = mouse.x < (float)(w / INPUTBOX_REL);

		// pan
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) & !on_input)
		{
			Vector2 delta = GetMouseDelta();
			if (delta.x || delta.y)
				changed = true;
			view.x_offset -= delta.x / view.scale;
			view.y_offset += delta.y / view.scale;
		}

		// wheel zoom or scroll
		float wheel = GetMouseWheelMove();

		if (wheel != 0.0f && !on_input)
		{
			double factor = (wheel > 0) ? ZOOM_FACTOR : (1.0 / ZOOM_FACTOR);
			Vector2 world_before = world_from_screen(view, mouse.x, mouse.y, w, h);
			view.scale = CLAMP(view.scale * factor, 1e-5, 1e5);
			Vector2 world_after = world_from_screen(view, mouse.x, mouse.y, w, h);
			view.x_offset += (world_before.x - world_after.x);
			view.y_offset += (world_before.y - world_after.y);
			changed = true;
		}

		if (wheel != 0.0f && on_input)
		{
			// TODO implemet inputbox scrolling 
		}

		// mouse over input panel
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			// clear all pressed
			if (CheckCollisionPointRec(mouse, erase_button(w, h, 1.0f)))
			{
				free_plots(&total, inputs);
				active = 0;
				total = 1;
				changed = true;
			}

			else if (on_input)
			{
				for (size_t i = 0; i < total; ++i)
				{
					if (CheckCollisionPointRec(mouse, input_box(w, h, 1.0f, i)))
					{
						active = i;
						changed = true;
						break;
					}
				}
			}
		}

		// equations input
		int c;
		while ((c = GetCharPressed()) != 0)
		{
			size_t len = strlen(inputs[active].text);
			if (len + 1 >= inputs[active].capacity)
			{
				size_t new_len = inputs[active].capacity * 2;
				char *new_text = realloc(inputs[active].text, new_len);
				if (!new_text)
					break;

				inputs[active].text = new_text;
				inputs[active].capacity = new_len;
			}

			if (c >= 32 && c <= 125)
			{
				inputs[active].text[len] = (char)c;
				inputs[active].text[len + 1] = '\0';
				bool periodic = strstr(inputs[active].text, "sin") || strstr(inputs[active].text, "sin");
				update_plot(&inputs[active], periodic);
				changed = true;
			}
		}

		if (IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE))
		{
			size_t len = strlen(inputs[active].text);
			if (len > 0)
			{
				inputs[active].text[len - 1] = '\0';
				bool periodic = strstr(inputs[active].text, "sin") || strstr(inputs[active].text, "sin");
				update_plot(&inputs[active], periodic);
				changed = true;
			}
		}

		// add new entry
		if (active == total - 1 && IsKeyPressed(KEY_ENTER) && total < MAX_EQUATIONS - 1)
		{
			++active;
			++total;
			changed = true;
		}

		// camera reset
		if (IsKeyPressed(KEY_R))
		{
			if (IsKeyDown(KEY_LEFT_CONTROL))
				view.scale = INITIAL_SCALE;
			view.x_offset = 0.0f;
			view.y_offset = 0.0f;
			changed = true;
		}

		if (IsWindowResized())
		{
			UnloadRenderTexture(plots_cache);
			plots_cache = LoadRenderTexture(w * SSAA, h * SSAA);
			SetTextureFilter(plots_cache.texture, TEXTURE_FILTER_BILINEAR);
			changed = true;
		}

		if (changed)
		{
			View ssaaView = view;
			ssaaView.scale = view.scale * SSAA;
			int sW = (w + w / INPUTBOX_REL) * SSAA;
			int sH = h * SSAA;

			BeginTextureMode(plots_cache);
				ClearBackground(BGND_COLOR);
				render_grid(ssaaView, font, sW, sH);
				for (size_t i = 0; i < total; ++i)
				{
					if (!inputs[i].valid)
						continue;
					render_plot(inputs[i].plot, ssaaView, sW, sH, color_cycle[i % 10], inputs[i].periodic);
				}
				render_menu(w, h, font, inputs, total, active);
			EndTextureMode();

			changed = false;
		}

		BeginDrawing();
			DrawTexturePro(plots_cache.texture,
				(Rectangle) {0, 0, (float)(w * SSAA), -(float)(h * SSAA)},
				(Rectangle) {0, 0, (float)(w       ),  (float)(h       )},
				  (Vector2) {0, 0},
				   0.0f, WHITE);
		EndDrawing();
	}

	free_plots(&total, inputs);
	CloseWindow();

	return 0;
}
