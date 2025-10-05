#ifndef DRAW_H
#define DRAW_H 1

#include <vita2d.h>

#define COLOR_WHITE ( RGBA8(255,255,255,255) )
#define COLOR_RED ( RGBA8(255,0,0,255) )
#define COLOR_BLUE ( RGBA8(0,255,0,255) )
#define COLOR_YELLOW ( RGBA8(255,255,0,255) )
#define COLOR_LIGHT_BLUE ( RGBA8(0,255,255,255 ) )
#define COLOR_GREEN ( RGBA8(0,0,255,255) )
#define COLOR_BLACK ( RGBA8(0,0,0,255) )

#define BUTTON_CROSS    "\xe2\x95\xb3"
#define BUTTON_CIRCLE   "\xe2\x97\x8b"
#define BUTTON_TRIANGLE "\xe2\x96\xb3"
#define BUTTON_SQUARE   "\xe2\x96\xa1"

#define FANCY_BUTTON_CROSS    "\xef\xa2\x80"
#define FANCY_BUTTON_CIRCLE   "\xef\xa2\x81"


void init_vita2d();
void term_vita2d();
void draw_background();
void start_draw();
void end_draw();
void draw_controls(uint8_t cancel) ;
void draw_texture(vita2d_texture* texture, int x, int y);
void draw_texture_center(int y, vita2d_texture* texture);
void draw_text_color(int x, int y, int color, const char* msg);
void draw_text(int x, int y, const char* msg);
void draw_text_center(int y, const char* msg);
void draw_text_center_color(int y, int color, const char* msg);
void draw_option(int y, const char* opt, int selected);
void draw_title(const char* msg);
void free_texture(vita2d_texture* texture);
void draw_progress_bar(int y, uint64_t done, uint64_t total);
vita2d_texture* load_texture(const char* path);

#endif // DRAW_H