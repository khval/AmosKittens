
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <stdint.h>
#include <stdbool.h>
#include "load_config.h"

std::vector<std::string> groups;

std::vector<ymlcfg> cfgs;

void add_group(unsigned int tabs, const char *first, const char *last )
{
	std::string tmp;
	const char *p;

	while (groups.size() > tabs)
	{
		groups.pop_back();
	}

	for (p=first;p<last;p++) tmp += *p ;
	groups.push_back(tmp);
}

void dump_groups()
{
	unsigned int i;

	for (i=0;i<groups.size();i++)
	{
		printf("%s%s", i ? "_" : "", groups[i].c_str());
	}
	printf("\n");
}

void getKey( std::string &out )
{
	unsigned int i;

	for (i=0;i<groups.size();i++)
	{
		out += (i ? "_" : "") + groups[i];
	}
}

std::string *getConfigValue( std::string key )
{
	unsigned int i;

	printf("*%s*\n",key.c_str());

	for (i=0;i<cfgs.size();i++)
	{
		if ( cfgs[i].key == key )
		{
			return &cfgs[i].value;
		}
	}
	return NULL;
}

int readYml( const char *str )
{
	ymlcfg entry;
	int tabs = 0;
	const char *p;
	const char *first = NULL;
	bool is_keyword = true;
	bool is_str = false;

	for (p = str;*p;p++)
	{
		if (is_keyword)
		{
			switch (*p)
			{
				case '#':	return 0;

				case '\t':	tabs++;
						break;

				case ':':	add_group(tabs, first, p);
						is_keyword = false;
						break;

				case ' ':	break;

				default:	if (first == NULL) first=p;
			}
		}
		else
		{
			switch (*p)
			{
				case '"':	is_str = ! is_str;	
						if (is_str == false)
						{
							getKey( entry.key );

							cfgs.push_back( entry );

							//	std::cout << value << '\n';
						}
						break;

				default:	if (is_str) entry.value += *p;	
						break;
			}
		}
	}
}

bool load_config(const char *filename)
{
	std::string line;
	const char *str;

	std::ifstream file(filename);
	if ( file.is_open() == false) return false;

	while ( getline(file, line ) )
	{
		str = line.c_str();
		
		readYml( str );
	}
}

