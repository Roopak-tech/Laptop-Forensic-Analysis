/*REFERENCES in no particular order (all though the first one is the best)
 *
 *http://www.tbray.org/ongoing/When/200x/2003/04/26/UTF
 *http://en.wikipedia.org/wiki/UTF-8 
 *http://www.cl.cam.ac.uk/~mgk25/unicode.html
 *http://www.utf8-chartable.de/unicode-utf8-table.pl
 *http://www.w3.org/TR/xml11/#NT-Nmtoken 
 *http://www.rfc-editor.org/rfc/rfc3629.txt
 *http://unicode.org/faq/utf_bom.html#utf16-2
 *http://www.dpawson.co.uk/xsl/sect2/N7150.html 
 *http://www.gnu.org/s/libiconv/#TOCintroduction
 *http://www.w3.org/TR/xml/
 *http://boodebr.org/main/python/all-about-python-and-unicode#UNI_XML 
 *http://www.deanlee.cn/programming/convert-unicode-to-utf8/
 *http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightLinear
 *http://unix.stackexchange.com/questions/12273/in-bash-how-can-i-convert-a-unicode-codepoint-0-9a-f-into-the-printabale-chara
 *http://stackoverflow.com/questions/4322191/how-many-bytes-do-we-need-to-store-an-arabic-character
 *http://code.swapnonil.com/2010/10/31/saving-only-real-unicode-characters-in-xml/
 *http://www.w3.org/International/questions/qa-forms-utf-8.en.php 
 *http://en.wikipedia.org/wiki/Mojibake
 *http://forums.mysql.com/read.php?107,433100,433785#msg-433785
 *http://www.unicode.org/charts/
 *http://lethain.com/stripping-illegal-characters-from-xml-in-python/
 *http://www.opensource.apple.com/source/WebCore/WebCore-1C25/icu/unicode/utf.h
 *http://en.wikipedia.org/wiki/Unicode_equivalence
 *http://www.w3.org/International/charlint/
 *http://unicode.org/reports/tr15/
 */

#include <stdio.h>
#include <stdlib.h>

#include <iostream> 
using namespace std;

#define BUFSIZE 4096

#define THREEBYTES 240
#define TWOBYTES 224
#define ONEBYTE 192
#define NOBYTES 0

#define SUCCESS 1
#define FAILED 0
#define VALID 1
#define INVALID 0
#define SURROGATE 1
#define NOT_SURROGATE 0

#define REPLACEMENT_CHARACTER "\xef\xbf\xbd"


static unsigned char buffer[BUFSIZE];

int BytesToRead(const unsigned char ubyte)
{
    if((ubyte&THREEBYTES) == THREEBYTES ) return 3;
    if((ubyte&TWOBYTES) == TWOBYTES ) return 2;
    if((ubyte&ONEBYTE) == ONEBYTE ) return 1;
    if((ubyte&NOBYTES) == NOBYTES ) return 0;
}
/* Valid UNICODE UTF 8 characters 

   [\09\0A\0D\x20-\x7E]                 # ASCII

   | [\xC2-\xDF][\x80-\xBF]             # non-overlong 2-byte

   |  \xE0[\xA0-\xBF][\x80-\xBF]        # excluding overlongs

   | [\xE1-\xEC\xEE\xEF][\x80-\xBF]{2}  # straight 3-byte

   |  \xED[\x80-\x9F][\x80-\xBF]        # excluding surrogates

   |  \xF0[\x90-\xBF][\x80-\xBF]{2}     # planes 1-3

   | [\xF1-\xF3][\x80-\xBF]{3}          # planes 4-15

   |  \xF4[\x80-\x8F][\x80-\xBF]{2}     # plane 16
*/


/*
 * isOverLong()
 * 
 *
 */


