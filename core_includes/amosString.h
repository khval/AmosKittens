
extern struct stringData *alloc_amos_string( int size );
extern struct stringData *amos_strdup( struct stringData *var );
extern struct stringData *amos_strndup( struct stringData *var, int len );
extern struct stringData *amos_mid( struct stringData *var, int start, int size );
extern struct stringData *amos_right( struct stringData *var, int size );
extern int amos_instr( struct stringData *var,int start,struct stringData *find  );
extern struct stringData *toAmosString( const char *txt,int len);
extern struct stringData *toAmosString_char(char *adr, char t);				// Warning will count chars internally, use toAmosString(sdr,len) if you have a count.
extern struct stringData *toAmosString_len_or_char(char *adr, int len, char t);	// this function is for cropping,

#define allocNewString(len,newstr) newstr = (struct stringData *) sys_public_alloc( sizeof(struct stringData) + len ); 

