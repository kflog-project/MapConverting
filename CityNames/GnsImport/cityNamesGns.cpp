/*
 * $Id$
 *
 * cityNamesGns.cpp
 *
 *  Created on: 06.10.2011
 *      Author: axel
 *
 *  This tools is used to read the population data from a CSV file, which is
 *  created by using the HTML forms of this web site:
 *
 *  http://geonames.nga.mil/ggmagaz/
 *
 *  I used the following options:
 *
 *  -Name Type Search: Approved Names
 *  -Select Name Display: UTF-8
 *  -Advanced  Search: All scales
 *  -Feature Designations: Populated Places, PPL, PPLC
 *  -Output File Format: CSV
 *  -Output Fields: Default selections
 *
 *  The queries can be restricted by using country option or a certain map area.
 *  The disadvantage of the output is, that too much single points are contained
 *  as cities. That caused often an assignment of a city part to the city
 *  polygon.
 */

using namespace std;

#include <iostream>
#include <QtCore>

// ===========================================================================
// Usage of program
// ===========================================================================

void usage( char * progName )
{
  cerr << "KFlog City name dictionary creator V1.0, Axel Pauli (axel@kflog.org)" << endl
       << "Usage: " << basename(progName)
       << " -i <input GNS-text-file> -o <output kflog-city-dictionary>"
       << endl << endl;

  exit(-1);
}

int main(int argc, char *argv[])
{
  if( argc < 3 )
    {
      cerr << "Missing mandatory arguments" << endl << endl;
      usage(argv[0]);
    }

  QString iFileName;
  QString oFileName;

  int c;

  while( (c = getopt(argc, argv, "i:o:")) != -1 )
    {
      switch (c)
        {
        case 'i':
          iFileName = optarg;
          break;
        case 'o':
          oFileName = optarg;
          break;
        case '?':
            {
              cerr << "Unrecognized option: -" << optopt << "!" << endl;
              return -1;
            }
        }
    }

  QFile iFile(iFileName);

  if (! iFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
      cerr << "Cannot open input file: " << iFileName.toLatin1().data() << endl;
      return -1;
    }

  QFile oFile(oFileName);

  if (! oFile.open(QIODevice::WriteOnly ) )
    {
      iFile.close();
      cerr << "Cannot open output file: " << oFileName.toLatin1().data() << endl;
      return -1;
    }

  QTextStream in(&iFile);
  QDataStream out(&oFile);

  uint lNo = 0;
  uint oNo = 0;

  while (!in.atEnd())
    {
      QString line = in.readLine();
      lNo++;

      // First line has to handled as comment line.
      if( line.startsWith("RC") )
        {
          continue;
        }

      QStringList list = line.split(",");

      if( list.size() < 25 )
        {
          cerr << "Line(" << lNo << "): contains too less parameters!" << endl;
          continue;
        }

      // entry must be:
      // PPL:  populated place
      // PPLC: capital of a political entity
//      if( list[10] != "PPL" && list[10] != "PPLC" )
//        {
//          cerr << "Line(" << lNo << "): DSG " << list[10].toLatin1().data() << " is ignored!" << endl;
//          continue;
//        }

      bool ok;

      double lat = list[3].toDouble(&ok);

      if( ! ok )
        {
          cerr << "Line(" << lNo << "): wrong latitude value " << list[3].toLatin1().data() << endl;
          continue;
        }

      double lon = list[4].toDouble(&ok);

      if( ! ok )
        {
          cerr << "Line(" << lNo << "): wrong longitude value " << list[4].toLatin1().data() << endl;
          continue;
        }

      // Full name
      QString name = list[22].trimmed().remove(QChar('"'));

      // Store results in the output file.
      out << lat << lon << name;
      oNo++;
    }

  cout << oNo << " entries are written to " << oFileName.toLatin1().data() << endl;

  iFile.close();
  oFile.close();
  return 0;
}
