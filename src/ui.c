#include "../include/ui.h"
#include "../include/app.h"

static const Color color_cycle[] = {
	{  45, 117, 180, 255 }, // blue
	{ 214,  39,  40, 255 }, // red
	{  44, 160,  44, 255 }, // green
	{ 148, 103, 189, 255 }, // purple
	{ 255, 200, 0, 255 },   // yellow
	{   0,   0,   0, 255 }, // black
	{ 227, 119, 194, 255 }, // pink
	{ 127, 127, 127, 255 }, // gray
	{  23, 190, 207, 255 }, // cyan
	{ 140,  93,  62, 255 }  // brown
};

static Rectangle erase_button(int w, int h, int s)
{
	float box_w = (float)(w / INPUTBOX_REL) * s;
	float bar_h = INPUT_HEAD_REL * (float)h * s;

	return (Rectangle) { box_w * 0.69f, bar_h * 0.25f, box_w * 0.26f, bar_h * 0.5f };
}

static Rectangle input_box(int w, int h, int s, size_t i)
{
	float box_w = (float)(w / INPUTBOX_REL) * s;
	float box_h = (float)h * INPUT_H_REL * s;

	// {out.width * 0.15f, out.y + box_h * 0.25f, out.width * 0.8, out.height * 0.5f};
	return (Rectangle) { 0, INPUT_HEAD_REL *s *h + i * box_h, box_w, box_h };
}

void render_menu(int w, int h, Font f, Input *inputs, size_t size, size_t active)
{
	float   font_size;
	Vector2 text_size;

	Rectangle main_box = (Rectangle){ 0, 0, (int)w / INPUTBOX_REL * SSAA, SSAA * h };
	Rectangle    upper = (Rectangle){ 0, 0, (int)w / INPUTBOX_REL * SSAA, INPUT_HEAD_REL * SSAA * h };
	Rectangle    erase = erase_button(w, h, SSAA);

	DrawRectangleRec(main_box, INPUTBOX_COLOR);
	DrawRectangleRec(upper, INPUTUP_COLOR);
	DrawRectangleRounded(erase, 0.90f, 0, ERASE_COLOR);

	// clear all is always centered in erase box
	font_size = upper.height * 0.35f;
	text_size = MeasureTextEx(f, "Clear all", font_size, 0.0f);
	DrawTextEx(f, "Clear all", (Vector2)
	{
		erase.x + (erase.width - text_size.x) * 0.5f,
			erase.y + (erase.height - text_size.y) * 0.5f
	},
		font_size, 0.0f, TEXT_COLOR);

	font_size *= 1.3f;
	// blinking caret every half of a second
	bool caret = ((int)(GetTime() / 0.5) % 2 == 0);
	for (size_t i = 0; i < size; ++i)
	{
		Rectangle ci = input_box(w, h, SSAA, i);
		DrawRectangleLinesEx(ci, INPUTB_THICK * SSAA, i == active ? DARKGRAY : LIGHTGRAY);

		if (inputs[i].valid)
			DrawCircle(ci.width * 0.075f, ci.y + ci.height * 0.5f, ci.height * 0.16f, inputs[i].color);
		else
		{
			Rectangle warning = (Rectangle){ ci.width * 0.073f, ci.y + ci.height * 0.3f, ci.height * 0.1f, ci.width * 0.05f };
			DrawRectangleRec(warning, ORANGE);
			DrawCircle(warning.x + warning.width * 0.52f, warning.y + warning.height * 1.45f, warning.width * 0.6f, ORANGE);
		}

		// circle indicator will be between text and x_screen=0
		text_size = MeasureTextEx(f, inputs[i].origin, font_size, 0.0f);
		Vector2 font_pos = (Vector2){ ci.width * 0.14f, ci.y + (ci.height - text_size.y) * 0.5f };

		// turns out lib sans is proposional, so we can't char width * caret
		char until = inputs[i].origin[inputs[i].caret];
		inputs[i].origin[inputs[i].caret] = '\0';
		float caret_x = MeasureTextEx(f, inputs[i].origin, font_size, 0.0f).x;
		inputs[i].origin[inputs[i].caret] = until;

		// [font_pos.x, render_canvas_beginning]
		float visible_w = ci.width - font_pos.x - INPUTB_PADDING * SSAA;;
		if (visible_w <= 0.0f)
			visible_w = 0.0f;

		if (text_size.x <= visible_w) // doesn't go past input width
			inputs[i].scroll = 0.0f;
		else
		{
			if (caret_x - inputs[i].scroll > visible_w)
				inputs[i].scroll = caret_x - visible_w;
			if (caret_x - inputs[i].scroll < 0.0f)
				inputs[i].scroll = caret_x;
			if (inputs[i].scroll > text_size.x - visible_w)
				inputs[i].scroll = text_size.x - visible_w;
			if (inputs[i].scroll < 0.0f)
				inputs[i].scroll = 0.0f;
		}

		BeginScissorMode((int)font_pos.x - SSAA, (int)ci.y, (int)(ci.width - font_pos.x - INPUTB_PADDING * SSAA), (int)ci.height);
		DrawTextEx(f, inputs[i].origin,
			(Vector2) {
			font_pos.x - inputs[i].scroll, font_pos.y
		},
			font_size, 0.0f, TEXT_COLOR);

		// draw caret if input box is focused
		if (i == active && caret)
		{
			float caret_y = font_pos.y - SSAA;
			if (text_size.x == 0.0f)
				caret_y += (ci.height - MeasureTextEx(f, "|", font_size, 0.0f).y) * 0.5f - ci.height * 0.5f;

			Vector2 caret_pos = (Vector2){ font_pos.x - inputs[i].scroll + caret_x - 3.0f * SSAA, caret_y };
			DrawTextEx(f, "|", caret_pos, font_size, 0.0f, TEXT_COLOR);
		}
		EndScissorMode();
	}
}

