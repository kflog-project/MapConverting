/*
 *
 *
 * Konvertiert Grass Dateien in KFlog Ascii Dateien
 *
 *
 *
 *
 * "-p" : Fortsetzungen werden geprüft
 *
 * "-n" : Benachbarte Elemente werden gesucht
 *
 * Bedeutung von "mode":
 *   0 : nicht gesetzt
 *   1 : Flüsse
 *   2 : Straßen
 *   3 : Eisenbahnen
 *   4 : Städte
 *   5 : Höhenlinien
 *   6 : Küstenlinien
 *   7 : Seen
 *
 *
 * Die Berg-Tal-Analyse dauert bei vielen großen Objekten sehr lange ...
 * Ein effizienterer Weg wäre sicherlich hilfreich!
 *
 * Die Prüfung, ob ein Objekt "offen" oder "geschlossen" ist, sollte erst nach dem
 * Zusammenfügen erfolgen ...
 *
 * Die Größe der Städte muss noch von der Punkt-Anzahl abhängig gemacht werden!
 *
 * Fortsetzungen noch nicht ganz richtig ...
 */

#include <iostream.h>
#include <stdlib.h>

/* Bis auf weiteres verwenden wir die Qt-Funktionen zum Zugriff auf Dateien und Datenströme */
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qpointarray.h>
#include <qregexp.h>
#include <qregion.h>
#include <qstring.h>
#include <qtextstream.h>

#define PI 3.141592654
#define RADIUS 6370289.509

/* Diese Werte müssen an den jeweiligen Bereich angepasst werden */
#define COL_MIN -12
#define COL_MAX 40
#define ROW_MIN 34
#define ROW_MAX 70

/* liefert den größeren der beiden übergebenen Werte zurück. */
#define MAX(a,b)   ( ( a > b ) ? a : b )

/* liefert den kleineren der beiden übergebenen Werte zurück. */
#define MIN(a,b)   ( ( a < b ) ? a : b )

#define CALCPOINT(a) ( ( a * 60 * 60 ) / 30 )

double minDist = 0.25;

/* beschreibt den Modus, in dem gearbeitet werden soll */
enum modeName {NotSet, River, Road, Railway, City, Isohypse, Coast, Lake};

/* enthält die geographischen Koordinaten eines Punktes, angegeben im internen KFLog-Format */
struct point
{
  int latitude;
  int longitude;
};

/* enthält eine Linie zwischen zwei Punkten */
struct line
{
  struct point p1;
  struct point p2;
};

/* enthält ein eingelesenes Element */
struct element
{
  int type;
  int value;
  int sortID;
  bool isClosed;
  bool isValley;
  QList<struct point> pointList;
  /* */
  int borderN;
  int borderS;
  int borderW;
  int borderE;
};

