//
// Erzeugt gekachelte KFLog Ascii Daten aus den konvertierten VMAP Daten
//
//

#include <iostream.h>
#include <stdlib.h>
#include <cmath>

#include <qdatetime.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qpoint.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qdatastream.h>

#include "gpc.h"

#include <resource.h>

int bound_s, bound_n, bound_w, bound_e;
int total_items = 0;
int old_id = -1;
int data_type;
// 0 Linie
// 1 Punkt
// 2 Area

QList<QPoint> positionList;

struct header
{
  QString name;
  int id;
  int type;
  int elevation;
  int sort;
//  QString description;
  unsigned int lm_typ;
};


#define MALLOC(p, b, s)    {if ((b) > 0) { \
                            p= malloc(b); if (!(p)) { \
                            fprintf(stderr, "gpc malloc failure: %s\n", s); \
                                        exit(0);}} else p= NULL;}

void out_gpc_polygon(gpc_polygon *p, QTextStream &outStream, header obj_header)
{
  for(int n = 0; n < p->num_contours; n++)
    {
      outStream << "[NEW]\n";
      outStream << "ID=" << obj_header.id << endl;
      outStream << "TYPE=" << obj_header.type << endl;
      outStream << "NAME=" << obj_header.name << endl;
      outStream << "ELEV=" << obj_header.elevation << endl;
//      outStream << "DESC=" << obj_header.name << endl;
      outStream << "SORT=" << obj_header.sort << endl;
      outStream << "LM_TYP=" << obj_header.lm_typ << endl;

      for(int f = 0; f < p->contour[n].num_vertices; f++)
        {
          outStream << (int)p->contour[n].vertex[f].x << " " << (int)p->contour[n].vertex[f].y << endl;
        }
      outStream << "[END]\n";
    }
}


void area_kacheln (int bound_n, int bound_s, int bound_w, int bound_e,
                   QList<QPoint> positionList, struct header obj_header, QString outDir)
{
  int border_s, border_n, border_w, border_e;

  // Kacheln erstellen und durchgehhen
  border_w = bound_w / 600000 / 2  * 2;
  border_e = bound_e / 600000 / 2  * 2;
  border_s = bound_s / 600000 / 2  * 2;
  border_n = bound_n / 600000 / 2  * 2;

  if(bound_w < 0) { border_w -=2; }
  if(bound_e < 0) { border_e -=2; }
  if(bound_n < 0) { border_n -=2; }
  if(bound_s < 0) { border_s -=2; }

  /*  cout << " W " << border_w
      << " E " << border_e
      << " N " << border_n
      << " S " << border_s << endl; */

  // GPC Polygon erzeugen
  gpc_polygon* result_polygon = new gpc_polygon;
  gpc_polygon* element_polygon = new gpc_polygon;

  element_polygon->contour = new gpc_vertex_list;
  element_polygon->contour->vertex = new gpc_vertex[1];

  element_polygon->contour->vertex =
      (gpc_vertex*)realloc(element_polygon->contour->vertex,
          positionList.count() * 2 * sizeof(double));

  element_polygon->num_contours = 1;
  element_polygon->hole = new int[1];
  element_polygon->hole[0] = 0;
  element_polygon->contour->num_vertices = positionList.count();

  for(unsigned int n = 0; n < positionList.count(); n++)
    {
      element_polygon->contour->vertex[n].x = positionList.at(n)->x();
      element_polygon->contour->vertex[n].y = positionList.at(n)->y();
    }

  // GPC Kachel Polygon
  gpc_polygon* kachel_polygon = new gpc_polygon;
  kachel_polygon->contour =  new gpc_vertex_list;
  kachel_polygon->contour->vertex = new gpc_vertex[1];

  kachel_polygon->contour->vertex =
      (gpc_vertex*)realloc(kachel_polygon->contour->vertex, 5 * 2 * sizeof(double));

  kachel_polygon->num_contours = 1;

  kachel_polygon->hole = new int[1];
  kachel_polygon->hole[0] = 0;

  kachel_polygon->contour->num_vertices = 5;

  int ueberlapp = 0;

  for(int lon_loop = border_w; lon_loop <= border_e; lon_loop += 2)
    {
      for(int lat_loop = border_s; lat_loop <= border_n; lat_loop += 2)
        {
          // Kachel Polygon
          kachel_polygon->contour->vertex[0].y = lon_loop * 600000 - ueberlapp;
          kachel_polygon->contour->vertex[0].x = (lat_loop + 2) * 600000 + ueberlapp;

          kachel_polygon->contour->vertex[1].y = (lon_loop + 2) * 600000 + ueberlapp;
          kachel_polygon->contour->vertex[1].x = (lat_loop + 2) * 600000 + ueberlapp;

          kachel_polygon->contour->vertex[2].y = (lon_loop + 2) * 600000 + ueberlapp;
          kachel_polygon->contour->vertex[2].x = lat_loop * 600000 - ueberlapp;

          kachel_polygon->contour->vertex[3].y = lon_loop * 600000 - ueberlapp;
          kachel_polygon->contour->vertex[3].x = lat_loop * 600000 - ueberlapp;

          kachel_polygon->contour->vertex[4].y = lon_loop * 600000 - ueberlapp;
          kachel_polygon->contour->vertex[4].x = (lat_loop + 2) * 600000 + ueberlapp;

          // verschneiden
          gpc_polygon_clip(GPC_INT, element_polygon, kachel_polygon, result_polygon);

          //Objekt in Kachel Schreiben!
          QString filename;
          char* lat;
          char* lon;

          lat = "N";
          lon = "E";
          if(lat_loop < 0) lat = "S";
          if(lon_loop < 0) lon = "W";

          filename.sprintf("%d%s_%d%s.out", abs(lat_loop), lat, abs(lon_loop), lon);

          QFile outFile(outDir + "/" + filename);
          QTextStream outStream(&outFile);
          outFile.open(IO_WriteOnly | IO_Append);


          // Polygon schreiben
          out_gpc_polygon(result_polygon, outStream, obj_header);

          outFile.close();
          gpc_free_polygon(result_polygon);
        }
    }


  gpc_free_polygon(kachel_polygon);
  gpc_free_polygon(element_polygon);
}


