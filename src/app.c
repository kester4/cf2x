#include "../include/app.h"
float  FONT_SIZE = 20.0f;
int GRID_SPACING = 140;
int MINORL_THICK = 1;
int MAJORL_THICK = 2;
int  GRAPH_THICK = 3;
int   AXIS_THICK = 3;

bool init_app(Font *font, RenderTexture2D *plots_cache, Input *inputs)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#ifdef HIGH_DPI
	SetConfigFlags(FLAG_WINDOW_HIGHDPI);
#endif
	InitWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "cf2x");
	SetTargetFPS(60);
	
#ifdef RELEASE_BUILD
	int mon_h = GetMonitorPhysicalHeight(GetCurrentMonitor());
	if (mon_h < 200)
	{
		float mult = 381.0f / (float)mon_h;
		FONT_SIZE *= mult;
		GRID_SPACING *= mult;
		MINORL_THICK *= mult;
		MAJORL_THICK *= mult;
		GRAPH_THICK *= mult;
		AXIS_THICK *= mult;
	}
#endif

	char image_path[50], font_path[50];
#if defined(RELEASE_BUILD) && defined(__linux__)
	strcpy(image_path, "/usr/local/share/cf2x/cf2x.png");
	strcpy(font_path,  "/usr/local/share/cf2x/LiberationSans-Regular.ttf");
#else
	strcpy(image_path, "./assets/cf2x.png");
	strcpy(font_path, "./assets/LiberationSans-Regular.ttf");
#endif

	Image icon = LoadImage(image_path);
	if (!icon.data)
		printf("[!] Failed to load icon!\n");
	else
	{
		ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		SetWindowIcon(icon);
		UnloadImage(icon);
	}

	*font = LoadFont(font_path);
	if (font->texture.id == 0)
		printf("[!] Missing fonts!\n");
	else
		SetTextureFilter(font->texture, TEXTURE_FILTER_BILINEAR);

	*plots_cache = LoadRenderTexture(INITIAL_WIDTH * SSAA, INITIAL_HEIGHT * SSAA);
	if (plots_cache->id == 0)
	{
		free_exit(NULL, inputs, plots_cache, font);
		printf("[!!!] Error loading render texture (try setting SSAA to 1), exiting...\n");
		return false;
	}
	SetTextureFilter(plots_cache->texture, TEXTURE_FILTER_BILINEAR);

	for (size_t i = 0; i < MAX_EQUATIONS; ++i)
	{
		inputs[i].text = malloc(INPUTLEN_INIT);
		inputs[i].origin = malloc(INPUTLEN_INIT);
		if (!inputs[i].text || !inputs[i].origin)
		{
			free_exit(NULL, inputs, plots_cache, font);
			return false;
		}

		inputs[i].text[0] = '\0';
		inputs[i].origin[0] = '\0';
		inputs[i].capacity = INPUTLEN_INIT;
	}

	inputs[0].color = (Color){ 45, 117, 180, 255 };
	return true;
}

void free_plots(size_t *size, Input *inputs)
{
	if (!inputs || !size)
		return;

	for (size_t i = 0; i < *size; ++i)
	{
		free_plot(&inputs[i].plot);
		inputs[i].text[0] = '\0';
		inputs[i].origin[0] = '\0';
		inputs[i].valid = false;
		inputs[i].caret = 0;
		inputs[i].scroll = 0.0f;
	}
}

bool resize_plot_cache(RenderTexture2D *plots_cache, int w, int h)
{
	UnloadRenderTexture(*plots_cache);
	*plots_cache = LoadRenderTexture(w * SSAA, h * SSAA);
	if (plots_cache->texture.id == 0)
	{
		printf("[!!!] Error loading render texture (try setting SSAA to 1), exiting...\n");
		return false;
	}
	SetTextureFilter(plots_cache->texture, TEXTURE_FILTER_BILINEAR);

	return true;
}

void rerender_plots(Input *inputs, RenderTexture2D plots_cache,
	View view, Font font, size_t total, int w, int h, bool light)
{
	View ssaaView = view;
	ssaaView.scale = view.scale * SSAA;

	int  input_w = (w / INPUTBOX_REL) * SSAA;
	int canvas_w = w * SSAA - input_w;
	int sH = h * SSAA;

	BeginTextureMode(plots_cache);
	ClearBackground(light ? BGND_LIGHT : BGND_DARK);

	rlPushMatrix();
	rlTranslatef((float)input_w, 0.0f, 0.0f);
	render_grid(ssaaView, font, canvas_w, sH, light);
	for (size_t i = 0; i < total; ++i)
	{
		if (!inputs[i].valid)
			continue;
		render_plot(inputs[i].plot, ssaaView, canvas_w, sH, inputs[i].color, inputs[i].periodic);
	}
	rlPopMatrix();

	EndTextureMode();
}

void render_frame(Input *inputs, RenderTexture2D plots_cache,
	int w, int h, bool light, Font font, size_t active, size_t total)
{
	BeginTextureMode(plots_cache);
	render_menu(w, h, font, inputs, total, active, light);
	EndTextureMode();

	BeginDrawing();
	DrawTexturePro(plots_cache.texture,
		(Rectangle) { 0, 0, (float)(w * SSAA), -(float)(h * SSAA) },
		(Rectangle) { 0, 0, (float)(w), (float)(h) },
		(Vector2) { 0, 0 },
		0.0f, WHITE);
	EndDrawing();
}

void free_exit(size_t *total, Input *inputs, RenderTexture2D *plots_cache, Font *font)
{
	if (total)
		free_plots(total, inputs);
	for (size_t i = 0; total && i < MAX_EQUATIONS; ++i)
	{
		free(inputs[i].text);
		free(inputs[i].origin);
	}
	if (font && font->texture.id)
		UnloadFont(*font);
	if (plots_cache && plots_cache->texture.id)
		UnloadRenderTexture(*plots_cache);
}