/* Dient dazu, Fehlerbereiche in der Ausgangsdatei zu definieren und auszuschliesen. */
struct border
{
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

unsigned int proofeMirrorLine(struct element* currentLine)
{
  bool abbr = true;
  int rueckcount = 0;
  int n;

  unsigned int length = currentLine->pointList.count();
  for(unsigned int loop = 2; loop < length; loop++)
    {
      struct point *currentPoint = currentLine->pointList.at(loop);
      if(currentPoint->latitude == currentLine->pointList.at(loop - 2)->latitude &&
	 currentPoint->longitude == currentLine->pointList.at(loop - 2)->longitude)
	{
	  rueckcount++;
	  // Sackgassen rauslöschen!
	  n = 1;
	  while(abbr)
	    {
	      if(loop - n < currentLine->pointList.count() ||
		 loop + n > currentLine->pointList.count())
		{
		  rueckcount--;
		  abbr = false;
		}

	      if(loop + n < currentLine->pointList.count())
		{
		  if(currentLine->pointList.at(loop + n)->latitude != 
		     currentLine->pointList.at(loop - n)->latitude ||
		     currentLine->pointList.at(loop + n)->longitude != 
		     currentLine->pointList.at(loop - n)->longitude)
		    {
		      rueckcount--;
		      for(int l = 0; l < n; l++)
			{
			  length--;
			  currentLine->pointList.remove(loop - n + 1);
			}
		      abbr = false;
		    }
		}
	      n++;
	    }
	}
    }

  if(rueckcount == -1) rueckcount++;

  return rueckcount;
}

/*
 * Prüft, ob eine Linie die Fortsetzung einer anderen Linie ist.
 */
unsigned int proofeContinues(QList<struct element>* eList, bool near)
{
  unsigned int maxPointCount = 0;
  unsigned int count = 0;
  unsigned int length = eList->count();

  for(unsigned int loop = 0; loop < length - 1; loop++)
    {
      // Wenn wir einen geschlossenen Zug haben, können wir die Prüfung erstmal
      // übergehen ...
      if(eList->at(loop)->isClosed) continue;

      struct point* pointA1 = eList->at(loop)->pointList.getFirst();
      struct point* pointA2 = eList->at(loop)->pointList.getLast();

      for(unsigned int loop2 = loop + 1; loop2 < length; loop2++)
	{
	  if(eList->at(loop2)->isClosed) continue;

	  //if(eList->at(loop)->value != eList->at(loop2)->value) continue;

	  struct point* pointB1 = eList->at(loop2)->pointList.getFirst();
	  struct point* pointB2 = eList->at(loop2)->pointList.getLast();

	  if(pointA1->latitude == pointB1->latitude &&
	     pointA1->longitude == pointB1->longitude)
	    {
	      for(unsigned int pLoop = 0; pLoop < eList->at(loop2)->pointList.count(); pLoop++)
		eList->at(loop)->pointList.insert(0, eList->at(loop2)->pointList.at(pLoop));

	      pointB1 = eList->at(loop)->pointList.getFirst();
	      eList->at(loop)->borderN = MAX(eList->at(loop)->borderN, eList->at(loop2)->borderN);
	      eList->at(loop)->borderS = MIN(eList->at(loop)->borderS, eList->at(loop2)->borderS);
	      eList->at(loop)->borderE = MAX(eList->at(loop)->borderE, eList->at(loop2)->borderE);
	      eList->at(loop)->borderW = MIN(eList->at(loop)->borderW, eList->at(loop2)->borderW);
	      eList->remove(loop2);
	      loop2--;
	      length--;
	      count++;
	    }
	  else if(pointA2->latitude == pointB1->latitude &&
		  pointA2->longitude == pointB1->longitude)
	    {
	      for(unsigned int pLoop = 0; pLoop < eList->at(loop2)->pointList.count(); pLoop++)
		eList->at(loop)->pointList.append(eList->at(loop2)->pointList.at(pLoop));

	      pointA2 = eList->at(loop)->pointList.getLast();
	      eList->at(loop)->borderN = MAX(eList->at(loop)->borderN, eList->at(loop2)->borderN);
	      eList->at(loop)->borderS = MIN(eList->at(loop)->borderS, eList->at(loop2)->borderS);
	      eList->at(loop)->borderE = MAX(eList->at(loop)->borderE, eList->at(loop2)->borderE);
	      eList->at(loop)->borderW = MIN(eList->at(loop)->borderW, eList->at(loop2)->borderW);
	      eList->remove(loop2);
	      loop2--;
	      length--;
	      count++;
	    }
	  else if(pointA1->latitude == pointB2->latitude &&
		  pointA1->longitude == pointB2->longitude)
	    {
	      for(int pLoop = eList->at(loop2)->pointList.count() - 1; pLoop >= 0; pLoop--)
		eList->at(loop)->pointList.insert(0, eList->at(loop2)->pointList.at(pLoop));

	      pointB1 = eList->at(loop)->pointList.getFirst();
	      eList->at(loop)->borderN = MAX(eList->at(loop)->borderN, eList->at(loop2)->borderN);
	      eList->at(loop)->borderS = MIN(eList->at(loop)->borderS, eList->at(loop2)->borderS);
	      eList->at(loop)->borderE = MAX(eList->at(loop)->borderE, eList->at(loop2)->borderE);
	      eList->at(loop)->borderW = MIN(eList->at(loop)->borderW, eList->at(loop2)->borderW);
	      eList->remove(loop2);
	      loop2--;
	      length--;
	      count++;
	    }
	  else if(pointA2->latitude == pointB2->latitude &&
		  pointA2->longitude == pointB2->longitude)
	    {
	      for(int pLoop = eList->at(loop2)->pointList.count() - 1; pLoop >= 0; pLoop--)
		eList->at(loop)->pointList.append(eList->at(loop2)->pointList.at(pLoop));

	      pointA2 = eList->at(loop)->pointList.getLast();
	      eList->at(loop)->borderN = MAX(eList->at(loop)->borderN, eList->at(loop2)->borderN);
	      eList->at(loop)->borderS = MIN(eList->at(loop)->borderS, eList->at(loop2)->borderS);
	      eList->at(loop)->borderE = MAX(eList->at(loop)->borderE, eList->at(loop2)->borderE);
	      eList->at(loop)->borderW = MIN(eList->at(loop)->borderW, eList->at(loop2)->borderW);
	      eList->remove(loop2);
	      loop2--;
	      length--;
	      count++;
	    }
	}
      maxPointCount = MAX(maxPointCount, eList->at(loop)->pointList.count());
    }
  warning("   %d Fortsetzungen gefunden", count);
  warning("Längstes Element: %d", maxPointCount);

  return count;
}

/*
 * Prüft, ob ein Element in einem anderen liegt. Dabei nehmen wir an, dass GRASS eine
 * Fehlermeldung ausgibt, wenn sich zwei Elemente schneiden. Somit reicht es hier aus,
 * nur einen Punkt zu prüfen.
 */
void proofeIntersections(QList<struct element>* eList)
{
  warning("Prüfe Überschneidungen ...");

  QList<QRegion> regList;

  for(unsigned int loop = 0; loop < eList->count() - 1; loop++)
    {
      struct element* elA = eList->at(loop);

      QPointArray pArray(elA->pointList.count());

      for(unsigned int pLoop = 0; pLoop < elA->pointList.count(); pLoop++)
	{
	  pArray.setPoint(pLoop, QPoint(elA->pointList.at(pLoop)->latitude / 2000,
					elA->pointList.at(pLoop)->longitude / 2000));
	}

      for(unsigned int regLoop = 0; regLoop < regList.count(); regLoop++)
	{
	  QRegion* testReg = regList.at(regLoop);

	  if(testReg->contains(pArray.point(0)))
	    {
	      // Element ist in früherer Region enthalten ...
	      elA->isValley = !eList->at(regLoop)->isValley;
	      elA->sortID++;
	    }
	}

      regList.append(new QRegion(pArray));
    }

  warning("   ... fertig");
}

int main(int argc, char* argv[], char* env[])
{
  /* Fehlerbereiche definieren:   */
  struct border errorA;
  struct border errorB;
  struct border errorC;

  /* 51°16'N -> 51°23'N / 11°46'E -> 11°56'E */
  errorA.borderS = 30760000;
  errorA.borderN = 30830000;
  errorA.borderW =  7060000;
  errorA.borderE =  7160000;

  /* 52°10'N -> 52°12'N / 11°00'E -> 11°03'E */
  errorB.borderS = 31300000;
  errorB.borderN = 31320000;
  errorB.borderW =  6600000;
  errorB.borderE =  6630000;

  /* 52°27'N -> 52°30'N / 13°46'E -> 13°50'E */
  errorC.borderS = 31470000;
  errorC.borderN = 31500000;
  errorC.borderW =  8260000;
  errorC.borderE =  8300000;

  QString baseFileName = "NotSet";
  unsigned int minLength = 2;
  bool proofeCont = false;
  bool setProofeNear = false;
  int mode = NotSet;

  int west = -100000000, east = -100000000, north = -100000000, south = -100000000;

  QString locationPath = getenv("LOCATION");
  QString locTargetPath = getenv("LOC_TARGET");

  if(locationPath == NotSet)
    {
      warning("\nVariable $LOCATION nicht definiert\n");
      exit(1);
    }

  if(locTargetPath == NotSet)
    {
      warning("\nVariable $LOC_TARGET nicht definiert\n");
      exit(1);
    }

  for(int loop = 1; loop < argc; loop++)
    {
      QString temp = argv[loop];

      if(temp.mid(0,5) == "input")
	// Eingabedatei angegeben!
	baseFileName = temp.mid(6,100);
      else if(temp.mid(0,3) == "min")
	// Mindestlänge angegeben!
	minLength = (temp.mid(4,10)).toUInt();
      else if(temp.mid(0,2) == "-p")
	// Es soll geprüft werden, ob eine Linie die Fortsetzung einer anderen ist.
	proofeCont = true;
      else if(temp.mid(0,4) == "mode")
	// Der Modus, in dem gearbeitet werden soll, muss angegeben werden.
	mode = (temp.mid(5,10)).toInt();
      else if(temp.mid(0,4) == "dist")
	minDist = (temp.mid(5,10)).toDouble();
      else if(temp.mid(0,2) == "-n")
	setProofeNear = true;
      else if(temp.mid(0,2) == "N=")
	north = (temp.mid(2,10)).toInt();
      else if(temp.mid(0,2) == "S=")
	south = (temp.mid(2,10)).toInt();
      else if(temp.mid(0,2) == "W=")
	west = (temp.mid(2,10)).toInt();
      else if(temp.mid(0,2) == "E=")
	east = (temp.mid(2,10)).toInt();
    }

  if(west == -100000000 || east == -100000000 || north == -100000000 || south == -100000000)
    {
      // keine Datei angegeben!
      warning("\nEs wurde keine Grenzen angegeben!\n");
      exit(1);
    }

  warning("Grenzen: %d <-> %d / %d <-> %d", west, east, south, north);

  north *= 60 * 10000;
  south *= 60 * 10000;
  west *= 60 * 10000;
  east *= 60 * 10000;

  // Bei flächenhaften Elementen brauchen wir mindestens 3 Punkte,
  // bei linienhaften mindestens 2.
  if(mode == City || mode == Isohypse || mode == Lake)
    minLength = MAX(minLength, 3);
  else
    minLength = MAX(minLength, 2);

  if(baseFileName == "NotSet")
    {
      // keine Datei angegeben!
      warning("\nEs wurde keine Eingabedatei angegeben!\n");
      exit(1);
    }

  if(mode == NotSet)
    {
      // kein Modus angegeben!
      warning("\nEs wurde kein Modus angegeben!\n");
      exit(1);
    }

  int elementType = NotSet;

  switch(mode)
    {
    case River:
      // alles auf "MidRiver" setzen ...
      elementType = 61;
      break;
    case Road:
      // alles auf "MidRoad" setzen ...
      elementType = 48;
      break;
    case Railway:
      elementType = 52;
      break;
    case City:
      elementType = 37;
      break;
    case Isohypse:
    case Coast:
      elementType = 70;
      break;
    case Lake:
      elementType = 58;
      break;
    default:
      ;
    }

  QString inFileName = locationPath + "/dig_ascii/" + baseFileName;
  QString attFileName = locationPath + "/dig_att/" + baseFileName;
  QFileInfo fInfo(baseFileName);
  QString outFileName = locTargetPath + baseFileName + ".out";

  fInfo.setFile(inFileName);
  if(!fInfo.exists() || !fInfo.isReadable())
    {
      warning("\nEingabe-Datei konnte nicht gefunden werden:\n    %s\n",
	      (const char*)inFileName);
      exit(1);
    }

  fInfo.setFile(attFileName);
  if(!fInfo.exists() || !fInfo.isReadable())
    {
      warning("\nAttribut-Datei konnte nicht gefunden werden:\n    %s\n",
	      (const char*)inFileName);
      exit(1);
    }

  warning("Datei: %s / Länge: %d", (const char*)baseFileName, minLength);

  QFile inFile(inFileName);
  QFile attFile(attFileName);
  QFile outFile(outFileName);

  QTextStream inStream(&inFile);
  QTextStream attStream(&attFile);
  QTextStream outStream(&outFile);

  QString inLine;
  QString attLine;

  attFile.open(IO_ReadOnly);
  //  int height = 0;
  double latVal, lonVal, preLat, preLon;

  int delCount = 0;

  bool isElement = false;

  QRegExp pos("^\\s[0-9]*");

  struct element* currentLine;
  struct point* currentPoint;
  unsigned int maxPointCount = 0;
  //  int openCount = 0;

  QList<struct element> elementList;

  ////////////////////////////////////////////////////////////////////////////////////////
  //
  // Alle Angaben wurden geprüft. Jetzt wird die Datei eingelesen.
  //
  ////////////////////////////////////////////////////////////////////////////////////////

  inFile.open(IO_ReadOnly);
  while(!inStream.eof())
    {
      inLine = inStream.readLine();
      if(pos.match(inLine) != -1)
	{
	  // Positionsangabe
	  if(isElement)
	    {
	      latVal = ((QString)inLine.mid(0,13)).toDouble();
	      lonVal = ((QString)inLine.mid(13,13)).toDouble();
	      if(currentLine->pointList.count() && (preLat == latVal && preLon == lonVal))
		// Der aktuelle Punkt ist gleich dem letzten ...
		continue;

	      preLat = latVal;
	      preLon = lonVal;

	      // Umrechnen der Punkte
	      currentPoint = new struct point;
	      currentPoint->latitude = (int)(latVal * 60 * 10000);
	      currentPoint->longitude = (int)(lonVal * 60 * 10000);
	      /*
	      if(currentPoint->latitude < south)
		currentPoint->latitude = south;
	      else if(currentPoint->latitude > north)
		currentPoint->latitude = north;

	      if(currentPoint->longitude < west)
		currentPoint->longitude = west;
	      else if(currentPoint->longitude > east)
		currentPoint->longitude = east;
	      */
	      currentLine->pointList.append(currentPoint);

	      if(currentLine->pointList.count() > 1)
		{
		  currentLine->borderN = MAX(currentLine->borderN, currentPoint->latitude);
		  currentLine->borderS = MIN(currentLine->borderS, currentPoint->latitude);
		  currentLine->borderE = MAX(currentLine->borderE, currentPoint->longitude);
		  currentLine->borderW = MIN(currentLine->borderW, currentPoint->longitude);
		}
	      else
		{
		  currentLine->borderN = currentPoint->latitude;
		  currentLine->borderS = currentPoint->latitude;
		  currentLine->borderE = currentPoint->longitude;
		  currentLine->borderW = currentPoint->longitude;
		}
	    }
	  else
	    warning("Positionsangabe vor Anfang eines Elementes");
	}
      else if(inLine.mid(0,2) == "L " || inLine.mid(0,2) == "A ")
	{
	  if(isElement)
	    {
	      maxPointCount = MAX(maxPointCount, currentLine->pointList.count());
	      if( ( currentLine->borderN <= errorA.borderN &&
		    currentLine->borderS >= errorA.borderS &&
		    currentLine->borderE <= errorA.borderE &&
		    currentLine->borderW >= errorA.borderW ) ||
		  ( currentLine->borderN <= errorB.borderN &&
		    currentLine->borderS >= errorB.borderS &&
		    currentLine->borderE <= errorB.borderE &&
		    currentLine->borderW >= errorB.borderW ) ||
		  ( currentLine->borderN <= errorC.borderN &&
		    currentLine->borderS >= errorC.borderS &&
		    currentLine->borderE <= errorC.borderE &&
		    currentLine->borderW >= errorC.borderW ) )
		// Wir haben eine der falschen Linien ...
		;
	      else
		if(currentLine->pointList.count() >= 2)
		  {
		    if(currentLine->borderN - currentLine->borderS >= -10 &&
		       currentLine->borderE - currentLine->borderW >= -10)
		      {
			// Nur bei Linien bis 250m muss die Ecke ergänzt werden ...
			/*
			if(currentLine->value <= 250)
			  {
			    if( ( currentLine->pointList.getFirst()->longitude > 2040000 &&
				  currentLine->pointList.getFirst()->longitude < 2160000 &&
				  currentLine->pointList.getFirst()->latitude < 2070000 ) ||
				( currentLine->pointList.getLast()->longitude > 2040000 &&
				  currentLine->pointList.getLast()->longitude < 2160000 &&
				  currentLine->pointList.getLast()->latitude < 2070000 ) )
			      {
				struct point* corner = new struct point;
				corner->latitude = 2040000;
				corner->longitude = 2520000;
				currentLine->pointList.append(corner);
			      }
			  }
			*/
			elementList.append(currentLine);
		      }
		    else
		      delCount++;
		  }
		else
		  delCount++;
	    }

	  // eine neue Linie beginnt!

	  isElement = true;

	  currentLine = new struct element;
	  currentLine->isValley = false;
	  currentLine->isClosed = false;
	  currentLine->type = elementType;
	  currentLine->sortID = 0;

	  if(mode == Isohypse)
	    {
	      // Einlesen der Höhe:
	      attLine = attStream.readLine();
	      currentLine->value = ((QString)attLine.mid(35,15)).toInt();
	    }
	}
    }

  // Das letzte Element muss noch gesichert werden ...
  elementList.append(currentLine);

  warning("Anzahl der Elemente: %d", elementList.count());
  //  warning("Anzahl der offenen Elemente: %d", openCount);
  warning("gelöschte Elemente: %d", delCount);
  warning("Längstes Element: %d", maxPointCount);

  ////////////////////////////////////////////////////////////////////////////////////////
  //
  // Alle Elemente wurden eingelesen. Jetzt werden Fortsetzungen gesucht.
  //
  ////////////////////////////////////////////////////////////////////////////////////////

  // Diese Fortsetzung macht nur dann Sinn, wenn Objekte gleichen Typs verglichen werden
  // und der Typ endgültig ist.
  if(proofeCont)
    {
      warning("Prüfe Fortsetzungen ...");

      // Diese Überprüfung muss solange wiederholt werden, bis sich die Zahl der Elemente
      // nicht mehr verändert. Gibt es einen effektiveren Weg???
      unsigned int count = 1;
      do
	{
	  warning("    Durchgang %d", count);
	  warning("Anzahl der Elemente: %d", elementList.count());
	  count++;
	}
      while(proofeContinues(&elementList, setProofeNear));

      warning("   ... fertig");
    }

  ////////////////////////////////////////////////////////////////////////////////////////
  //
  // Die Fortsetzungen wurden gesucht. Jetzt werden Überschneidungen gesucht.
  //
  ////////////////////////////////////////////////////////////////////////////////////////

  // Es wird angenommen, dass sich keine Elemente schneiden. Die Berg-Tal-Analyse
  // ist anscheinend noch fehlerhaft, deshalb wird es "von Hand" gemacht ...
  if(mode == City || mode == Isohypse || mode == Coast || mode == Lake)
    proofeIntersections(&elementList);

  ////////////////////////////////////////////////////////////////////////////////////////
  //
  // Jetzt werden die Elemente ausgegeben
  //
  ////////////////////////////////////////////////////////////////////////////////////////

  outFile.open(IO_WriteOnly);

  outStream << "#KFLog-Mapfile Version: 0.5 (c) 2000 The KFLog-Team\n";

  int preLatInt = (90 * 60 * 10000) + 100, preLonInt;

  for(unsigned int loop = 0; loop < elementList.count(); loop++)
    {
      // zu kurze Elemente werden nicht geschrieben ...
      if(elementList.at(loop)->pointList.count() < minLength) continue;

      outStream << "[NEW]\nTYPE=" << elementList.at(loop)->type << "\nNAME=" << loop
		<< "," << elementList.at(loop)->pointList.count()
		<< "\nSORT=" << elementList.at(loop)->sortID << endl;
      if(mode == Isohypse)
	{
	  outStream << "ELEVATION=" << elementList.at(loop)->value << endl;
	  outStream << "MOUNTAIN=" << elementList.at(loop)->isValley << endl;
	}
      else if(mode == Coast)
	{
	  outStream << "ELEVATION=0\n";
	  outStream << "MOUNTAIN=" << elementList.at(loop)->isValley << endl;
	}

      for(unsigned int loop2 = 0; loop2 < elementList.at(loop)->pointList.count(); loop2++)
	{
	  if(elementList.at(loop)->pointList.at(loop2)->latitude != preLatInt &&
	     elementList.at(loop)->pointList.at(loop2)->longitude != preLonInt)
	    outStream << elementList.at(loop)->pointList.at(loop2)->latitude << " "
		      << elementList.at(loop)->pointList.at(loop2)->longitude << endl;
	  preLat = elementList.at(loop)->pointList.at(loop2)->latitude;
	  preLon = elementList.at(loop)->pointList.at(loop2)->longitude;
	} 
      outStream << "[END]\n";
    }

  // Alles getan :-))
  exit(0);
}
