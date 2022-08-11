
#define STMX 64
#define extensions_max 27

extern const char *config_name;
extern char *ST_str[STMX];
extern struct extension *extensions[extensions_max];
extern BOOL load_config( const char *name );
extern BOOL load_config_try_paths( char *filename);

