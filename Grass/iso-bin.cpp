/*
 * Erzeugt binäre Höhenlinien-Dateien für KFLog
 *
 */

#include <iostream.h>
#include <stdlib.h>

#include <qdatetime.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qdatastream.h>

#include <ressource.h>

void readFile(QString inFileName, Q_UINT16 SecNumber, int lat, int lon)
{
  Q_INT8 typID;
  Q_UINT16 formatID = FORMAT_VERSION;

  QString outTerrainFileName, outGroundFileName, inLine;
  int elevation, sort, valley, *latList, *lonList, minElev = 9000, minValley;
  unsigned int length;

  outTerrainFileName.sprintf("/opt/kde2/share/apps/kflog/mapdata/T_%.5d.kfl", SecNumber);
  outGroundFileName.sprintf("/opt/kde2/share/apps/kflog/mapdata/G_%.5d.kfl", SecNumber);
  warning("Datei: %s -> %s", (const char*)inFileName, (const char*) outGroundFileName);
  warning("          -> %s", (const char*) outTerrainFileName);

  QFile inGroundFile("/data/KartenDaten/KFLog-Karten/grenzen_gekachelt/" +
	       inFileName + ".out");
  QFile inTerrainFile("/data/KartenDaten/KFLog-Karten/höhenlinien_gekachelt/" +
	       inFileName + ".out");
  /*
  warning("/data/KartenDaten/KFLog-Karten/grenzen_gekachelt/" +
	  inFileName + ".out");
  warning("/data/KartenDaten/KFLog-Karten/höhenlinien_gekachelt/" +
	       inFileName + ".out");

  return;
  */
  QFile outGroundFile(outGroundFileName);
  QFile outTerrainFile(outTerrainFileName);

  QTextStream inGroundStream(&inGroundFile);
  QTextStream inTerrainStream(&inTerrainFile);
  QDataStream outGroundStream(&outGroundFile);
  QDataStream outTerrainStream(&outTerrainFile);

  outTerrainFile.open(IO_WriteOnly);
  outTerrainStream.setVersion(2);
  /*
  outGroundFile.open(IO_WriteOnly);
  outGroundStream.setVersion(2);

  if(inGroundFile.open(IO_ReadOnly) && inGroundFile.size() > 0)
    {
      ///////////////////////////////////////////////////////////////////////////////////
      //
      // Ground-Data:
      //
      ///////////////////////////////////////////////////////////////////////////////////

      typID = TYPE_GROUND;

      // Schreiben des Dateiheaders:
      outGroundStream << KFLOG_FILE_MAGIC
		      << typID
		      << formatID
		      << SecNumber
		      << QDateTime::currentDateTime();

      latList = new int[1];
      lonList = new int[1];

      while(!inGroundStream.eof())
	{
	  inLine = inGroundStream.readLine();
	  if(inLine.mid(0,1) == "#")
	    {
	    }
	  else if(inLine.mid(0,5) == "[NEW]")
	    {
	      length = 0;
	    }
	  else if(inLine.mid(0,4) == "TYPE")
	    {
	    }
	  else if(inLine.mid(0,9) == "ELEVATION")
	    {
	      elevation = ((QString)inLine.mid(10,4)).toInt();
	    }
	  else if(inLine.mid(0,4) == "SORT")
	    {
	      sort = ((QString)inLine.mid(5,3)).toInt();
	    }
	  else if(inLine.mid(0,8) == "MOUNTAIN")
	    {
	      // In den Ascii-Dateien hat der Wert genau die gegenteilige Bedeutung ...
	      valley = !((QString)inLine.mid(9,1)).toInt();
	    }
	  else if(inLine.mid(0,4) == "NAME")
	    {
	      // Abfragen des Namens fehlte !
	    }
	  else if(inLine.mid(0,5) == "[END]")
	    {
	      // in Binärdatei schreiben
	      outGroundStream << (Q_UINT8) 70   << (Q_INT16) elevation
			      << (Q_INT8) valley   << (Q_INT8) sort << (Q_UINT32) length;

	      for(unsigned int i = 0; i < length; i++)
		  outGroundStream << (Q_INT32) latList[i] << (Q_INT32) lonList[i];
	    }
	  else
	    {
	      // Koordinaten
	      latList = (int*)realloc(latList, ((length + 1) * sizeof(int)));
	      lonList = (int*)realloc(lonList, ((length + 1) * sizeof(int)));

	      latList[length] = ((QString)inLine.mid(0,8)).toInt();
	      lonList[length] = ((QString)inLine.mid(9,10)).toInt();

	      length++;
	    }
	}
    }
*/
  if(inTerrainFile.open(IO_ReadOnly) && inTerrainFile.size() > 0)
    {
      /**********************************************************************************
       *
       * Terrain-Data:
       *
       **********************************************************************************/

      typID = TYPE_TERRAIN;
      // Schreiben des Dateiheaders:
      outTerrainStream << KFLOG_FILE_MAGIC
		       << typID
		       << formatID
		       << SecNumber
		       << QDateTime::currentDateTime();

      latList = new int[1];
      lonList = new int[1];

      while(!inTerrainStream.eof())
	{
	  inLine = inTerrainStream.readLine();
	  if(inLine.mid(0,1) == "#")
	    {
	    }
	  else if(inLine.mid(0,5) == "[NEW]")
	    {
	      length = 0;
	    }
	  else if(inLine.mid(0,4) == "TYPE")
	    {
	    }
	  else if(inLine.mid(0,9) == "ELEVATION")
	    {
	      elevation = ((QString)inLine.mid(10,4)).toInt();
	    }
	  else if(inLine.mid(0,4) == "SORT")
	    {
	      sort = ((QString)inLine.mid(5,3)).toInt();
	    }
	  else if(inLine.mid(0,8) == "MOUNTAIN")
	    {
	      // In den Ascii-Dateien hat der Wert genau die gegenteilige Bedeutung ...
	      valley = !((QString)inLine.mid(9,1)).toInt();
	    }
	  else if(inLine.mid(0,4) == "NAME")
	    {
	      // Abfragen des Namens fehlte !
	    }
	  else if(inLine.mid(0,5) == "[END]")
	    {
	      /*
	       * Wenn die unterste Höhenlinie ein Tal ist, muss auch die tiefer liegende
	       * zusätzliche Höhenlinie ein Tal werden!
	       */
	      if(minElev > elevation)
		{
		  minElev = elevation;
		  minValley = valley;
		}
	      else if( ( minElev == elevation ) && ( valley == 1 ) )
		{
		  minValley = 1;
		}

	      // in Binärdatei schreiben
	      if(sort >= 0 && sort <= 3)
		{
		  outTerrainStream << (Q_UINT8) 70   << (Q_INT16) elevation
				   << (Q_INT8) valley   << (Q_INT8) sort << (Q_UINT32) length;

		  for(unsigned int i = 0; i < length; i++)
		      outTerrainStream << (Q_INT32) latList[i] << (Q_INT32) lonList[i];
		}
	    }
	  else
	    {
	      // Koordinaten
	      latList = (int*)realloc(latList, ((length + 1) * sizeof(int)));
	      lonList = (int*)realloc(lonList, ((length + 1) * sizeof(int)));

	      latList[length] = ((QString)inLine.mid(0, inLine.find(' '))).toInt();
	      lonList[length] = ((QString)inLine.mid(inLine.find(' '), 10)).toInt();

	      length++;
	    }
	}

      if(minElev > 1000)
	minElev -= 250;
      else if(minElev > 100)
	minElev -= 100;
      else if(minElev > 25)
	minElev -= 25;
      else
	minElev -= 15;

      //      minValley = 1;
      //      minElev = 200;

      minElev = -1;
      if(minElev > 0)
	{
	  warning("Ergänze unterste Ebene: %d", minElev);
	  /*
	   * Wieso stand hier vorher "lon * 60..." und "( lon + 2) * 60..." ???
	   */
	  outTerrainStream << (Q_UINT8) 70 << (Q_INT16) minElev
			   << (Q_INT8) minValley   << (Q_INT8) 0 << (Q_UINT32) 4
			   << (Q_INT32) ( lat * 600000 - 10000 )  << (Q_INT32) ( ( lon - 2 ) * 600000 - 10000 )
			   << (Q_INT32) ( lat * 600000 - 10000 )  << (Q_INT32) ( ( lon ) * 600000  + 10000 )
			   << (Q_INT32) ( ( lat + 2 ) * 600000 + 10000 ) << (Q_INT32) ( ( lon ) * 600000 + 10000 )
			   << (Q_INT32) ( ( lat + 2 ) * 600000 + 10000 ) << (Q_INT32) ( ( lon - 2 ) * 600000 - 10000 );
	}
    }
}


