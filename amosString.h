
extern struct stringData *alloc_amos_string( int size );
extern struct stringData *amos_strdup( struct stringData *var );
extern struct stringData *amos_strndup( struct stringData *var, int len );
extern struct stringData *amos_mid( struct stringData *var, int start, int size );
extern struct stringData *amos_right( struct stringData *var, int size );
extern struct stringData *amos_instr( struct stringData *var,int start,struct stringData *find  );
extern struct stringData *toAmosString( const char *txt,int len);

