

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

//Code to strip invalid XML UTF-8 characters.

/*
 Legend:
  F == C0 control characters
  T == iso-8859-1 (ASCII or equivilent)
  I == iso-8859-1 (Extended character set)
  X == C1 control characters
  For a full explanation of control characters see:
    http://en.wikipedia.org/wiki/C0_and_C1_control_codes
*/
/*
F-0
T-1
X-2
I-3
*/
const char text_chars[256] = {
        /*                  BEL BS H1 L0    00 CR    */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,  /* 0x0X */
        /*                              ESC          */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x1X */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x2X */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x3X */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x4X */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x5X */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x6X */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  /* 0x7X */
        /*            NEL                            */
        2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  /* 0x8X */
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  /* 0x9X */
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xaX */
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xbX */
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xcX */
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xdX */
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xeX */
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3   /* 0xfX */
};
void validateXMLFile(const char *filename,const char *output_filename)
{
  ifstream fdr;
  ofstream fdw;
 
  fdr.open(filename);
  fdw.open(output_filename);

  //strip control chars from output
  size_t latinBufferSize = 0;

  if(fdr.is_open())
  {
	  while(!fdr.eof())
	  {
		  string line;
		  string output_line;
		  getline(fdr,line);
		  if(strstr(line.c_str(),"<TestUnicode")!=NULL)
			  ;//cout<<line<<endl;
		  for (size_t x=0;x<line.length();x++)
		  {
			  if (text_chars[(unsigned char)((line.c_str())[x])] != 0)
			  {
				  output_line += (line.c_str())[x];
			  }
		  }
		  fdw<<output_line<<endl;
		  line.clear();
		  output_line.clear();
		  //output the results (only the characters that passed the test)
		  //bufferPosition = bufferSize;
		  //fwrite(latinBuffer,1,latinBufferSize,stdout);
	  }
  }
}

