/*
 * Erzeugt binäre Karten-Dateien für KFLog
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

#include <resource.h>


#define DEBUG 0

int eins = 0, zwei = 0;

void readFile(QString inFileName, Q_UINT16 SecNumber)
{

  // inFileName ab sofort gesamter Name
  Q_INT8 typID = TYPE_MAP;
//  Q_UINT16 formatID = FORMAT_VERSION;
  Q_UINT16 formatID = 101;

  unsigned int lm_typ;
  QString outFileName, inLine, name;
  int sort = 0;
  int old_id = -1, element_type;
  int id, typ, typeID, *latList, *lonList, elev;
  unsigned int length = 0;

  outFileName.sprintf("/data/KartenDaten/KFLog-Karten/bin_maps/%c_%.5d.kfl", typID, SecNumber);
  
  if(DEBUG) warning("%c_%.5d.kfl", typID, SecNumber);

//  QFile inFile("/data/KartenDaten/KFLog-Karten/Gesamt/" + inFileName + ".out");
  QFile inFile(inFileName);

  QFile outFile(outFileName);

  QTextStream inStream(&inFile);

  QDataStream outStream(&outFile);

  outFile.open(IO_WriteOnly);
  outStream.setVersion(2);

  // Schreiben des Dateiheaders:
  outStream << KFLOG_FILE_MAGIC
            << typID
            << formatID
            << SecNumber
            << QDateTime::currentDateTime();

  latList = new int[1];
  lonList = new int[1];

  int smallItem = 0, smallRoad = 0, smallHighway = 0, smallCanal = 0, smallRiver = 0, smallRail = 0, smallCity = 0, smallLake = 0;
  int totalItem = 0, totalRoad = 0, totalHighway = 0, totalCanal = 0, totalRiver = 0, totalRail = 0, totalCity = 0, totalLake = 0;
  bool isItem = false;

  /**********************************************************************************
   *
   * ACHTUNG:
   *
   *   In einigen Dateien mit Strassen und Eisenbahnen (auch Flüsse?) treten
   *   Fehler auf. Es gibt Elemente, die nur aus einem Punkt bestehen und nicht
   *   mit einem "[END]" enden.
   *
   *   Wenn ein "[NEW]" eingelesen wird, bevor ein "[END]" kommt, wird die Anzahl
   *   der Punkte ausgegeben und das alte Element ignoriert!
   *
   **********************************************************************************/


