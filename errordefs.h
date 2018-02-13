
// Custom error numbers
#define E_16C	0	// Only 16 colours allowed on non-AGA hires screen
#define E_UOS	1	// Unable to open screen
#define E_KS2	2	// Need Kickstart 2.0 or higher
#define E_UOW	3	// Unable to open window
#define E_CW0	4	// Window 0 can't be closed
#define E_MW0	5	// Window 0 can't be modified
#define E_WNO	6	// Window not opened
#define E_WTS	7	// Window too small
#define E_WTL	8	// Window too large
#define E_IWP	9	// Illegal window parameter
#define E_WNC	10	// Window not closed
#define E_NWB	11	// Unable to open Workbench

// These are standard errors, but we duplicate them here so our error trapper
// can catch them.
#define E_PI	12	// Program interrupted
#define E_IFC	13	// Illegal function call
#define E_OOM	14	// Out of memory
#define E_FNA	15	// Font not available
#define E_SNO	16	// Screen not opened
#define E_ISP	17	// Illegal screen parameter
#define E_INC	18	// Illegal number of colours

// More of our errors.
#define E_SNC	19	// Screen not closed
#define E_IND	20	// Icon not defined
#define E_NIB	21	// Icon bank not defined
#define E_NES	22	// Error text not available  [if prog compiled w/o error text]
#define E_OND	23	// Object not defined
#define E_NOB	24	// Object bank not defined
#define E_BWC	25  // Backward coordinates
#define E_MAA	26	// Menu already active
#define E_INT	27	// Internal error, code xxxxxxxx
#define E_IN2	28	// Internal error, code xxxxxxxx, subcode yyyyyyyy
#define E_NRT	29	// ReqTools.library version 2 or higher required
#define E_TMG	30	// Only 65535 gadgets allowed
#define E_GAA	31	// Gadget already active
#define E_WGT	32	// Wrong gadget type
#define E_GND	33	// Gadget not defined
#define E_GNR	34	// Gadget not reserved
#define E_ASN	35	// Valid AMOS screen numbers range from 0 to 7
#define E_BND	36	// Bank not defined
#define E_FNU	37	// Bank format not understood
#define E_IDT	38	// Inconsistent data
 
// Errors from IoErr(), etc. 
#define ERROR_NO_FREE_STORE			103
#define ERROR_TASK_TABLE_FULL		105
#define ERROR_LINE_TOO_LONG			120
#define ERROR_FILE_NOT_OBJECT		121
#define ERROR_INVALID_RESIDENT_LIBRARY	   122
#define ERROR_OBJECT_IN_USE		   	202
#define ERROR_OBJECT_EXISTS			203
#define ERROR_OBJECT_NOT_FOUND		205
#define ERROR_ACTION_NOT_KNOWN		209
#define ERROR_INVALID_COMPONENT_NAME	   210
#define ERROR_INVALID_LOCK		   		211
#define ERROR_OBJECT_WRONG_TYPE			212
#define ERROR_DISK_NOT_VALIDATED		213
#define ERROR_DISK_WRITE_PROTECTED		214
#define ERROR_RENAME_ACROSS_DEVICES		215
#define ERROR_DIRECTORY_NOT_EMPTY		216
#define ERROR_DEVICE_NOT_MOUNTED		218
#define ERROR_SEEK_ERROR				219
#define ERROR_COMMENT_TOO_BIG			220   
#define ERROR_DISK_FULL					221
#define ERROR_DELETE_PROTECTED			222
#define ERROR_WRITE_PROTECTED			223 
#define ERROR_READ_PROTECTED			224
#define ERROR_NOT_A_DOS_DISK			225
#define ERROR_NO_DISK					226
#define ERROR_NO_MORE_ENTRIES			232

// These are the return codes used by convention by AmigaDOS commands 
// See FAILAT and IF for relvance to EXECUTE files		     
#define RETURN_OK			0   // No problems, success 
#define RETURN_WARN			5   // A warning only 
#define RETURN_ERROR		10  // Something wrong 
#define RETURN_FAIL			20  // Complete or severe failure#define 