int isSurrogate(const unsigned char utf8_buffer[])/*U+D800-U+DFFF - 2048 code points*/
{
    
    if(  (utf8_buffer[0] == 0xED) && (utf8_buffer[1]>= 0xA0  && utf8_buffer[1]<= 0xBF) && ( utf8_buffer[2] >= 0x80 && utf8_buffer[2] <= 0xBF))
        return SURROGATE;
    return NOT_SURROGATE;
}

int ByteInRange(const unsigned char byte,int lb,int ub)
{
    if(lb<=byte && byte>=ub)
        return 1;
    return 0;
}


//http://www.w3.org/TR/xml/ - Section 2.2
//http://en.wikipedia.org/wiki/Valid_characters_in_XML
//http://software.hixie.ch/utilities/cgi/unicode-decoder/character-identifier?keywords=unicode

int isValidXMLUTF8Character(const unsigned char utf8_buffer[],int num_utf8_bytes)
{
    switch(num_utf8_bytes)
    {
    //Avoid Control characters 
    //[#x7F-#x84], [#x86-#x9F]
        case 1:
            if(utf8_buffer[0] == 0x7F)
            {
                return INVALID;
            }
        case 2:
            if( (utf8_buffer[0] == 0xC2)  && (utf8_buffer[1]== 0x80 ||utf8_buffer[1]== 0x81 ||utf8_buffer[1]== 0x82 ||\
                                              utf8_buffer[1]== 0x83 ||utf8_buffer[1]== 0x84))
            {
                return INVALID;
            }
           // [#xFDD0-#xFDEF] After arabic
        case 3:
            if( (utf8_buffer[0] == 0xEF)  && (utf8_buffer[1]== 0xB7) && (utf8_buffer[2] >=  0x90 && utf8_buffer[2] <= 0xAF))
            {
                return INVALID;
            }
            /*
            [#x1FFFE-#x1FFFF], [#x2FFFE-#x2FFFF], [#x3FFFE-#x3FFFF],
                [#x4FFFE-#x4FFFF], [#x5FFFE-#x5FFFF], [#x6FFFE-#x6FFFF],
                [#x7FFFE-#x7FFFF], [#x8FFFE-#x8FFFF], [#x9FFFE-#x9FFFF],
                [#xAFFFE-#xAFFFF], [#xBFFFE-#xBFFFF], [#xCFFFE-#xCFFFF],
                [#xDFFFE-#xDFFFF], [#xEFFFE-#xEFFFF], [#xFFFFE-#xFFFFF],
                [#x10FFFE-#x10FFFF]
                */
        case 4:
            if( ByteInRange(utf8_buffer[0],0xF0,0xF3)  &&\
                ByteInRange(utf8_buffer[1],0x8F,0xBF)  &&\
                (utf8_buffer[2] == 0xBF)               &&\
                ByteInRange(utf8_buffer[3],0xBE,0xBF))
            {
                return INVALID;
            }

    };
    return VALID;
}


