/*
 * Programmmodul für KFLog zum Importieren von csv Wendepunkt Dateien
 *
 *
 * 
 *
 */
#include <qfile.h>
#include <qlist.h>
#include <qdatastream.h>
#include <qtextstream.h>
#include <iostream.h>


QList<QString> split_string(QString string)
{
  /*
   * Splittet den Strings an den semicolons auf und gibt eine QList 
   * zurück
   */
  QList<QString> out;
  int index = 0;
  int oldindex = 0;
  
  string = string.insert(0," ");
  
  while(index != -1)
    {
      oldindex = index + 1;
      index = string.find(";",oldindex);
      
      QString* temp = new QString(string.mid(oldindex,index - oldindex));
      
      out.append(temp);
    }

  return out;
}

int convert_pos(QString in)
{
  /*
   * Wandelt die Zehtausendstel MIn in min sec um
   *
   */
  int grad = 0;
  int min = 0;
  int t_min = 0;
  int erg = 0;
  
  if(in.mid(3,1) == ".") 
    {
      grad = in.mid(0,3).toInt();
      min = in.mid(4,2).toInt();
      t_min = in.mid(7,3).toInt();
    }
  else 
    {
      grad = in.mid(0,2).toInt();
      min = in.mid(3,2).toInt();
      t_min = in.mid(6,3).toInt();
    }

  if((in.mid(10,1) == "S") || (in.mid(11,1) == "W"))
    {
      grad = grad * -1;
    }


  erg = (grad * 600000) + (min * 10000) + t_min * 10;

  return erg;

}

int get_elevation(QString string, int* id, int* elev)
{
  /*
   *  Bekommt ein String mit einer Höhenangabe
   *  Gibt eine Höhenartenindex und die Höhe aus
   */

  if(string.contains("FL",false))
    {
      *id = 3;
      *elev = string.remove(0,2).toInt();
    }
  else if(string.contains("MSL",false))
    {
      *id = 1;
      *elev = string.remove(string.length() - 3,3).toInt();
    }
  else if(string.contains("GND",false))
    {
      *id = 2;
      *elev = 0;
    }
  else if(string.contains("NOTAM",false))
    {
      *id = 4;
      *elev = -1;
    }
  else if(string.contains("UNLTD",false))
    {
      /// ??? Unbegrenzt
      *id = 1;
      *elev = 99999;
    }
  else if(string.contains("AGL",false))
    {
      // GND ???
      *id = 2;
      *elev = string.remove(string.length() - 3,3).toInt();
    }
    
}


