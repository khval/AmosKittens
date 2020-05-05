
struct ymlcfg
{
	std::string key;
	std::string value;
};

extern std::vector<ymlcfg> cfgs;

void dump_groups();
void add_group(unsigned int tabs, const char *first, const char *last );
bool load_config(const char *filename);
std::string *getConfigValue( std::string key );