int isValidUTF8Character(const unsigned char utf8_buffer[],int num_utf8_bytes)
{

    
    switch(num_utf8_bytes)
    {
        case 1:
            if(isValidXMLUTF8Character(utf8_buffer,num_utf8_bytes) == INVALID)
                return INVALID;
            if( (0x20<=utf8_buffer[0]) && (utf8_buffer[0]<=0x7F))
            {
                return VALID;
            }
            else if((utf8_buffer[0] == 0x09) || (utf8_buffer[0] == 0x0A) || (utf8_buffer[0] == 0x0D))
            {
                return VALID;
            }
            break;
        case 2:
            if(isValidXMLUTF8Character(utf8_buffer,num_utf8_bytes) == INVALID)
                return INVALID;
            if(  (utf8_buffer[0]>=0xC2 &&  utf8_buffer[0]<=0xDF) && (utf8_buffer[1]>=0x80 && utf8_buffer[1]<=0xBF) ) 
            {
                return VALID;
            }
            break;
        case 3:
            if(isValidXMLUTF8Character(utf8_buffer,num_utf8_bytes) == INVALID)
                return INVALID;
            if(isSurrogate(utf8_buffer) == SURROGATE)
            {
                return INVALID;
            }
            if( (utf8_buffer[0]>=0xE1 && utf8_buffer[0]<=0xEF) && (utf8_buffer[1]>=0x80 && utf8_buffer[1]<=0xBF)\
                && (utf8_buffer[2]>=0x80 && utf8_buffer[2]<=0xBF) )
            {
                return VALID;
            }
            break;
        case 4:
            if(isValidXMLUTF8Character(utf8_buffer,num_utf8_bytes) == INVALID)
                return INVALID;
            //  \xF0[\x90-\xBF][\x80-\xBF][\x80-\xBF]    # Astral planes 1-3
            if( (utf8_buffer[0] == 0xF0) && (utf8_buffer[1]>=0x90 && utf8_buffer[1]<=0xBF) && (utf8_buffer[2]>=0x80 && utf8_buffer[2]<=0xBF)\
                && (utf8_buffer[3]>=0x80 && utf8_buffer[3]<=0xBF)  )
            {
                return VALID;
            }
            // [\xF1-\xF3][\x80-\xBF]{3}          #Astral planes 4-15
            if( (utf8_buffer[0] >= 0xF1 && utf8_buffer[0] <= 0xF3) && (utf8_buffer[1]>=0x80 && utf8_buffer[1]<=0xBF)\
                && (utf8_buffer[2]>=0x80 && utf8_buffer[2]<=0xBF)\
                && (utf8_buffer[3]>=0x80 && utf8_buffer[3]<=0xBF))
            {
                return VALID;
            }
            //  \xF4[\x80-\x8F][\x80-\xBF]{2}     #Astral plane 16
            if( (utf8_buffer[0] == 0xF4) && (utf8_buffer[1]>=0x80 && utf8_buffer[1]<=0x8F)\
                && (utf8_buffer[2]>=0x80 && utf8_buffer[2]<=0xBF)\
                && (utf8_buffer[3]>=0x80 && utf8_buffer[3]<=0xBF))
            {
                return VALID;
            }

    };

    return INVALID;

}


int unicode_codepoints_to_utf8_bytes(char *&unicode_buffer)
{
    wchar_t codepoint[3];
    wchar_t unicode_bufferW[20];
    memset(codepoint,0,sizeof(wchar_t)*3);
    memset(unicode_bufferW,0,sizeof(wchar_t)*20);

    mbstowcs(unicode_bufferW,unicode_buffer,20);        
    swscanf(unicode_bufferW,L"%s",codepoint);
    //mbstowcs(codepoint,unicode_buffer,3);        
    wprintf(L"%s\n",unicode_bufferW);


    for(int i=0;i<wcslen(codepoint);i++)
    {
        wprintf(L"%lc\n",codepoint[i]);
    }
    printf("\n");
    


    

    return 0;


}