int main()
{
  QFile in_file("Flugplätze.csv");

  QString line, name, short_name, description, temp, type_name, rwy_surface_name, icao, frequency;
  bool newobject = false;
  int type, elevation, country,
    rwy_landable, rwy_length, rwy_direc, rwy_surface, rwy_bidirectional, lat, lon;
  int points = 0;
  int ignore_c = 0;

  unsigned int count = 0;
  
  if(!in_file.open(IO_ReadOnly)) 
    {
      //      warning("KFLog: Can not open file %s", file);
      return false;
    }

  QTextStream in(&in_file);

  QList<QString> neu;

  while(!in.eof()) 
    {
      line = in.readLine();
      neu = split_string(line);

      // Felder
      name = *neu.at(0);
      short_name = *neu.at(1);
      description = *neu.at(2);

      lat  = convert_pos(*neu.at(3));
      lon = convert_pos(*neu.at(4));

      elevation = neu.at(5)->toInt();

      type = neu.at(6)->toInt();
      type_name = *neu.at(7);

      country = neu.at(8)->toInt();

      icao = *neu.at(9);

      frequency = *neu.at(10);

      rwy_landable = neu.at(11)->toInt();
      rwy_length = neu.at(12)->toInt();
      rwy_direc = neu.at(13)->toInt();
      rwy_surface = neu.at(14)->toInt();
      rwy_surface_name = *neu.at(15);
      rwy_bidirectional = neu.at(16)->toInt();

      // Ausgeben
      switch (type)
	{
	case 1024:
	  // Flughafen
	  type = 2;
	  break;
	case 1792:
	  // Segelfluggelände
	  type = 10;
	  break;
	case 256:
	  // Internationaler Flughafen
	  type = 1;
	  break;
	case 2816:
	  // Außenlandefeld
	  type = 15;
	  break;
	case 512:
	  // mil. Flugplatz
	  type = 3;
	  break;
	case 768:
	  // Sonderlandeplatz
	  type = 5;
	  break;
	}



      switch (country)
	{
	case 1060:
	  // Deutschland
	  break;
	}

      switch (rwy_surface)
	{
	case 0:
	  // Beton
	  break;
	case 1:
	  // Asphalt
	  break;
	case 3:
	  // Gras
	  break;
	case 4:
	  // sonstiger Belag
	  break;
	case 5:
	  // unbekannt
	  break;
	}


      // Ausgabe
      cout << "[NEW]"      << endl;
      cout << "TYPE="      << type       << endl;
      cout << "ALIAS="     << icao       << endl;
      cout << "ABBREV="    << short_name << endl;
      cout << "AT"         << " " << lat << " " << lon << endl;
      cout << "ELEVATION=" << elevation << endl;
      cout << "FREQUENCY=" << frequency << endl;
      cout << "NAME="      << name << endl;
      cout << "RUNWAY" << " " << rwy_direc
               	       << " " << rwy_length
	               << " " << rwy_surface << endl;
      cout << "[END]" << endl;


      
    }
      /*
	  x = 0;
	  int elev = -1;
	  int id = -1;

	  newobject = true;
	  
	  type = neu.at(1)->toInt();
	  points = neu.at(2)->toInt();
	  name = *neu.at(3);


	  switch(type)
	    {
	    case 1:
	      // Luftraum D + F (CTR und nicht)
	      type = 24;
	      break;
	    case 2:
	      // Flugbeschränkungsgebiet (R)
	      type = 32;
	      break;
	    case 3:
	      // Sperrgebiet (P)
	      type = 32;
	      break;
	    case 4:
	      // Gefahrengebiet (D)
	      type = 30;
	      break;
	    case 5:
	      // Tieffluggebiet
	      type = 31;
	      break;
	    case 6:
	      // Luftraum C
	      type = 23;
	      break;
	    case 9:
	      // Kontrollbezirk
	      // wird von uns nicht verwendet
	      type = 100;
	      break;
	    default:
	      break;
	    }


	  //Höhen
	  temp = *neu.at(8);
	  get_elevation(*neu.at(8),&id,&elev);
	  low = elev;
	  low_id = id;
	  get_elevation(*neu.at(9),&id,&elev);
	  high = elev;
	  high_id = id;

	  if(low_id == 2 && low == 0 && type == 22)
	    {
	      // Kontrolzone
	      type = 29;
	    }
	  

	  if(type == 31 && low != 0)
	    {
	      	      cerr << temp << endl;
	      // cerr << "Ey we've got a real low flying area!!\n";
	      ignore_c++;
	    }
	  if((low_id == 3 && low >= 100) || type == 100)
	    {
	      newobject = false;

	      ignore_c++;

	      //	      cerr << "High Level Airspace!! Ignoring...     " << temp << "   " << ignore_c << endl;

	    }
	  else {
	    cout << "[NEW]\n";
	    cout << "TYPE=" << type << endl;

	    cout << "LOWER=" << low << endl;
	    cout << "LTYPE=" << low_id << endl;
	    cout << "UPPER=" << high << endl;
	    cout << "UTYPE=" << high_id << endl;
	    
	    cout << "NAME=" << name << endl;
	  }
	}
      else if(newobject)
	{
	  // nun Punkte einlesen
	  

	  x = neu.at(1)->toFloat() * 600000.0;
	  y = neu.at(2)->toFloat() * 600000.0;
	  /* if(y < 0)
	     {
	     y = 0;
	     }
	     if(x < 0)
	     {
	     x = 0;
	     }

	  //	  cout << y << ".00'00\"N" << " " << x << ".00'00\"E" << endl;
	  cout << y << " " << x << endl;
	  count++;
	  if(count == points)
	    {
	      x = 0;
	      y = 0;

	      newobject = false;
	      cout << "[END]\n";

	      count = 0;
	    }
	}
    }
  cerr << ignore_c << endl;

*/
}
