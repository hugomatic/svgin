#include <stdlib.h>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include "tinyxml.h"

// ----------------------------------------------------------------------
// STDOUT dump and indenting utility functions
// ----------------------------------------------------------------------
const unsigned int NUM_INDENTS_PER_SPACE=2;

const char * getIndent( unsigned int numIndents )
{
    static const char * pINDENT="                                      + ";
    static const unsigned int LENGTH=strlen( pINDENT );
    unsigned int n=numIndents*NUM_INDENTS_PER_SPACE;
    if ( n > LENGTH ) n = LENGTH;

    return &pINDENT[ LENGTH-n ];
}

// same as getIndent but no "+" at the end
const char * getIndentAlt( unsigned int numIndents )
{
    static const char * pINDENT="                                        ";
    static const unsigned int LENGTH=strlen( pINDENT );
    unsigned int n=numIndents*NUM_INDENTS_PER_SPACE;
    if ( n > LENGTH ) n = LENGTH;

    return &pINDENT[ LENGTH-n ];
}

int dump_attribs_to_stdout(TiXmlElement* pElement, unsigned int indent)
{
    if ( !pElement ) return 0;

    TiXmlAttribute* pAttrib=pElement->FirstAttribute();
    int i=0;
    int ival;
    double dval;
    const char* pIndent=getIndent(indent);
    printf("\n");
    while (pAttrib)
    {
        printf( "%s%s: value=[%s]", pIndent, pAttrib->Name(), pAttrib->Value());

        if (pAttrib->QueryIntValue(&ival)==TIXML_SUCCESS)    printf( " int=%d", ival);
        if (pAttrib->QueryDoubleValue(&dval)==TIXML_SUCCESS) printf( " d=%1.1f", dval);
        printf( "\n" );
        i++;
        pAttrib=pAttrib->Next();
    }
    return i;   
}

void dump_to_stdout( TiXmlNode* pParent, unsigned int indent = 0 )
{
    if ( !pParent ) return;

    TiXmlNode* pChild;
    TiXmlText* pText;
    int t = pParent->Type();
    printf( "%s", getIndent(indent));
    int num;

    switch ( t )
    {
    case TiXmlNode::TINYXML_DOCUMENT:
        printf( "Document" );
        break;

    case TiXmlNode::TINYXML_ELEMENT:
        printf( "Element [%s]", pParent->Value() );
        num=dump_attribs_to_stdout(pParent->ToElement(), indent+1);
        switch(num)
        {
            case 0:  printf( " (No attributes)"); break;
            case 1:  printf( "%s1 attribute", getIndentAlt(indent)); break;
            default: printf( "%s%d attributes", getIndentAlt(indent), num); break;
        }
        break;

    case TiXmlNode::TINYXML_COMMENT:
        printf( "Comment: [%s]", pParent->Value());
        break;

    case TiXmlNode::TINYXML_UNKNOWN:
        printf( "Unknown" );
        break;

    case TiXmlNode::TINYXML_TEXT:
        pText = pParent->ToText();
        printf( "Text: [%s]", pText->Value() );
        break;

    case TiXmlNode::TINYXML_DECLARATION:
        printf( "Declaration" );
        break;
    default:
        break;
    }
    printf( "\n" );
    for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
    {
        dump_to_stdout( pChild, indent+1 );
    }
}


std::string lowercase(const std::string& in)
{
  std::string out = in;
  std::transform(out.begin(), out.end(), out.begin(), ::tolower);
  return out;
}

std::string lowercase(const char* in)
{
  std::string ins = in;
  return lowercase(ins);
}


struct Command
{
   char type;
   double data[6]; // not all used
};

struct Path
{
   std::vector<Command> cmds;
   std::string id;
   std::string style;
};



std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}



void get_path_commands(std::vector<std::string> &tokens, std::vector<Command> &cmds)
{
     std::string lookup = "cCmMlLvVhHzZ";
     unsigned int size = tokens.size();

/*
     int i =0; 
     for(std::string token: tokens)
     {
        std::cout << i << "/ " << size <<  "] " << token << std::endl;
        i++;     
     }
*/
     char lastCmd = 'x';
     std::vector<double> numbers;
     
     for(std::string token: tokens)
     {
       // new command?
       if(lookup.find(token[0]) == std::string::npos)
       { 
         // its just numbers
         std::cout << std::endl;
         std::vector<std::string> numberStrs;
         split(token, ',', numberStrs);
         for(std::string numberStr : numberStrs)
         {
           std::cout << "    " << lastCmd  << " "  << numberStr << std::endl;
           double f = atof(numberStr.c_str());
           numbers.push_back(f);
         }
       }
       else
       {
         if(lastCmd != 'x')
         {
           Command cmd;
           cmd.type = lastCmd;
           if (lastCmd == 'c')
           {
              unsigned int i =0; 
           }
//           else if ()
           cmds.push_back(cmd);
         }
         // its new command
         lastCmd = token[0];
         numbers.resize(0);
         std::cout << "  CMD:" << lastCmd << std::endl;
       }
     }
}

void get_path_attribs(TiXmlElement* pElement, Path &path)
{
    if ( !pElement ) return;

    TiXmlAttribute* pAttrib=pElement->FirstAttribute();
    while (pAttrib)
    {
        std::string name = lowercase(pAttrib->Name());
        std::string value= lowercase(pAttrib->Value());
        if (name == "style")
        {
            path.style = value;
        }
        if (name == "id")
        {
            path.id = value;
        }
        if (name == "d")
        {
            using namespace std;
            // this attribute contains a list of coordinates
            std::vector<std::string> tokens;
            split(value, ' ', tokens);
            get_path_commands(tokens, path.cmds);
            
        }
        // int ival;
        // double dval;
        // if (pAttrib->QueryIntValue(&ival)==TIXML_SUCCESS)    printf( " int=%d", ival);
        // if (pAttrib->QueryDoubleValue(&dval)==TIXML_SUCCESS) printf( " d=%1.1f", dval);
        pAttrib=pAttrib->Next();
    }
}


void get_svg_paths(TiXmlNode* pParent, std::vector<Path> &paths)
{
    if ( !pParent ) return;

    TiXmlNode* pChild;
    TiXmlText* pText;
    int t = pParent->Type();
    int num;
    std::string name;
    switch ( t )
    {
      case TiXmlNode::TINYXML_ELEMENT:
        name = lowercase(pParent->Value());
        if (name == "path")
        {
          Path p;
          get_path_attribs(pParent->ToElement(), p);
          paths.push_back(p);
        }
        break; 

     default:
        break;
   }

  for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
  {
      get_svg_paths( pChild, paths );
  }
}

// load the named file and dump its structure to STDOUT
void dump_to_stdout(const char* pFilename)
{
    TiXmlDocument doc(pFilename);
    bool loadOkay = doc.LoadFile();
    if (!loadOkay)
    {
        printf("Failed to load file \"%s\"\n", pFilename);
    }
    else
    {
      printf("\n%s:\n", pFilename);
//      dump_to_stdout( &doc ); // defined later in the tutorial
      std::vector<Path> paths;

      std::cout << "\n\n\n****\n\n\n*****\n\n\n****" << std::endl;

      get_svg_paths( &doc, paths);

      std::cout << "PATHS: " << std::endl;
      for (Path path : paths)
      {
        std::cout << " -" << path.id << std::endl;
     }
  }
}

// ----------------------------------------------------------------------
// main() for printing files named on the command line
// ----------------------------------------------------------------------
int main(int argc, char* argv[])
{
    for (int i=1; i<argc; i++)
    {
        dump_to_stdout(argv[i]);
    }
    return 0;
}