int utf8_to_unicode_codepoints(unsigned char *&utf8_buffer,int num_utf8_bytes,char *&unicode_buffer) //Useful function for debugging never called at the moment
{
    //fprintf(stdout,"Converting from utf8 to unicode_codepoints for %d bytes\n",num_utf8_bytes);
    unsigned char codepoint=0;

    unsigned char codepointA=0;
    unsigned char codepointB=0;
    unsigned char codepointC=0;
    int utf8_counter = num_utf8_bytes;

    if(num_utf8_bytes == 1)//Takes care of all the ASCII   U+0000 - U+007F
    {
        codepointB=utf8_buffer[0]&0x007F;
        sprintf(unicode_buffer,"U+%02X%02X",codepointA,codepointB);
        cout<<unicode_buffer<<" "<<utf8_buffer<<" ";
        for(int k=0;k<utf8_counter;k++)
        {     
            fprintf(stdout,"\\x%x",utf8_buffer[k]);
        }
        cout<<endl;
        return SUCCESS;

    }

    else if(num_utf8_bytes == 2)//Takes care of range U+0080 - U+07FF
    {
        codepointA = (utf8_buffer[0] & 0x38)>>2;
        codepointB = ( ((utf8_buffer[0] & 0x03) << 6) | (utf8_buffer[1] &  0x3F));
        sprintf(unicode_buffer,"U+%02X%02X",codepointA,codepointB);
        cout<<unicode_buffer<<" "<<utf8_buffer<<" ";
        for(int k=0;k<utf8_counter;k++)
        {     
            fprintf(stdout,"\\x%x",utf8_buffer[k]);
        }
        cout<<endl;
        return SUCCESS;

    }

    else if(num_utf8_bytes == 3)//Takes care of the range U+0800 - U+FFFF
    {
        codepointA = (( (utf8_buffer[0]&0x0F) << 4) | ((utf8_buffer[1]&0x3C) >> 2) ); 
        codepointB = ( ((utf8_buffer[2]&0x3F)) | ((utf8_buffer[1]&0x03) << 6) );
        sprintf(unicode_buffer,"U+%02X%02X",codepointA,codepointB);
        cout<<unicode_buffer<<" "<<utf8_buffer<<" ";
        for(int k=0;k<utf8_counter;k++)
        {     
            fprintf(stdout,"\\x%x",utf8_buffer[k]);
        }
        cout<<endl;
        return SUCCESS;

    }
    
    else if(num_utf8_bytes == 4)//Takes care of the range U+0800 - U+FFFF
    {
        codepointA = (( (utf8_buffer[0]&0x07) << 2) | ((utf8_buffer[1]&0x30) >> 4) ); 
        codepointB = (( (utf8_buffer[1]&0x0F) << 4) | ((utf8_buffer[2]&0x3C) >> 2) ); 
        codepointC = ( ((utf8_buffer[3]&0x3F)) | ((utf8_buffer[2]&0x03) << 6) );
        sprintf(unicode_buffer,"U+%02X%02X%02X",codepointA,codepointB,codepointC);
        cout<<unicode_buffer<<" "<<utf8_buffer<<" ";
        for(int k=0;k<utf8_counter;k++)
        {     
            fprintf(stdout,"\\x%x",utf8_buffer[k]);
        }
        cout<<endl;
        return SUCCESS;

    }
    
    cerr<<"Number of utf-8 bytes to decode is "<<num_utf8_bytes<<endl;
    return FAILED;
}

int globalSlack=0;//HACK ALERT

