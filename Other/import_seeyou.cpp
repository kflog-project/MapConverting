/*
 * Programmmodul für KFLog zum Importieren von SeeYou Luftraum Dateien
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
   * Splittet den Strings an den Kommas auf und gibt eine QList 
   * zurück
   */
  QList<QString> out;
  int index = 0;
  int oldindex = 0;
  
  string = string.insert(0," ");
  
  while(index != -1)
    {
      oldindex = index + 1;
      index = string.find(",",oldindex);
      
      QString* temp = new QString(string.mid(oldindex,index - oldindex));
      
      out.append(temp);
    }

  return out;
}

int convert_pos(int pos, float* min, int* sec)
{
  /*
   * Wandelt die Zehtausendstel MIn in min sec um
   *
   */
  *min = pos / 600000;

  sec = 0;

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
  QFile in_file("europe.car");

  QString line, name, temp;
  bool newobject = false;
  int type, x, y, low, low_id, high, high_id;
  int points = 0;
  int ignore_c = 0;

  unsigned int count = 0;
  
  if(!in_file.open(IO_ReadOnly)) 
    {
      //      warning("KFLog: Can not open airspacefile %s", file);
      return false;
    }

  QTextStream in(&in_file);

  QList<QString> neu;

  while(!in.eof()) 
    {
      line = in.readLine();
      neu = split_string(line);

      if(*neu.at(0) == "H")
	{
	  y = 0;
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
	  */
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
}