void line_kacheln (int bound_n, int bound_s, int bound_w, int bound_e,
                   QList<QPoint> positionList, struct header obj_header, QString outDir)
{
  int border_s, border_n, border_w, border_e;
  int kachel_s, kachel_n, kachel_w, kachel_e;

  //
  kachel_s = bound_s / 2  * 2;
  kachel_n = bound_n / 2  * 2;
  kachel_e = bound_e / 2  * 2;
  kachel_w = bound_w / 2  * 2;

  // Kacheln erstellen und durchgehhen
  border_w = bound_w / 600000 / 2  * 2;
  border_e = bound_e / 600000 / 2  * 2;
  border_s = bound_s / 600000 / 2  * 2;
  border_n = bound_n / 600000 / 2  * 2;

  if(bound_w < 0) { border_w -=2; }
  if(bound_e < 0) { border_e -=2; }
  if(bound_n < 0) { border_n -=2; }
  if(bound_s < 0) { border_s -=2; }


  for(int lon_loop = border_w; lon_loop <= border_e; lon_loop += 2)
    {
      for(int lat_loop = border_s; lat_loop <= border_n; lat_loop += 2)
        {
          //Objekt in Kachel Schreiben!
          QString filename;
          char* lat;
          char* lon;

          int old_n = 0;
          bool new_element = true;

          lat = "N";
          lon = "E";
          if(lat_loop < 0) lat = "S";
          if(lon_loop < 0) lon = "W";

          filename.sprintf("%d%s_%d%s.out",abs(lat_loop),lat,abs(lon_loop),lon);


          QFile outFile(outDir + filename);
          QTextStream outStream(&outFile);
          outFile.open(IO_WriteOnly | IO_Append);

          outStream << "[NEW]\n";
          outStream << "ID=" << obj_header.id << endl;
          outStream << "TYPE=" << obj_header.type << endl;
          outStream << "NAME=" << obj_header.name << endl;
          outStream << "ELEV=" << obj_header.elevation << endl;
          outStream << "LM_TYP=" << obj_header.lm_typ << endl;
//          outStream << "DESC=" << obj_header.description << endl;
          outStream << "SORT=" << obj_header.sort << endl;

          for(unsigned int n = 0; n < positionList.count(); n++)
            {

              if(positionList.at(n)->x() <= kachel_n && positionList.at(n)->x() >= kachel_s &&
                 positionList.at(n)->y() <= kachel_e && positionList.at(n)->y() >= kachel_w)
                {
                  if(n > 0 && new_element)
                    {
                      new_element = false;
                      outStream << positionList.at(n - 1)->x() << " "
                                << positionList.at(n - 1)->y() << endl;

                    }

                  outStream << positionList.at(n)->x() << " "
                            << positionList.at(n)->y() << endl;


                  old_n = n;
                }

            }


          if(old_n < positionList.count() - 1)
            {
              outStream << positionList.at(old_n + 1)->x() << " "
                        << positionList.at(old_n + 1)->y() << endl;
            }
          outStream << "[END]\n";

          outFile.close();
        }
    }
}

