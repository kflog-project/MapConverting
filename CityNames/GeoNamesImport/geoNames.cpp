/*
 * $Id$
 *
 * geonames.cpp
 *
 *  Created on: 06.10.2011
 *      Author: axel
 *
 *  This tools is used to read the population data from a population file, which
 *  is downloaded from this web site:
 *
 *  http://www.geonames.org/
 *
 *  All entries in the file are tabulator separated. The advantage of this file
 *  is, that beside the location names an additional attribute is available
 *  which describes the number of inhabitants. By using this attribute as
 *  filter criterion, the found city name is often the right one.
 *
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
       << " -i <input GeoNames-text-file> -o <output kflog-city-dictionary>"
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

  // in.setCodec( "ISO 8859-15" );
  in.setCodec( "UTF-8" );

  uint lNo = 0;
  uint oNo = 0;

  while (!in.atEnd())
    {
      QString line = in.readLine();
      lNo++;

      QStringList list = line.split("\t");

      if( list.size() < 19 )
        {
          cerr << "Line(" << lNo << "): contains too less parameters!" << endl;
          continue;
        }

      // entry must be:
      // PPL:  populated place
      // PPLC: capital of a political entity
      if( ! list[7].startsWith("PPL") )
        {
          continue;
        }

      bool ok;

      int population = list[14].toInt(&ok);

      if( ! ok )
        {
          continue;
        }

      if( population == 0 )
        {
          // No population number, ignore entry. That is the best filter method
          // for eliminating of spot points.
          continue;
        }

      double lat = list[4].toDouble(&ok);

      if( ! ok )
        {
          cerr << "Line(" << lNo << "): wrong latitude value " << list[4].toLatin1().data() << endl;
          continue;
        }

      double lon = list[5].toDouble(&ok);

      if( ! ok )
        {
          cerr << "Line(" << lNo << "): wrong longitude value " << list[4].toLatin1().data() << endl;
          continue;
        }

      // Full name
      QString name = list[1].trimmed().remove(QChar('"'));

      // Store results in the output file.
      out << lat << lon << name;
      oNo++;
    }

  cout << oNo << " entries are written to " << oFileName.toLatin1().data() << endl;

  iFile.close();
  oFile.close();
  return 0;
}
