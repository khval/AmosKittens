
namespace amalBank
{
	int play( struct kittyChannel *self, int id );
}

class amalBankScript
{
	public:
		ULONG progs;
		char *prog_block;
		unsigned short *offsets;
		char *first_script;

	amalBankScript( char *start);
};