//einlesen
void read_file(QString inFileName, QString outDir)
{

  int hyc, loc, tuc, zv2, id, dqdescr_id, fco, med;
  QString nam, f_code, feature_class, inLine;
  QFile inFile(inFileName);
  header obj_header;

  QTextStream inStream(&inFile);

  if(inFile.open(IO_ReadOnly))
    {
      int bound_s, bound_n, bound_w, bound_e;
      if(inFileName.right(1) == "a")
        {
          data_type = 2;
        }
      else if (inFileName.right(1) == "p")
        {
          data_type = 1;
        }
      else
        {
          data_type = 0;
        }

      warning("Beginne mit Einlesen (%d)\n\n", data_type);

      while(!inStream.eof())
        {
          inLine = inStream.readLine();

          if(inLine.mid(0,16) == "#    Total rows:")
            {
              // Gesamt Elemente
              total_items = inLine.mid(17,20).toInt();
            }
          else if(inLine.mid(0,1) == "#")
            {
              // Kommentar ignorieren
            }
          else if(inLine.mid(0,5) == "[NEW]")
            {
              //              warning("neues Element");
              obj_header.type = -1;
              obj_header.name = "";
              obj_header.lm_typ = 99;
//              obj_header.elevation = -1;
              obj_header.elevation = 0;
              obj_header.id = -1;
              obj_header.sort = 0;

              while(positionList.remove());

              bound_s = 90 * 600000;
              bound_n = -90 * 600000;
              bound_w = 180 * 600000;
              bound_e = -180 * 600000;

            }
          else if(inLine.mid(0,6) == "f_code")
            {
              f_code = inLine.mid(7,10);
            }
          else if(inLine.mid(0,2) == "id")
            {
              // Id
              obj_header.id = inLine.mid(3,10).toInt();

              // Löcher (Inseln)
              if(obj_header.id == old_id)
                {
                  obj_header.sort = 1;
                }
              old_id = obj_header.id;
            }
          else if(inLine.mid(0,3) == "zv2")
            {
              // Höhe
              zv2 = inLine.mid(4,10).toInt();
            }
          else if(inLine.mid(0,3) == "loc")
            {
              // Location Category
              loc = inLine.mid(4,10).toInt();
            }
          else if(inLine.mid(0,3) == "hyc")
            {
              // Hydrological Category
              hyc = inLine.mid(4,10).toInt();
            }
          else if(inLine.mid(0,3) == "nam")
            {
              nam = inLine.mid(4,50);
            }
          else if(inLine.mid(0,10) == "dqdescr_id")
            {
              dqdescr_id = inLine.mid(11,10).toInt();
            }
          else if(inLine.mid(0,13) == "feature_class")
            {
              feature_class = inLine.mid(14,100);
            }
          else if(inLine.mid(0,3) == "fco")
            {
              // Feature Configuration (Railroad)
              fco = inLine.mid(4,10).toInt();
            }
          else if(inLine.mid(0,3) == "med")
            {
              // Median Category (Road)
              med = inLine.mid(4,10).toInt();
            }
          else if(inLine.mid(0,3) == "tuc")
            {
              // Transportation use Category
              tuc = inLine.mid(4,10).toInt();
            }
          else if(inLine.mid(0,3) == "txt")
            {
              // Text attribute
              nam = inLine.mid(5,20);
            }
          else if( ( (inLine.mid(0,1) >= "0")  && (inLine.mid(0,1) <= "9") ) ||
                   (inLine.mid(0,1) == "-") )
            {
              unsigned int loop;
              for(loop = 0; loop < strlen(inLine); loop++)
                if(inLine.mid(loop, 1) == " ") break;

              int lat_temp = inLine.left(loop).toInt();
              int lon_temp = inLine.mid((loop + 1), inLine.length() - loop).toInt();
              positionList.append(new QPoint(lat_temp, lon_temp));

              // Bounding Box bestimmen
              if(lat_temp > bound_n)
                {
                  bound_n = lat_temp;
                }

              if(lat_temp < bound_s)
                {
                  bound_s = lat_temp;
                }

              if(lon_temp > bound_e)
                {
                  bound_e = lon_temp;
                }

              if(lon_temp < bound_w)
                {
                  bound_w = lon_temp;
                }
            }
          else if(inLine.mid(0,5) == "[END]")
            {

              //              warning("end %d"can,positionList.count());
              // Filter
              if((f_code != "") && (positionList.count() > 0))
                {

                  if(f_code == "FA001")
                    {
                      // Boundary Area
                      obj_header.type = ISOHYPSE;
//                      obj_header.type = BORDER;
                    }
                  else if((f_code == "CA030") || (f_code == "CA035"))
                    {
                      // Spot Elevation
                      // Inland Water Elevation
                      obj_header.type = SPOT;
                      obj_header.elevation = zv2;
                    }
                  else if(f_code == "BH090")
		    {
		      // Überschwemmungsgebiet
		    }
                  else if(f_code == "BH000")
                    {
                      if(data_type == 0)
                        {
                          // Aqeduct/Canal/Flume/
                          if(loc == 0 || loc == 8 || loc == 25)
                            {
                              obj_header.type = CANAL;
                              obj_header.name = nam;
                            }
                          else if(loc == 4)
                            {
                              // underground ignorieren
                            }
                          else
                            {
                              warning("loc = %d",loc);
                            }
                        }
                      else if(data_type == 2)
                        {
                          // Lakes
                          if(hyc == 6)
                            {
                              // Non Permanent
                              obj_header.type = LAKE_T;
                            }
                          else if(hyc == 8)
                            {
                              // Permanent
                              obj_header.type = LAKE;
                            }
                          else
                            {
                              warning("hyc = %d",hyc);
                            }
                          obj_header.name = nam;
                        }
                    }
                  else if(f_code == "BH140")
                    {
                      // River/Streams
                      if(hyc == 6)
                        {
                          // Non Permanent
                          obj_header.type = RIVER_T;
                        }
                      else if(hyc == 8)
                        {
                          // Permanent
                          obj_header.type = RIVER;
                        }
                      else
                        {
                          warning("hyc = %d",hyc);
                        }
                      obj_header.name = nam;
                    }
                  else if(f_code == "BB230")
                    {
                      // Seawall (Line)
                    }
                  else if((f_code == "BI020") || (f_code == "BI030"))
                    {
                      // Dam / Weir (Line/Point)
                      // Lock
                      if(data_type == 1)
                        {
                          obj_header.type = LANDMARK;
                          obj_header.lm_typ = LM_DAM;
                        }
                    }
                  else if((f_code == "AA010") || (f_code == "BH155"))
                    {
                      // Mine / Quarry
                      // Salt Evaporator
                      if(data_type == 1)
                        {
                          obj_header.type = LANDMARK;
                          obj_header.lm_typ = LM_MINE;
                        }
                    }
                  else if(f_code == "BH050")
                    {
                      // Fish Farm
                    }
                  else if((f_code == "AM070") || (f_code == "AM010"))
                    {
                      // Tank
                      // Depot
                      obj_header.type = LANDMARK;
                      obj_header.lm_typ = LM_DEPOT;
                    }
                  else if(f_code == "AM080")
                    {
                      // Water Tower
                      obj_header.type = LANDMARK;
                      obj_header.lm_typ = LM_TOWER;
                    }
                  else if(f_code == "BJ070" || f_code == "BJ065")
                    {
                      // Pack Ice
		      // Shelf Ice
                      obj_header.type = PACK_ICE;
                      obj_header.name = nam;
                    }
                  else if((f_code == "BJ080") || (f_code == "BJ100"))
                    {
                      // Polar Ice
                      // Snow / Ice Field
                      obj_header.type = GLACIER;
                      obj_header.name = nam;
                    }
                  else if(f_code == "AL020")
                    {
                      // Build Up Area
                      if(data_type == 1)
                        {
                          // Dorf
                          obj_header.type = VILLAGE;
                        }
                      else
                        {
                          // Stadt
                          obj_header.type = CITY;
                        }
                      obj_header.name = nam;
                    }
                  else if(f_code == "AN010")
                    {
                      // Railroad
                      if(fco == 0 || fco == 3)
                        {
                          // Unknown
                          // Single
                          obj_header.type = RAILWAY;
                        }
                      else if(fco == 2)
                        {
                          // multiple
                          obj_header.type = RAILWAY_D;
                        }
                      else
                        {
                          warning("fco = %d",fco);
                        }
                    }
                  else if(f_code == "AP030")
                    {
                      // Road
                      if(med == 0 || med == 2)
                        {
                          // Unknown
                          // without median
                          obj_header.type = ROAD;
                        }
                      else if(med == 1)
                        {
                          // median
                          obj_header.type = HIGHWAY;
                        }
                      else
                        {
                          warning("med = %d",med);
                        }
                    }
                  else if(f_code == "AP050")
                    {
                      // Trail
                      obj_header.type = TRAIL;
                    }
                  else if(f_code == "AQ040")
                    {
                      // Bridge/overpass/Viaduct
                      if(data_type == 1)
                        {
                          obj_header.type = LANDMARK;
                          obj_header.lm_typ = LM_BRIDGE;
                        }
                    }
                  else if(f_code == "AQ010")
                    {
                      // Aerial Cable / ski lift
                      obj_header.type = AERIAL_CABLE;
                    }
                  else if(f_code == "AN060")
                    {
                      // Railroad Yard
                      obj_header.type = LANDMARK;
                      obj_header.lm_typ = LM_STATION;
                    }
                  else if(f_code == "AQ064")
                    {
                      // Cause Way
                    }
                  else if(f_code == "AQ113")
                    {
                      // Pipeline
                    }
                  else if(f_code == "AT130")
                    {
                      // Power Transmission Line
                    }
                  else if(f_code == "AT060")
                    {
                      // Telephone Line
                    }
                  else if((f_code == "AD010") || (f_code == "AD030") ||
                          (f_code == "AQ116") || (f_code == "ZD040") ||
                          (f_code == "AC040") || (f_code == "AC000"))
                    {
                      // Power Plant
                      // SubStation / Transformer Yard
                      // Pumping Station
                      // Named LOcation
                      // Oil / Gas
                      // Processing Plant (Point)
                      obj_header.type = LANDMARK;
                      obj_header.lm_typ = LM_INDUSTRY;
                    }
                  else if(f_code == "EC030")
                    {
                      // Trees
                      obj_header.type = FOREST;
                      obj_header.name = nam;
                    }
                  else
                    {
                      // kein brauchbares Element !
                      warning("Unknown Element %s Id: %d",(const char*)f_code,obj_header.id);
                    }

                  if(obj_header.type != -1)
                    {
                      cout << "\r";
                      cout << "id: ";
                      cout.width(10);
                      cout << obj_header.id << " von ";
                      cout.width(10);
                      cout << total_items << " ( ";
                      cout.precision(1);
                      cout.width(4);
                      cout.setf(ios::fixed);
                      cout << (float)((float)obj_header.id / (float)total_items * 100) << "%)";

                      if(data_type == 0 || data_type == 1)
                        {
                          // Linien oder Punkte
                          line_kacheln(bound_n, bound_s, bound_w, bound_e,
                                       positionList, obj_header, outDir);
                        }
                      else
                        {
                          // Gebiete

                          // kacheln aufrufen
                          area_kacheln(bound_n, bound_s, bound_w, bound_e,
                                       positionList, obj_header, outDir);
                        }
                    }
                }
            }
        }
    }
}



int main(int argc, char* argv[])
{
  if(argc != 3)
    {
      warning("Usage: %s <vpf_file> <output_dir>", argv[0]);
      return 1;
    }

  QString vpfFile(argv[1]);
  QString outDir(argv[2]);


  // Get coverage und feature from the filename:
  QString coverage(vpfFile.left(vpfFile.find("_") - 1));
  QString feature(vpfFile.mid(vpfFile.find("_"), 20));

  read_file(vpfFile,outDir);

  cout << "\nVerarbeitung beendet!\n" ;
}
