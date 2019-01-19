
#ifdef __amigaos4__
extern struct Process *main_task ;
#endif

#ifdef __linux__
extern int main_task ;		// pid number +1
#endif

extern struct Library 			 *AmosExtensionBase ;
extern struct AmosExtensionIFace	 *IAmosExtension ;

extern struct Library 		 *AslBase ;
extern struct AslIFace 		 *IAsl ;

extern BOOL init();
extern void closedown();