///////////////////////////////////////////



  if(inFile.open(IO_ReadOnly))
    {

      //warning("Lese Datei ein!!!");
      isItem = false;
      //
      while(!inStream.eof())
        {
          inLine = inStream.readLine();

          if(inLine.mid(0,1) == "#")
            {
              // Kommentar ignorieren
            }
          else if(inLine.mid(0,5) == "[NEW]")
            {
//            totalItem++;

              if(isItem)
                {
                  fatal("Problem");
//                if(length < 5) smallItem++;
                }
              length = 0;
              isItem = true;
	      element_type = -1;
            }
          else if(inLine.mid(0,2) == "ID")
            {
              id = inLine.mid(3,10).toInt();
            }
          else if(inLine.mid(0,4) == "NAME")
            {
              name = inLine.mid(5,40);
            }
          else if(inLine.mid(0,4) == "TYPE")
            {
              // Wir haben einen Typ
              typ = inLine.mid(5,10).toInt();

              switch (typ)
                {
                  case CITY:
                  case LAKE:
                  case LAKE_T:
                  case GLACIER:
		    //                  case PACK_ICE:
                  case FOREST:
                    element_type = 3;
                    break;
                  case BORDER:
                  case AERIAL_CABLE:
                  case RAILWAY:
                  case RAILWAY_D:
                  case HIGHWAY:
                  case ROAD:
                  case TRAIL:
                  case CANAL:
                  case RIVER:
                  case RIVER_T:
                    element_type = 2;
                    break;
                  case VILLAGE:
                  case SPOT:
                  case LANDMARK:
		    element_type = 1;
                    break;
                  default:
		    element_type = -1;
                    warning("Unbekanntes Element: %d", typ);
                }
            }
          else if(inLine.mid(0,4) == "SORT")
            {
              sort = inLine.mid(5,10).toInt();
            }
          else if(inLine.mid(0,4) == "ELEV")
            {
              elev = inLine.mid(5,10).toInt();
            }
/*          else if(inLine.mid(0,4) == "DESC")
            {
              desc = inLine.mid(5,10);
            } */
          else if(inLine.mid(0,5) == "LM_TYP")
            {
              lm_typ = inLine.mid(6,10).toUInt();
            }
          else if(( inLine.mid(0,5) == "[END]" ) && ( typ != PACK_ICE ))
            {
              // in Binärdatei schreiben
              totalItem++;

	      //	      cerr << length << endl;

	      if( (length >= element_type) && (element_type > 0) )
		{

		  bool draw(true);

		  /////////////////////////////////////////////////////////////////////
		  ///
		  ///       ÜBERARBEITEN!!!!! -> GEMACHT
		  ///
		  outStream << (Q_UINT8) typ;
		  switch (typ)
		    {
		    case CITY:
		      totalCity++;
		    case LAKE:
		    case LAKE_T:
		    case GLACIER:
		      //                  case PACK_ICE:
		    case FOREST:
		      outStream << (Q_INT8) sort << (QString) name;
		      break;
		    case BORDER:
                    outStream << (Q_INT8) sort;
                    break;
		    case AERIAL_CABLE:
		    case RAILWAY:
		    case RAILWAY_D:
		    case HIGHWAY:
		    case ROAD:
		    case TRAIL:
		      // No further elements!
		      break;
		    case CANAL:
		    case RIVER:
		    case RIVER_T:
		      outStream << (QString) name;
		      break;
		    case VILLAGE:
		      outStream << (QString) name;
		      break;
		    case SPOT:
		      outStream << (Q_UINT8) elev;
		      break;
		    case LANDMARK:
		      // Richtige Größe für Typ??
		      /**
		       *  0 Dam / Weir
		       *  1 Lock
		       *  2 Mine / Quarry
		       *  3 Industrie
		       *  4 Depot / Tank
		       *  5 Tower
		       *  6 Camp / Settlement / Cairn
		       *  7 Ruin / Castle
		       *  8 Lighthouse
		       *  9 Yard / Station
		       * 10 Bridge
		       * 11 Ferry
		       *
		       **/
		      
		      outStream << (Q_UINT8)  lm_typ << (QString) name;
		      break;
		    default:
		      warning("Unbekanntes Element: %d", typ);
		    }
		  

		  //cout << (Q_UINT32) length << endl;
		  outStream << (Q_UINT32) length;

		  if( element_type == 1)
		    {
		      outStream << (Q_INT32) latList[0] << (Q_INT32) lonList[0];
		    }
		  else
		    {
		      for(unsigned int i = 0; i < length; i++)
			outStream << (Q_INT32) latList[i] << (Q_INT32) lonList[i];
		    }

		  if(length < 5) smallItem++;

		}
	      else
		{
		  if( length == 1 )
		    {
		      eins++;
		      cerr << typ << endl;
		    }
		  else
		    {
		      zwei++;
		    }
		}

	      isItem = false;
		  
            }
          else if( ( (inLine.mid(0,1) >= "0")  && (inLine.mid(0,1) <= "9") ) ||
		               (inLine.mid(0,1) == "-") )
            {
              // Koordinaten
              latList = (int*)realloc(latList, ((length + 1) * sizeof(int)));
              lonList = (int*)realloc(lonList, ((length + 1) * sizeof(int)));

              latList[length] = ((QString)inLine.mid(0, inLine.find(" ") ) ).toInt();
              lonList[length] = ((QString)inLine.mid(inLine.find(" ") + 1, 10 ) ).toInt();

              length++;
            }
        }
    }


    if(DEBUG)
    {
      warning("Elemente:");
      warning("Straßen:    %d", totalRoad);
  warning("Autobahnen: %d", totalHighway);
  warning("Flüsse:     %d", totalRiver);
  warning("Kanäle:     %d", totalCanal);
  warning("Seen:       %d", totalLake);
  warning("Städte:     %d", totalCity);
  warning("\nKurze Elemente: %d von %d",smallItem,totalItem);



  warning("Kurze Elemente:");
  warning("   %d von %d Strassen", smallRoad, totalRoad);
  warning("   %d von %d Eisenbahnen", smallRail, totalRail);
  warning("   %d von %d Flüssen", smallRiver, totalRiver);
  warning("   %d von %d Seen", smallLake, totalLake);
  warning("   %d von %d Städten", smallCity, totalCity);

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
	  warning("rootfilename %s",(const char*) rootFileName);
          fatal("\nFalscher Dateiname!!!");
          continue;
        }

      lat = rootFileName.mid(0, pos - 1).toInt();
      lon = rootFileName.mid(pos + 1, rootFileName.length() - pos - 2).toInt();

      if(lat < 0 || lat > 90)
        {
          // FEHLER !!!
	  warning("rootfilename %s",(const char*) rootFileName);
          fatal("\nnicht erlaubter Dateiname!");
        }

      if(lon < 0 || lon > 180)
        {
          // FEHLER !!!
	  warning("rootfilename lon %s",(const char*) rootFileName);
          fatal("\nnicht erlaubter Dateiname!");
        }

      int latBorder = ( ( lat / 2 ) * 2 - 88 ) / -2;
      int lonBorder = ( ( lon / 2 ) * 2 + 180 ) / 2;

      if(!isNorth) latBorder = 88 - latBorder;
      if(!isEast) lonBorder = 180 - lonBorder;

      /* Kachel-ID: */
      short int ID = latBorder + ( lonBorder + ( latBorder * 179 ) );


      // Ausgabe auf Konsole
      cout << "\r";
      cout << "Kachel Datei: ";
      cout.width(10);
      cout << loop << " von ";
      cout.width(10);
      cout << argc << " ( ";
      cout.precision(1);
      cout.width(4);
      cout.setf(ios::fixed);
      cout << (float)((float)loop / (float)argc * 100) << "%)";

      // Datei lesen
            readFile(argv[loop], ID);
    }
    
  cout << endl;


warning("einser Elemente: %d", eins);
warning("zweier Elemente: %d", zwei);

}