int sanitize_utf8(const unsigned char *inbuffer,string &outbuffer,int &madeSane,FILE *&fdr,FILE *&fdw,FILE *&fdc)
{
    //printf("In sanitize_utf8\n");

    int counter=0;
    int bytesToRead=0;
    int utf8_counter=0;
    char *unicode_buffer=NULL;
    unsigned char *utf8_buffer=NULL;
    static int bytePosBase=0;
    int slack=0;

    while(counter<BUFSIZE && inbuffer[counter])
    {
        bytesToRead = BytesToRead(inbuffer[counter]);
        //fprintf(stdout,"Bytes to Read %d\n",bytesToRead);
        slack = (counter + 1 + bytesToRead) - BUFSIZE;

        unicode_buffer = new char[bytesToRead+1];
        memset(unicode_buffer,0,(bytesToRead+1) * sizeof(char));
        utf8_buffer = new unsigned char[bytesToRead+1];
        memset(utf8_buffer,0,(bytesToRead + 1) * sizeof(unsigned char));
        


        utf8_counter=0;

        if(slack > 0)//Implies we need to read slack number of bytes from the file and return to read BUFSIZE more
        {
			
            globalSlack+=slack;
            for(int i=counter;i<BUFSIZE;i++)
            {
                utf8_buffer[utf8_counter]=inbuffer[i];
                utf8_counter++;
            }
            while(slack)
            {
                //printf("Reading  utf8_buffer\n");
                fread(&utf8_buffer[utf8_counter],1,1,fdr);
                //printf("Done reading utf8_buffer\n");
                utf8_counter++;
                slack--;
            }
        }
        else//Usually the case
        {
            for(int i=counter;i<counter+bytesToRead+1;i++)
            {
                utf8_buffer[utf8_counter]=inbuffer[i];
                utf8_counter++;
            }
        }

        //printf("utf8_counter %d\n",utf8_counter);
        /*
        for(int k=0;k<utf8_counter;k++)
        {     
            printf("utf8_buffer \\x%x \n",utf8_buffer[k]);
        }
        */
       
        

        //utf8_to_unicode_codepoints(utf8_buffer,utf8_counter,unicode_buffer);

        if(isValidUTF8Character(utf8_buffer,utf8_counter) == INVALID)
        {
            cout<<"Invalid UTF-8 sequence at byte "<<(BUFSIZE*bytePosBase)+counter+globalSlack<<"-->";
            utf8_to_unicode_codepoints(utf8_buffer,utf8_counter,unicode_buffer);
            //unicode_codepoints_to_utf8_bytes(unicode_buffer);

            for(int k=0;k<utf8_counter;k++)
            {     
                fprintf(fdc,"\\x%x",utf8_buffer[k]);
            }
            fprintf(fdc,"\n");
            fwrite(REPLACEMENT_CHARACTER,3,1,fdw);

        }
        else
        {
            fwrite(utf8_buffer,utf8_counter,1,fdw);
        } 
        delete [] unicode_buffer;
        delete [] utf8_buffer;
        unicode_buffer=NULL;
        utf8_buffer=NULL;

        counter++;//Increment for the byte read
		
        counter+=bytesToRead;//increment for the bytes to be read
    }
    if(unicode_buffer != NULL)
    {
		delete [] unicode_buffer;
		unicode_buffer=NULL;
    }
    if(utf8_buffer != NULL)
    {
        delete [] utf8_buffer;
        utf8_buffer=NULL;
    }
    bytePosBase+=1;//One addition BUFSIZE accounted for

    return 0;
}

#define ERR_IO_FILE_SAME -1
#define ERR_INPUT_FILE_NOT_EXIST -2

int main(int argc,char *argv[])
{
    if(argc < 4 )
    {
        fprintf(stdout,"Sanitizes the input-xml file by detecting and replacing invalid UTF-8 byte sequences as well as Invalid UTF-8 XML byte sequences\n\n"); 
        fprintf(stderr,"Usage: %s input-file-name sanatized-output-file-name conversion-log-results\n",argv[0]);
        exit(0);
    }

    if(!strcmp(argv[1],argv[2]))
    {
        fprintf(stderr,"Input and Output files cannot be the same\n");
        return ERR_IO_FILE_SAME;
    }
    
    FILE *fdr = fopen(argv[1],"rb");//Fixed (toread in biary mode)  because this was causing weird reads  -- RJM

    if(fdr == NULL)
    {
        fprintf(stderr,"Input file does not exist\n");
        return ERR_INPUT_FILE_NOT_EXIST;
    }



    FILE *fdw = fopen(argv[2],"w+");
    FILE *fdc = fopen(argv[3],"w+");

    string sanitizedString;
    int madeSane;
    int bytesRead=0;
	memset(buffer,0,BUFSIZE*sizeof(unsigned char));
    while(!feof(fdr))
    {
        madeSane=0;
        bytesRead=fread((void*)buffer,BUFSIZE,1,fdr);
		sanitize_utf8(buffer,sanitizedString,madeSane,fdr,fdw,fdc);	
        memset(buffer,0,BUFSIZE*sizeof(unsigned char));
    }

    fclose(fdr);
    fclose(fdw);
    fclose(fdc);

    return 0;
}
