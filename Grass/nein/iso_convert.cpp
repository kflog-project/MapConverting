/*
 *
 */

#include <iostream.h>
#include <stdlib.h>

/* Bis auf weiteres verwenden wir die Qt-Funktionen zum Zugriff auf Dateien und Datenströme */
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextstream.h>

#define PI 3.141592654
#define RADIUS 6370289.509

/* liefert den größeren der beiden übergebenen Werte zurück. */
#define MAX(a,b)   ( ( a > b ) ? a : b )

/* liefert den kleineren der beiden übergebenen Werte zurück. */
#define MIN(a,b)   ( ( a < b ) ? a : b )

/* enthält die geographischen Koordinaten eines Punktes, angegeben im internen KFLog-Format */
struct point
{
  int latitude;
  int longitude;
};

/* enthält ein eingelesenes Element */
struct element
{
  int value;
  bool isClosed;
  int isMountain;

  QList<struct point> pointList;
  int borderN;
  int borderS;
  int borderW;
  int borderE;
};

double dist(int lat1, int lon1, int lat2, int lon2)
{
  double pi_180 = PI / 108000000.0;
  double dlat = lat1 - lat2;
  double dlon = lon1 - lon2;

  double dist = RADIUS * sqrt(
      ((pi_180) * dlat * (pi_180) * dlat)
      + ((pi_180) * cos(pi_180 * lat1)* dlon
        * (pi_180) * cos(pi_180 * lat1)* dlon) );

  return dist / 1000.0;
}

double dist(struct point* p1, struct point* p2)
{
  return dist(p1->latitude, p1->longitude, p2->latitude, p2->longitude);
}

int main(int argc, char* argv[], char* env[])
{
  QString baseFileName = "NotSet";
  //  unsigned int minLength = 2;

  QString sourcePath = "/data/KartenDaten/KFLog-Karten/";
  QString targetPath = "/data/KartenDaten/KFLog-Karten/Schritt1/";

  bool isElement = false;
  struct element* currentLine;
  struct point* currentPoint;

  for(int loop = 1; loop < argc; loop++)
    {
      QList<struct element> eList;

      // Linien dürfen nicht an 0°N/0°W beginnen ...
      int preLat = 0, preLon = 0;
      unsigned int countBack = 0;

      QString inLine;
      QString temp = argv[loop];
      QString inFileName = sourcePath + temp;
      QString outFileName = targetPath + temp;

      QFile inFile(inFileName);
      QFile outFile(outFileName);

      QTextStream inStream(&inFile);
      QTextStream outStream(&outFile);

      warning("Bearbeite Datei %s ...", (const char*)temp);

      inFile.open(IO_ReadOnly);
      while(!inStream.eof())
	{
	  inLine = inStream.readLine();

	  if(inLine.mid(0,1) == "#" || inLine.mid(0,4) == "TYPE" || inLine.mid(0,4) == "NAME") continue;

	  if(inLine.mid(0,5) == "[NEW]")
	    {
	      isElement = true;

	      currentLine = new struct element;
	      currentLine->isMountain = -1;
	      currentLine->isClosed = false;
	    }
	  else if(inLine.mid(0,5) == "[END]")
	    {
	      if(!isElement)
		{
		  warning("PROBLEM!");
		  break;
		}
	      isElement = false;

	      warning("Teste Element: %d", eList.count());

	      eList.append(currentLine);
	    }
	  else if(inLine.mid(0,9) == "ELEVATION")
	    {
	      currentLine->value = inLine.mid(10,10).toInt();
	    }
	  else if(inLine.mid(0,8) == "MOUNTAIN")
	    {
	    }
	  else
	    {
	      if(inLine.mid(0,8).toInt() == preLat && inLine.mid(8,10).toInt() == preLon)
		continue;

	      // Klappt nur zwischen 16° und 90° Nord!!!
	      currentPoint = new struct point;
	      currentPoint->latitude = inLine.mid(0,8).toInt();
	      currentPoint->longitude = inLine.mid(8,10).toInt();
	      currentLine->pointList.append(currentPoint);
	      preLat = currentPoint->latitude;
	      preLon = currentPoint->longitude;
	    }
	}

      warning("Anzahl der Elemente: %d / countBack: %d", eList.count(), countBack);

      outFile.open(IO_WriteOnly);

      outStream << "#KFLog-Mapfile Version: 0.5 (c) 2002 The KFLog-Team\n";

      int preLatInt = (90 * 60 * 10000) + 100, preLonInt;

      for(unsigned int loop = 0; loop < eList.count(); loop++)
	{
	  outStream << "[NEW]\nTYPE=70\nNAME=" << loop << endl;
	  outStream << "ELEVATION=" << eList.at(loop)->value << endl;
	  outStream << "MOUNTAIN=" << eList.at(loop)->isMountain << endl;

	  for(unsigned int loop2 = 0; loop2 < eList.at(loop)->pointList.count(); loop2++)
	    {
	      if(eList.at(loop)->pointList.at(loop2)->latitude != preLatInt &&
		 eList.at(loop)->pointList.at(loop2)->longitude != preLonInt)
		outStream << eList.at(loop)->pointList.at(loop2)->latitude << " "
			  << eList.at(loop)->pointList.at(loop2)->longitude << endl;
	    } 
	  outStream << "[END]\n";
	}
    }

  // Alles getan :-))
  exit(0);
}
