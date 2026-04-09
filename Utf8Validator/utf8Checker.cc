//UTF-8 correction program that validates the repository.xml written out.

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "utf8.h"

using namespace std;
ofstream ofs;
//ofstream los; 
bool valid_utf8_file(const char* fileName)
{
	ifstream ifs(fileName);
	if(!ifs)
		return false;
	istreambuf_iterator<char> it(ifs.rdbuf());
	istreambuf_iterator<char> eos;
	ifs.close();
	return utf8::is_valid(it,eos);
}

void fix_utf8_string(std::string& str,ofstream &newfile)
{
	static unsigned long line=0;
	std::string temp;
	utf8::replace_invalid(str.begin(), str.end(), back_inserter(temp),'!');
	/*if(temp.compare(str))
	{
		los<<"<Line>"<<line<<"</Line>"<<endl;
	}
	*/
	line++;
	newfile<<temp<<endl;
}



//__declspec(dllexport) int utfchecker(string srcXML,string destXML)
int utfchecker(string srcXML,string destXML)
{
	bool valid;

	
	ifstream ifs;
	ifs.open(srcXML.c_str());
	if(ifs.fail())
	   return -1;

	valid=valid_utf8_file(srcXML.c_str());	
	string line;

	ofs.open(destXML.c_str());
	
	//string logFileName=string("utf8Logger_")+string(destXML);
	//los.open(logFileName.c_str());


	//los<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"<<endl;
	//los<<"<?xml-stylesheet type=\"text/xsl\"?>"<<endl;
	//los<<"<UTF8_VALIDATE>"<<endl;

	if(ifs.is_open())
	{
		while(!ifs.eof())
		{
			getline(ifs,line);
			fix_utf8_string(line,ofs);
			line.clear();
		}
	}

	//los<<"</UTF8_VALIDATE>"<<endl;
	ifs.close();
	ofs.close();
	//los.close();
	_unlink(srcXML.c_str());

	return 0;


}

#ifdef _DEBUG
/*
int main(int argc,char *argv[])
{

	utfchecker(argv[1],argv[2]);
	return 0;
}
*/
#endif