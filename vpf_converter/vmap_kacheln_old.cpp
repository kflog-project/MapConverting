//
// Erzeugt gekachelte Daten aus den konvertierten VMAP Daten
//
//

#include <iostream.h>
#include <stdlib.h>

#include <qdatetime.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qpoint.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qdatastream.h>

#include <ressource.h>


int bound_s, bound_n, bound_w, bound_e;

QList<QPoint> positionList;

struct header
{
  QString name;

  unsigned int type;

  //....
};


//einlesen

QFile inFile(inFileName);

QTextStream inStream(&inFile);



if(inFile.open(IO_ReadOnly))
{
  isItem = false;
  while(!inStream.eof())
    {
      bound_s = 90 * 600000;
      bound_n = -90 * 600000;
      bound_w = 180 * 600000;
      bound_e = -180 * 600000;


      inLine = inStream.readLine();
      
      if(inLine.mid(0,1) == "#")
	{
	  // Kommentar ignorieren
	}
      else if(inLine.mid(0,4) == "nam")
	{
	  name = inLine.mid(5,1000);
	}
      else if(inLine.mid(0,5) == "[NEW]")
	{
	  count++;
	  while(contactList.remove());
	  while(positionList.remove());
	}
      else if(inLine.mid(0,4) == "TYPE")
	{
	  typeID = inLine.mid(5,10).toUInt();
	}
      else if(inLine.mid(0,5) == "ALIAS")
	{
	  icaoName = inLine.mid(6,100);
	}
      else if(inLine.mid(0,5) == "LTYPE")
	    {
              lLimitType = inLine.mid(6,1).toUInt();
	    }
      else if(inLine.mid(0,5) == "UTYPE")
	{
	  uLimitType = inLine.mid(6,1).toUInt();
	}
      else if(inLine.mid(0,5) == "LOWER")
	{
	  lLimit = inLine.mid(6,100).toUInt();
	}
      else if(inLine.mid(0,5) == "UPPER")
	{
	  uLimit = inLine.mid(6,100).toUInt();
	}
      else if(((inLine.mid(0,1) >= "0")  && (inLine.mid(0,1) <= "9")) && (inLine.mid(0,1) == "-"))
	{
	  unsigned int loop;
	  for(loop = 0; loop < strlen(inLine); loop++)
	    if(inLine.mid(loop, 1) == " ") break;
	  
	  lat_temp = inLine.left(loop).toInt();
	  lon_temp = inLine.mid((loop + 1), inLine.length() - loop).toInt();
	  positionList.append(new QPoint(lat_temp, lon_temp));

	  // Bounding Box bestimmen
	  if(lat_temp > bound_n)
	    {
	      bound_n = lat_temp;
	    }
	  else if(lat_temp < bound_s)
	    {
	      bound_s = lat_temp;
	    }

	  if(lon_temp > bound_e)
	    {
	      bound_e = lon_temp;
	    }
	  else if(lon_temp < bound_w)
	    {
	      bound_w = lon_temp;
	    }
	}
      else if(inLine.mid(0,5) == "[END]")
	{
	  // kacheln aufrufen
	}
    }
}



void kacheln (int bound_n, int bound_s, int bound_w, int bound_e,
	      QList<QPoint> positionList, struct header)
{
  // Kacheln erstellen und durchgehhen



  if((bound_n < border_s || bound_s > border_n) || 
     (bound_e < border_w && bound_w > border_e))
    {
      return;
    }
  
  //Objekt in Kachel Schreiben!
  QFile outFile("/opt/kde2/share/apps/kflog/mapdata/airspace/"
		+ QFileInfo(inFileName).baseName() + "_airspace.kfl");
  QTextStream outStream(&outFile);
  outFile.open(IO_WriteOnly);
}




