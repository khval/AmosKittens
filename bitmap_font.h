
struct TextFont *open_font( char const *filename, int size );
void draw_glyph(struct retroScreen *screen, struct TextFont *font, int rp_x, int rp_y, int glyph, int pen);
void draw_char(struct retroScreen *screen, struct retroTextWindow *textWindow, int lX, int lY, char c, int pen, int paper );
void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

