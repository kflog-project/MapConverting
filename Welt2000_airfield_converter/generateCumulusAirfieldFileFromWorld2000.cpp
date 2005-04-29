/************************************************************************
**
**   Copyright (c): 2005 by Axel Pauli, axel@kflog.org
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

using namespace std;

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <iostream>
#include <math.h>

#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdatetime.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qdict.h>

// Defines from cumulus

#define KFLOG_FILE_MAGIC      0x404b464c
#define FILE_TYPE_AERO        0x41
#define FILE_VERSION_AIRFIELD 101

#define NOT_SELECTED      0
#define INT_AIRPORT       1
#define AIRPORT           2
#define MIL_AIRPORT       3
#define CIVMIL_AIRPORT    4
#define AIRFIELD          5
#define CLOSED_AIRFIELD   6
#define CIV_HELIPORT      7
#define MIL_HELIPORT      8
#define AMB_HELIPORT      9
#define GLIDERSITE       10
#define ULTRALIGHT       11
#define HANGGLIDER       12
#define PARACHUTE        13
#define BALLON           14
#define OUTLANDING       15

// ===========================================================================
// Usage of programm
// ===========================================================================

void usage( char * progName )
{
  cerr << "KFlog Airfield File Generator V1.0, Axel Pauli (axel@kflog.org)" << endl
       << "Using WELT2000.TXT file as input from "
       << "http://www.segelflug.de/segelflieger/michael.meier." << endl
       << "Output file contains all Airfields in KFLOG format." << endl
       << "Option -f can be used as country filter. Countries are coded " << endl
       << "according to ISO 3166 standard." << endl
       << "If option -u for UltraLight fields was set they are included too." << endl
       << "Output can be used too by Cumulus as airfield source file." << endl << endl
       << "Usage: " << basename(progName)
       << " -i <input-welt-2000-file> -o <output-kflog-file> [-f country | -u]"
       << endl << endl;

  exit(-1);
}

int main(int argc, char *argv[])
{
  if( argc < 5 )
    {
      cerr << "Missing mandatory arguments" << endl << endl;
      usage(argv[0]);
    }

  const char *iFileName = 0;
  const char *oFileName = 0;
  QString filter = "";

  int c;
  extern char *optarg;
  extern int optind, optopt;
  bool ulOpt = false;

  while( (c = getopt(argc, argv, "f:i:o:u")) != -1 )
    {
      switch (c)
	{
	case 'u':
	  ulOpt = true;
	  break;
	case 'i':
	  iFileName = optarg;
	  break;
	case 'o':
	  oFileName = optarg;
	  break;
	case 'f':
	  filter = optarg;
	  filter = filter.upper();
	  break;
	case '?':
	  if( optopt == 'f' )
	    {
	      cerr << "Option -" << optopt << " requires an argument!" << endl;
	      break;
	    }
	  else
	    {
	      cerr << "Unrecognized option: -" << optopt << "!" << endl;
	      return -1;
	    }
	}
    }

  QFile inFile(iFileName);
  QFile outFile(oFileName);

  if( !inFile.open(IO_ReadOnly) )
    {
      cerr << "Cannot open input file: " << argv[1] << endl;
      exit(-1);
    }

  if( !outFile.open(IO_WriteOnly) )
    {
      cerr << "Cannot open output file: " << argv[2] << endl;
      exit(-1);
    }

  QDataStream out(&outFile);
  out.setVersion(2);

  // ICAO signs of international airports of germany

  QDict<char> intAirPortsDe(101);

  intAirPortsDe.insert( "EDDB", "EDDB" );
  intAirPortsDe.insert( "EDDT", "EDDT" );
  intAirPortsDe.insert( "EDDI", "EDDI" );
  intAirPortsDe.insert( "EDDW", "EDDW" );
  intAirPortsDe.insert( "EDDC", "EDDC" );
  intAirPortsDe.insert( "EDDL", "EDDL" );
  intAirPortsDe.insert( "EDDE", "EDDE" );
  intAirPortsDe.insert( "EDDF", "EDDF" );
  intAirPortsDe.insert( "EDDH", "EDDH" );
  intAirPortsDe.insert( "EDDV", "EDDV" );
  intAirPortsDe.insert( "EDDK", "EDDK" );
  intAirPortsDe.insert( "EDDP", "EDDP" );
  intAirPortsDe.insert( "EDDM", "EDDM" );
  intAirPortsDe.insert( "EDDG", "EDDG" );
  intAirPortsDe.insert( "EDDN", "EDDN" );
  intAirPortsDe.insert( "EDDR", "EDDR" );
  intAirPortsDe.insert( "EDDS", "EDDS" );

  Q_UINT32 magic = KFLOG_FILE_MAGIC;
  out << magic;

  Q_INT8 loadTypeID = FILE_TYPE_AERO;
  out << loadTypeID;

  Q_UINT16 formatID = FILE_VERSION_AIRFIELD;
  out << formatID;

  out << QDateTime::currentDateTime();

  uint lineNo = 0;
  uint counter = 0;
  QString lastName = "";
  uint lastCounter = 0;

  // statistik counter
  uint ul, gl, af; ul = gl = af = 0;

  // Input file was generated from Michael Meiers world 2000 data dase
  //
  // 0         1         2         3         4         5         6    
  // 0123456789012345678901234567890123456789012345678901234567890123
  // 1234567890123456789012345678901234567890123456789012345678901230
  // AACHE1 AACHEN  MERZBRUC#EDKAA 53082612287 189N504923E0061111DEO5
  // AICHA1 AICHACH         # S !G 43022012230 440N482824E0110807DEX
  // ARGEN2 ARGENBUEHL EISE?*ULM G 40082612342 686N474128E0095744DEN
  // BASAL2 BAD SALZUNGEN UL*ULM G 65092712342 233N504900E0101305DEN
  // FUERS1 FUERSTENWALDE   #EDALG 80112912650  55N522323E0140539DEO3
  // BERLT1 BERLIN  TEGEL   #EDDTA303082611870  37N523335E0131716DEO
  // BERSC1 BERLIN SCHOENFEL#EDDBC300072512002  49N522243E0133114DEO
  // BERTE1 BERLIN TEMPELHOF#EDDIC208092711810  52N522825E0132406DEO

  while( ! inFile.atEnd() )
    {
      bool ok;
      QString line, buf;
      int result = inFile.readLine(line, 128);
      lineNo++;

      if( result <= 0 )
	{
	  continue;
	}

      // step over comment or invalid lines
      if( line.startsWith("#") || line.startsWith("//") ||
	  line.startsWith("$") || line.startsWith(" " ) ||
	  line.startsWith("\t") )
	{
	  continue;
	}

      // remove white spaces and line end characters
      line = line.stripWhiteSpace();

      if( line.length() < 62 )
	{
	  // country sign not included
	  continue;
	}

      // convert all to upper case
      line = line.upper();

      // Extract country sign. It is coded according to ISO 3166.
      QString country = line.mid( 60, 2 );

      // look, if country filter option is set
      if( ! filter.isEmpty() && filter != country )
	{
	  continue;
	}

      // look, what kind of line was read.
      // COL5 = 1 Airfield
      // COL5 = 2 Outlanding

      QString kind = line.mid( 5, 1 );

      if( kind != "1" && kind != "2" )
	{
	  continue; // not of interest for us
	}

      bool ulField = false;
      bool glField = false;
      bool afField = false;
      QString icao;

      if( kind == "2" ) // can be an UL field
	{
	  if( line.mid( 23, 4 ) == "*ULM" && ulOpt == true )
	    {
	      ulField = true;
	    }
	  else
	    {
	      // step over other outlandings
	      continue;
	    }
	}
      else if( line.mid( 23, 3 ) == "# S" )
	{
	  // Glider field
	  glField = true;
	}
      else
	{
	  afField = true;
	  icao = line.mid( 24, 4 ).stripWhiteSpace();

	  if( line.mid( 20, 4 ) == "GLD#" )
            {
              // other possibility for a glider field with ICAO signature
              glField = true;
            }       
	}

      // Airfield name
      QString afName = line.mid( 7, 16 );

      // remove special mark signs
      afName.replace( QRegExp("[!?]+"), "" );

      // remove resp. replace white spaces against one space
      afName = afName.simplifyWhiteSpace();

      if( afName.length() == 0 )
	{
	  cerr << "Line " << lineNo
	       << ": Airfield name is undefined, ignoring entry!" << endl;
	  continue;
	}

      // airfield type
      Q_UINT8 afType = NOT_SELECTED;

      // determine airfield type so good as possible
      if( ulField == true )
	{
	  afType = ULTRALIGHT;
	  //afType = GLIDERSITE; // ULTRALIGHT;   workaround atm
	}
      else if( glField == true )
	{
	  afType = GLIDERSITE;
	}
      else if( afField == true )
	{
	  if( intAirPortsDe[icao] )
	    {
	      afType = INT_AIRPORT;
	    }
	  else if( icao.mid( 0, 2 ) == "ET" )
	    {
	      // German military airfield
	      afType = MIL_AIRPORT;
	    }
	  else if( afName.contains(QRegExp(" MIL$")) )
	    {
	      // should be an military airfiled but not 100% sure
	      afType = MIL_AIRPORT;
	    }
	  else
	    {
	      afType = AIRFIELD;
	    }
	}

      // airfield name
      afName = afName.lower();
      
      QChar lastChar(' ');

      // convert airfield names to upper-lower
      for( uint i=0; i < afName.length(); i++ )
	{
	  if( lastChar == ' ' )
	    {
	      afName.replace( i, 1, afName.mid(i,1).upper() );
	    }

	  lastChar = afName[i];
	}

      // gps name, only 8 characters without spaces
      QString gpsName = afName.left(8);

      if( lastName == gpsName )
	{
	  gpsName.replace( gpsName.length()-1, 1, QString::number(++lastCounter) );
	}
      else
	{
	  lastName = gpsName;
	  lastCounter = 0;
	}

      if( ulField  )
	{
	  if( afName.right(3) == " Ul" )
	    {
	      // Convert lower l of Ul to upper case
	      afName.replace( afName.length()-1, 1, "L" );
	    }
	  else
	    {
	      // append UL substring
	      // afName += " UL";
	    }
	}

      Q_INT32 lat, lon;
      QString degree, min, sec;
      double d, m, s;

      // convert latitude
      degree = line.mid(46,2);
      min    = line.mid(48,2);
      sec    = line.mid(50,2);

      d = degree.toDouble(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo
	       << ": wrong latitude degree value, ignoring entry!" << endl;
	  continue;
	}

      m = min.toDouble(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo
	       << ": wrong latitude minute value, ignoring entry!" << endl;
	  continue;
	}

      s = sec.toDouble(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo
	       << ": wrong latitude second value, ignoring entry!" << endl;
	  continue;
	}

      double latTmp = (d * 600000.) + (10000. * (m + s / 60. ));

      lat = (Q_INT32) rint(latTmp);

      if( line.mid(45,1) == "S" )
	{
	  lat = -lat;
	}

      // convert longitude
      degree = line.mid(53,3);
      min    = line.mid(56,2);
      sec    = line.mid(58,2);

      d = degree.toDouble(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo
	       << ": wrong longitude degree value, ignoring entry!" << endl;
	  continue;
	}

      m = min.toDouble(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo
	       << ": wrong longitude minute value, ignoring entry!" << endl;
	  continue;
	}

      s = sec.toDouble(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo
	       << ": wrong longitude second value, ignoring entry!" << endl;
	  continue;
	}

      double lonTmp = (d * 600000.) + (10000. * (m + s / 60. ));

      lon = (Q_INT32) rint(lonTmp);

      if( line.mid(52,1) == "W" )
	{
	  lon = -lon;
	}

      // elevation
      buf = line.mid(42,3 ).stripWhiteSpace();
      Q_INT16 elevation = buf.toInt(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo 
	       << ": missing or wrong elevation"
	       << ", set value to zero!" << endl;
	  elevation = 0;
	}

      // frequency
      QString frequency = line.mid(36,3).stripWhiteSpace() + "." +
	                  line.mid(39,2).stripWhiteSpace();

      double f = frequency.toDouble(&ok);

      if( !ok || f < 117.97 || f > 137.0 )
	{
	  cerr << "Line " << lineNo
	       << ": missing or wrong frequency"
	       << ", set value to zero!" << endl;
	  frequency = "000.000";
	}

      // check, what has to be appended as last digit
      if( line.mid(40,1) == "2" || line.mid(40,1) == "7" )
	{
	  frequency += "5";
	}
      else
	{
	  frequency += "0";
	}

      // runway direction as two digits, we consider only the first entry
      buf = line.mid(32,2).stripWhiteSpace();
      Q_UINT8 rwDir = buf.toUInt(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo 
	       << ": missing or wrong runway direction"
	       << ", set value to zero!" << endl;
	  rwDir = 0;
	}
      
      // runway length in meters, must be multiplied by 10
      buf = line.mid(29,3).stripWhiteSpace();
      Q_UINT16 rwLen = buf.toUInt(&ok);

      if( ! ok )
	{
	  cerr << "Line " << lineNo 
	       << ": missing or wrong runway length"
	       << ", set value to zero!" << endl;
	  rwLen = 0;
	}
      else
	{
	  rwLen *= 10;
	}

      enum SurfaceType {Unknown = 0, Grass = 1, Asphalt = 2, Concrete = 3};

      // runway surface
      Q_UINT8 rwSurface;
      buf = line.mid(28,1);

      if( buf == "A" )
	{
	  rwSurface = Asphalt;
	}
      else if( buf == "C" )
	{
	  rwSurface = Concrete;
	}
      else if( buf == "G" )
	{
	  rwSurface = Grass;
	}
      else
	{
	  rwSurface = Unknown;
	}

      //---------------------------------------------------------------
      // write out new record into file
      //---------------------------------------------------------------

      // count output records separated by kind
      if( ulField )
	ul++;
      else if( glField )
	gl++;
      else
	af++;

      // airfield type
      out << afType;
      // airfield name with country
      out << afName + " (" + country + ")";
      // airfield id
      out << QString::number(++counter);
      // icao
      out << icao.stripWhiteSpace();
      // GPS name
      out << gpsName.upper();
      // latitude
      out << lat;
      // longitude
      out << lon;
      // elevation in meters
      out << elevation;
      // winch available is always set to no
      if( afType == GLIDERSITE ) out << Q_INT8(0);
      // number of contact data, always set to one
      out << Q_INT8(1);
      // frequency written as 126.500
      out << frequency;
      // contact type
      out << Q_INT8(0);
      // call sign
      out << QString("");
      // number of runways always set to one
      out << Q_INT8(1);
      // runway direction
      out << rwDir;
      // runway length in meters
      out << rwLen;
      // runway surface
      out << rwSurface;
      // runway open always true assumed
      out << Q_INT8(1);

    } // End of while( ! in.atEnd() )

  inFile.close();
  outFile.close();

  // We should check, if something was given out, otherwise we should
  // remove the output file, because it is empty and senseless.

  if( ! counter )
    {
      // remove empty output file
      unlink(oFileName);
      cout << "Neither Airfields or UL-Fields found in file!" << endl;
      return 0;
    }

  cout << counter << " records written into output file";

  if( filter.length() == 2 )
    {
      cout << " for Country " << filter.upper().latin1();
    }

  cout << ", "
       << "Statistics:"
       << " Airfields=" << af
       << ", Glider=" << gl
       << ", UL=" << ul
       << endl;

  return 0;
}
