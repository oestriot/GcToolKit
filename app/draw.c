#include "draw.h"
#include "ctrl.h"
#include "io.h"
#include <vita2d.h>
#include <vitasdk.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define IS_ASCII(c) (c < 0x7F)
#define IS_CODEPOINT_BEGIN(c) (!((c & 0x80) != 0) || ((c & 0x40) != 0))
#define IS_CODEPOINT_PART(c) !IS_CODEPOINT_BEGIN(c)

#define MENUOVERLAY_WIDTH (494)
#define MENUOVERLAY_HEIGHT (506) 

#define MENUOVERLAY_PAD (19)
#define MENUOVERLAY_POS_X ((SCREEN_WIDTH - MENUOVERLAY_WIDTH) - MENUOVERLAY_PAD)
#define MENUOVERLAY_POS_Y (MENUOVERLAY_PAD)

#define BUTTONOVERLAY_Y MENUOVERLAY_PAD + (MENUOVERLAY_HEIGHT - 10)
#define TITLE_Y (MENUOVERLAY_POS_Y + 35)

#define SCREEN_WIDTH (960)
#define SCREEN_HEIGHT (544)

#define TEXT_SIZE (1.0f)

static vita2d_pgf* pgf;
static vita2d_pvf* pvf;

static vita2d_texture* background;
static vita2d_texture* menuoverlay;


void init_vita2d() {
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0xFF, 0xFF, 0xFF, 0xFF));
	
	pgf = vita2d_load_default_pgf();
	pvf = vita2d_load_default_pvf();

	background = load_texture("app0:/res/background.png");
	menuoverlay = load_texture("app0:/res/menuoverlay.png");
}

void term_vita2d() {
	vita2d_fini();

	vita2d_free_pgf(pgf);
	vita2d_free_pvf(pvf);

	vita2d_free_texture(background);
	vita2d_free_texture(menuoverlay);
}

void draw_text_color(int x, int y, int color, const char* msg) {	
	vita2d_pvf_draw_text(pvf, x, y, color, TEXT_SIZE, msg);
}

void draw_text(int x, int y, const char* msg) {
	draw_text_color(x, y, COLOR_WHITE, msg);
}

void draw_progress_bar(int y, uint64_t done, uint64_t total) {
	int end = (MENUOVERLAY_WIDTH - ((MENUOVERLAY_PAD) * 2));
	int start = MENUOVERLAY_POS_X + MENUOVERLAY_PAD;
	int height = 32;
	
	int bar_px = (int)(((float)done / (float)total) * (float)end);
	
	vita2d_draw_rectangle(start, y, end, height, RGBA8(128,128,128,128));
	vita2d_draw_rectangle(start, y, bar_px, height, RGBA8(0,255,0,128));

}

void draw_text_center_color(int y, int color, const char* msg) {
	int text_width = MENUOVERLAY_WIDTH;
	int text_height = 0;	
	char processed_msg[MAX_PATH];
	
	// Basically we want to trim every message so it fits insize the MENUOVERLAY image,
	strncpy(processed_msg, msg, sizeof(processed_msg));
	int msg_len = strnlen(msg, sizeof(processed_msg)-1);
	
	do{
		vita2d_pvf_text_dimensions(pvf, TEXT_SIZE, processed_msg, &text_width, &text_height); // check size
		processed_msg[msg_len--] = 0;
	} while(msg_len > 0 && text_width > (MENUOVERLAY_WIDTH - (MENUOVERLAY_PAD * 2)));
	
	
	// Calculate the x position as center based on the menu overlay;
	int text_x = MENUOVERLAY_POS_X + (((MENUOVERLAY_WIDTH)/2) - (text_width/2));
	
	// draw this new message to the screen
	draw_text_color(text_x, y, color, processed_msg);
}

void draw_text_center(int y, const char* msg) {
	draw_text_center_color(y, COLOR_WHITE, msg);
}

void draw_option(int y, const char* opt, int selected) {
	if(!selected) {
		draw_text_center_color(y, COLOR_WHITE, opt);
	}
	else {
		draw_text_center_color(y, COLOR_YELLOW, opt);
	}
	
}

void draw_title(const char* title) {
	draw_text_center(TITLE_Y, title);	
}

void draw_texture_center(int y, vita2d_texture* texture) {
	int x = MENUOVERLAY_WIDTH / 2;	
	int w = vita2d_texture_get_width(texture)/2;
	
	draw_texture(texture, MENUOVERLAY_POS_X + (x - w), y);
}

void draw_controls(uint8_t cancel) {
	
	char ctrl_text[512];
	if(!cancel) {
	snprintf(ctrl_text, sizeof(ctrl_text), "%s OK", 
											(SCE_CTRL_CONFIRM == SCE_CTRL_CROSS) ? BUTTON_CROSS : BUTTON_CIRCLE);
	}
	else {
	snprintf(ctrl_text, sizeof(ctrl_text), "%s OK / %s BACK", 
											(SCE_CTRL_CONFIRM == SCE_CTRL_CROSS) ? BUTTON_CROSS : BUTTON_CIRCLE,
											(SCE_CTRL_CANCEL == SCE_CTRL_CIRCLE) ? BUTTON_CIRCLE : BUTTON_CROSS);
	}
	
	draw_text_center_color(BUTTONOVERLAY_Y, COLOR_LIGHT_BLUE, ctrl_text);
}

void draw_background() {
	draw_texture(background, 0, 0);
	draw_texture(menuoverlay, MENUOVERLAY_POS_X, MENUOVERLAY_POS_Y);	
}

void start_draw() {
	vita2d_start_drawing();
	vita2d_clear_screen();
}

void end_draw() {
	vita2d_end_drawing();
	vita2d_swap_buffers();	
}

void draw_texture(vita2d_texture* texture, int x, int y) {
	vita2d_draw_texture(texture, x, y);
}

void free_texture(vita2d_texture* texture) {
	vita2d_free_texture(texture);
}

vita2d_texture* load_texture(const char* path) {
	return vita2d_load_PNG_file(path);
}