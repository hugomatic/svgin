#include <stdlib.h>
#include <exception>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include "tinyxml.h"

class SvgError: public std::runtime_error
{ public: SvgError(const std::string& what_arg): std::runtime_error(what_arg){};};



struct Point
{
    double x;
    double y;
};

struct Command
{
   char type;
   std::vector<double> numbers;
/*
   double data[6]; // not all used
   Command()
   {
     data[0] = 0;
     data[1] = 0;
     data[2] = 0;
     data[3] = 0;
     data[4] = 0;
     data[5] = 0;
   }
*/
   
   std::string tostr() 
   {
     std::ostringstream os;
     os << type << "[";
     for (double d : numbers)
     {
       os << d << ", ";
     }
     os << "]";
/*
     os << data[0] << ", ";
     os << data[1] << ", ";
     os << data[2] << ", ";
     os << data[3] << ", ";
     os << data[4] << ", ";
     os << data[5] << "]";
*/
     return os.str();
   }   
};

struct Path
{
//   std::vector<Command> cmds;
   std::string id;
   std::string style;

    std::vector< std::vector<Command> > subPaths;   
};



class SvgReader
{

  public: SvgReader(){}

  public: void Parse(const char*path, std::vector<Path> &paths);
  public: void Dump_paths(const std::vector<Path> paths ) const;

  private: void make_commands(char cmd, const std::vector<double> &numbers, std::vector<Command> &cmds);
  private: void get_path_commands(const std::vector<std::string> &tokens, Path &path);
  private: void get_path_attribs(TiXmlElement* pElement, Path &path);
  private: void get_svg_paths(TiXmlNode* pParent, std::vector<Path> &paths);

  private: void SplitCommands(const std::vector<Command> cmds, double resolution, std::vector< std::vector<Command> > &split_cmds);
  private: void PathToPoints(const Path &path, double resolution, std::vector< std::vector<Point> > &polys);

};


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


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

void SvgReader::SplitCommands(const std::vector<Command> cmds, double resolution, std::vector< std::vector<Command> > &split_cmds)
{
  for(Command cmd: cmds)
  {
    if( tolower(cmd.type) == 'm')
    {
        
    }
  }
  
}

void SvgReader::make_commands(char cmd, const std::vector<double> &numbers, std::vector<Command> &cmds)
{
  if(cmd != 'x')
  { 
    Command c;
    c.type = cmd;
    c.numbers = numbers;
    cmds.push_back(c);
  }

/*
  if (tolower(cmd) == 'c')
  {
    unsigned int i = 0;
	size_t size = numbers.size();
	while( i < size)
	{
      Command c;
      c.type = cmd;
      
	  c.data[0] = numbers[i+0];
	  c.data[1] = numbers[i+1];
	  c.data[2] = numbers[i+2];
	  c.data[3] = numbers[i+3];
	  c.data[4] = numbers[i+4];
	  c.data[5] = numbers[i+5];
      cmds.push_back(c);
      i += 6;
	}
    return; 
  }

  if (tolower(cmd) == 'm' || tolower(cmd) == 'l')
  {
    unsigned int i = 0;
    size_t size = numbers.size();
    std::cout << "MCOMMAND " << size << std::endl;
    while (i < size)
    {
      Command c;
      c.type = cmd;
      size_t size =  numbers.size();
      c.data[0] = numbers[i+0];
      c.data[1] = numbers[i+1];
      cmds.push_back(c);
      i += 2;
    }
    return;
  }

  if (tolower(cmd) == 'v' || tolower(cmd) == 'h')
  {
    unsigned int i = 0;
    size_t size = numbers.size();
    while (i < size)
    {
      Command c;
      c.type = cmd;
      c.data[0] = numbers[0];
      c.data[1] = numbers[1];
      cmds.push_back(c);
      i += 1;
    }
    return;
  }

  if (tolower(cmd) == 'z')
  {
    Command c;
    c.type = cmd;
    cmds.push_back(c);
    return;
  }

  std::string s = "Unknown cmd " + cmd;
  SvgError x(s.c_str());
  throw(x);
 */

}


void SvgReader::get_path_commands(const std::vector<std::string> &tokens, Path &path)
{
     std::vector <Command> cmds;
     
     std::string lookup = "cCmMlLvVhHzZ";
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
           double f = atof(numberStr.c_str());
           numbers.push_back(f);
         }
       }
       else
       {
         
         if(lastCmd != 'x')
         { 
           Command c;
           c.type = lastCmd;
           c.numbers = numbers;
           cmds.push_back(c);
         }

         // its new command
         lastCmd = token[0];
         numbers.resize(0);
       }
     }
     // the last command
     if(lastCmd != 'x')
     { 
       Command c;
       c.type = lastCmd;
       c.numbers = numbers;
       cmds.push_back(c);
     }
    // split the commands into sub_paths
    
}

void SvgReader::get_path_attribs(TiXmlElement* pElement, Path &path)
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
            get_path_commands(tokens, path);
            
        }
        // int ival;
        // double dval;
        // if (pAttrib->QueryIntValue(&ival)==TIXML_SUCCESS)    printf( " int=%d", ival);
        // if (pAttrib->QueryDoubleValue(&dval)==TIXML_SUCCESS) printf( " d=%1.1f", dval);
        pAttrib=pAttrib->Next();
    }
}


void SvgReader::get_svg_paths(TiXmlNode* pParent, std::vector<Path> &paths)
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
void SvgReader::Parse(const char* pFilename, std::vector<Path> &paths)
{
    TiXmlDocument doc(pFilename);
    bool loadOkay = doc.LoadFile();
    if (!loadOkay)
    {
      std::ostringstream os;
      os << "Failed to load file " <<  pFilename;
      SvgError x(os.str());
      throw x;
    }

  get_svg_paths( &doc, paths);

}

void SvgReader::Dump_paths(const std::vector<Path> paths ) const
{
  std::cout << "PATHS: " << std::endl;
  for (Path path : paths)
  {
    std::cout << " -" << path.id << " " << path.style << std::endl;
    for (std::vector<Command> subPath : path.subPaths)
    {
      std::cout << "   subpath (" << subPath.size() << " cmds)" << std::endl;
      for (Command cmd: subPath)
  	  {
	    std::cout << "    " << cmd.tostr() << std::endl;
	  }
   }
 }

}

// ----------------------------------------------------------------------
// main() for printing files named on the command line
// ----------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::vector<Path> paths;

    for (int i=1; i<argc; i++)
    {
      std::cout << "=========\nFILE: " << argv[i] << std::endl;
      std::vector<Path> paths;

      SvgReader svg;
      svg.Parse(argv[i], paths);
      svg.Dump_paths(paths);     
    }

    return 0;
}