bool handle_input_click(Input *inputs, Vector2 mouse, int w, int h,
	size_t *total, size_t *active, size_t *next_color, bool on_input)
{
	if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		return false;

	if (CheckCollisionPointRec(mouse, erase_button(w, h, 1.0f)))
	{
		free_plots(total, inputs);
		*active = 0;
		*total = 1;
		*next_color = 1;
		return true;
	}

	if (!on_input)
		return false;

	// determine what input should be active if clicked
	for (size_t i = 0; i < *total; ++i)
	{
		if (CheckCollisionPointRec(mouse, input_box(w, h, 1.0f, i)))
		{
			*active = i;
			return true;
		}
	}

	return false;
}

bool handle_input_typing(Input *inputs, size_t active)
{
	int c = GetCharPressed();
	if (c == 0 || c == 114) // 114 == 'r', reserved for camera reset
		return false;

	// resize if exceeding malloc'd size
	size_t len = strlen(inputs[active].origin);
	if (len + 1 >= inputs[active].capacity)
	{
		size_t new_len = inputs[active].capacity * 2;

		char *new_text = realloc(inputs[active].text, new_len);
		if (!new_text)
			return false;
		inputs[active].text = new_text;

		char *new_orig = realloc(inputs[active].origin, new_len);
		if (!new_text || !new_orig)
			return false;
		inputs[active].origin = new_orig;

		inputs[active].capacity = new_len;
	}

	if (c >= 32 && c <= 122)
	{
		Input *inp = &(inputs[active]);

		// shift both text fields and insert at caret position
		memmove(inp->origin + inp->caret + 1, inp->origin + inp->caret, len - inp->caret + 1);
		inp->origin[inp->caret] = (char)c;
		++(inp->caret);

		bool periodic = strchr(inputs[active].origin, 'n') || strchr(inputs[active].origin, 'o');
		update_plot(&inputs[active], periodic);
		return true;
	}

	return false;
}

bool handle_input_delete(Input *inputs, size_t active)
{
	if (!IsKeyPressedRepeat(KEY_BACKSPACE) && !IsKeyPressed(KEY_BACKSPACE))
		return false;

	size_t len = strlen(inputs[active].origin);
	if (len == 0 || inputs[active].caret == 0)
		return false;

	Input *inp = &(inputs[active]);
	// delete at caret
	memmove(inp->origin + inp->caret - 1, inp->origin + inp->caret, len - inp->caret + 1);
	--(inp->caret);

	bool periodic = strchr(inputs[active].origin, 'n') || strchr(inputs[active].origin, 'o');
	update_plot(&inputs[active], periodic);

	return true;
}

bool handle_inputs_navigation(Input *inputs, size_t *active, size_t total)
{
	if (!active || total == 0)
		return false;

	Input *inp = &inputs[*active];

	if ((IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) && inp->caret)
		return --(inp->caret), true;

	if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) &&
		inp->caret < strlen(inp->origin))
		return ++(inp->caret), true;

	if ((IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) && *active > 0)
		return --(*active), true;

	if ((IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) &&
		*active + 1 < total)
		return ++(*active), true;

	return false;
}

bool handle_inputs_adding(Input *inputs, size_t *next_color, size_t *active, size_t *total)
{
	if (!IsKeyPressed(KEY_ENTER) || *total >= MAX_EQUATIONS)
		return false;

	// the idea is to shift everything to the end and
	// assign newly added enrty to the last one
	// (which exists but is unused)
	char *free_text = inputs[*total].text;
	char *free_orig = inputs[*total].origin;
	size_t free_cap = inputs[*total].capacity;

	for (size_t i = *total; i > (*active) + 1; --i)
		inputs[i] = inputs[i - 1];

	// no shallow copy
	inputs[*active + 1].plot = (Plot){ 0 };
	inputs[*active + 1].text = free_text;
	inputs[*active + 1].origin = free_orig;
	inputs[*active + 1].capacity = free_cap;
	inputs[*active + 1].text[0] = '\0';
	inputs[*active + 1].origin[0] = '\0';
	inputs[*active + 1].caret = 0;
	inputs[*active + 1].scroll = 0.0f;
	inputs[*active + 1].valid = false;
	inputs[*active + 1].periodic = false;
	inputs[*active + 1].color = color_cycle[(*next_color) % 10];

	++(*next_color);
	++(*active);
	++(*total);

	return true;
}