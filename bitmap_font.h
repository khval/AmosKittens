
struct TextFont *open_font( char const *filename, int size );
short draw_glyph(struct RastPort *rp, struct TextFont *font, int rp_x, int rp_y, int glyph);
void _my_print_text(struct retroScreen *screen, char *text);