int main(int argc, char* argv[], char* env[])
{
  QList<QString> fileList;

  int lat = -1, lon = -1;
  bool isNorth = true, isEast = true;

  for(int loop = 1; loop < argc; loop++)
    {
      /*
       * Zunächst muss der Dateiname analysiert werden, da dieser die Kachelgrenzen
       * enthält. Damit wird die Kachel-ID bestimmt.
       */
      QString rootFileName = QFileInfo(argv[loop]).baseName();

      int pos = rootFileName.find("_");

      isNorth = rootFileName.find("N") > -1;
      isEast = rootFileName.find("E") > -1;

      if(pos == -1)
	{
	  // Falscher Dateiname!!!
	  warning("Falscher Dateiname!!!");
	  continue;
	}

      lat = rootFileName.mid(0, pos - 1).toInt();
      lon = rootFileName.mid(pos + 1, rootFileName.length() - pos - 2).toInt();

      if(lat < 0 || lat > 90)
	{
	  // FEHLER !!!
	  fatal("nicht erlaubter Dateiname!");
	}

      if(lon < 0 || lon > 180)
	{
	  // FEHLER !!!
	  fatal("nicht erlaubter Dateiname!");
	}

      int latBorder = ( ( lat / 2 ) * 2 - 88 ) / -2;
      int lonBorder = ( ( lon / 2 ) * 2 + 180 ) / 2;

      if(!isNorth)
	{
	  latBorder = 88 - latBorder;
	  lat *= -1;
	}
      if(!isEast)
	{
	  lonBorder = 180 - lonBorder;
	  lon *= -1;
	}

      /* Kachel-ID: */
      short int ID = latBorder + ( lonBorder + ( latBorder * 179 ) );

      readFile(rootFileName, ID, lat, lon);
    }
}
