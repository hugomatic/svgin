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
  
   std::string tostr() 
   {
     std::ostringstream os;
     os << type << "[";
     for (double d : numbers)
     {
       os << d << ", ";
     }
     os << "]";
     return os.str();
   }   
};

struct Path
{
   std::string id;
   std::string style;

   std::vector< std::vector<Command> > subpaths;   

   std::vector< std::vector<Point> > polylines;
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

  private: void ExpandCommands(const std::vector< std::vector<Command> > &subpaths, Path &path);
  private: void SplitSubpaths(const std::vector<Command> cmds, std::vector< std::vector<Command> > &split_cmds);
  private: void PathToPoints(const Path &path, double resolution, std::vector< std::vector<Point> > &polys);

  private: Point SubpathToPolyline(const std::vector<Command> &subpath, Point last, std::vector<Point> &polyline);

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

Point bezierInterpolate(double t, const Point &p0, const Point &p1, const Point &p2, const Point &p3)
{
  double t_1 = 1.0 - t;
  double t_1_2 = t_1 * t_1;
  double t_1_3 = t_1_2 * t_1;
  double t2 = t * t;
  double t3 = t2 * t;
  
  Point p;  
  p.x = t_1_3 * p0.x + 3 * t *  t_1_2 * p1.x + 3 * t2 * t_1 * p2.x + t3 * p3.x;
  p.y = t_1_3 * p0.y + 3 * t *  t_1_2 * p1.y + 3 * t2 * t_1 * p2.y + t3 * p3.y;

  return p;  
}

double Distance(const Point &p0, const Point &p1)
{
  double xx = (p0.x - p1.x) *  (p0.x - p1.x);
  double yy = (p0.y - p1.y) *  (p0.y - p1.y);
  return sqrt(xx + yy);
}

unsigned int GetStepCount(const Point &p0, const Point &p1, const Point &p2, const Point &p3, double res)
{
  double d = Distance(p0,p1) + Distance(p1,p2) + Distance(p2, p3);
  double steps = res / d;
  return (unsigned int) fabs(steps);
}


Point SvgReader::SubpathToPolyline(const std::vector<Command> &subpath, Point last,  std::vector<Point> &polyline)
{
  for (Command cmd: subpath)
  {
    if (cmd.type == 'm' || cmd.type == 'l')
    {
       size_t i =0;
       size_t count = cmd.numbers.size();
       while(i < count)
       {
         Point p;
         p.x = cmd.numbers[i+0];
         p.y = cmd.numbers[i+1];
         // m and l cmds are relative to the last point
         p.x += last.x;
         p.y += last.y;
         polyline.push_back(p);
         last = p; 
         i += 2;
      }
    }
  }
  return last;
}


void SvgReader::SplitSubpaths(const std::vector<Command> cmds, std::vector< std::vector<Command> > &subpaths)
{
  if(cmds.size() ==0)
  {
    std::ostringstream os;
    os << "Path has no commands";
    SvgError x(os.str());
    throw x;
  }
  
  for(Command cmd: cmds)
  {
    if( tolower(cmd.type) == 'm')
    {
      // the path contains a subpath
      std::vector<Command> sub;
      subpaths.push_back(sub);
    }
    // get a reference to the latest subpath
    std::vector<Command> &subpath = subpaths.back();
    // give the cmd to the latest
    subpath.push_back(cmd);
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


}

void SvgReader::ExpandCommands(const std::vector< std::vector<Command> > &subpaths,  Path &path)
{
  for (std::vector<Command> compressedSubpath :subpaths)
  {
    // add new subpath
    path.subpaths.push_back( std::vector<Command>());
    // get a reference	
    std::vector<Command> &subpath = path.subpaths.back();
    // copy the cmds with repeating commands, grouping the numbers
    for (Command xCmd : compressedSubpath)
    {
      unsigned int numberCount = 0;      
      if (tolower(xCmd.type) == 'c') 
	numberCount = 6;
      if (tolower(xCmd.type) == 'm')
	numberCount = 2;
      if (tolower(xCmd.type) == 'l')
	numberCount = 2;
      if (tolower(xCmd.type) == 'v')
	numberCount = 1;
      if (tolower(xCmd.type) == 'h')
	numberCount = 1;
      if (tolower(xCmd.type) == 'z')
      {
        subpath.push_back(xCmd);
      }	
      
      // group numbers together and repeat the command
      // for each group
      unsigned int n = 0;
      size_t size = xCmd.numbers.size();

      while(n < size)
      {
        subpath.push_back(Command());
        Command &cmd = subpath.back();
        cmd.type = xCmd.type;
	
     	for(size_t i=0; i < numberCount; i++)
	    {
	      cmd.numbers.push_back(xCmd.numbers[i+n]);
	    }
        n += numberCount;
      } 
    }
  }
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
    std::vector< std::vector< Command> > subpaths;
    this->SplitSubpaths(cmds, subpaths);

    this->ExpandCommands(subpaths, path );

    // the starting point for the subpath
    // it is the end point of the previous one
    Point p;
    p.x = 0;
    p.y = 0;
    for (std::vector<Command> subpath : subpaths)
    {
        path.polylines.push_back(std::vector<Point>());
        std::vector<Point> &polyline = path.polylines.back();
        
    	std::cout << "XXX " << polyline.size() << std::endl;
        p = this->SubpathToPolyline(subpath, p, polyline);
        std::cout << "X X X " << polyline.size() << std::endl;
    }
        
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
  std::cout << "var svg = [];" << std::endl;
  for (Path path : paths)
  {
    std::cout << "svg.push({name:\"" << path.id <<  "\", subpaths:[], style: \"" << path.style << "\"}); " << std::endl;
    // std::cout << " -" << path.id << " " << path.style << std::endl;
//    for (std::vector<Command> subpath : path.subpaths)
//    {
      // std::cout << "//  subpath (" << subpath.size() << " cmds)" << std::endl;
      // for (Command cmd: subpath)
      // {
        // std::cout << "//    " << cmd.tostr() << std::endl;
     // }
//    }
    std::cout << "svg[svg.length-1].subpaths = [";
    char psep = ' ';
    for (unsigned int i=0; i < path.polylines.size(); i++)
    {
      std::vector<Point> poly = path.polylines[i];
      std::cout << psep <<  "[" << std::endl;
      psep = ',';
      char sep = ' ';
      for( Point p : poly)
      {
        std::cout << " " << sep << " [" <<  p.x << ", " << p.y << "]" <<std::endl;
	    sep = ',';
      }
      std::cout << " ] " << std::endl;
    }
    std::cout << "];" << std::endl;
    std::cout << "\n\n";
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

