/*
 * $Id$
 *
 *  Created on: 16.11.2011
 *      Author: axel
 *
 * This file is distributed under the terms of the General Public License.
 *
 * This tool makes a replace of motorway, road or railway polylines in a KFLog
 * map tile. The input data file is expected as data stream of QPolygons, which
 * contain the new polylines with their coordinate pairs (latitude/longitude)
 * as decimal degrees using WGS84 format. Such data file can be created with
 * the polylineDict tool.
 *
 */

using namespace std;

#include <cmath>
#include <iostream>
#include <climits>

#include <QtGui>

#include "resource.h"

// versions
#define FILE_FORMAT_ID        100 // used to handle a previous version
#define FILE_VERSION_GROUND   102
#define FILE_VERSION_TERRAIN  102
#define FILE_VERSION_MAP      101

#define FILE_TYPE_MAP         0x4d

#define READ_POINT_LIST \
  {\
    inm >> locLength;\
    all.resize(locLength);\
    for(uint i = 0; i < locLength; i++) \
    { \
      inm >> lat_temp; \
      inm >> lon_temp; \
      all.setPoint(i, QPoint(lat_temp, lon_temp)); \
    }\
  };

#define SKIP_POINT_LIST \
  {\
    inm >> locLength;\
    for(uint i = 0; i < locLength; i++) \
    { \
      inm >> lat_temp; \
      inm >> lon_temp; \
    }\
  };

#define RW_POINT_LIST \
  {\
    inm >> locLength;\
    out << locLength;\
    for(uint i = 0; i < locLength; i++) \
    { \
      inm >> lat_temp; \
      inm >> lon_temp; \
      out << lat_temp; \
      out << lon_temp; \
    }\
  };

class BaseMapElement
{
public:
  enum objectType
  {
    NotSelected = NOT_SELECTED,
    IntAirport = INT_AIRPORT,
    Airport = AIRPORT,
    MilAirport = MIL_AIRPORT,
    CivMilAirport = CIVMIL_AIRPORT,
    Airfield = AIRFIELD,
    ClosedAirfield = CLOSED_AIRFIELD,
    CivHeliport = CIV_HELIPORT,
    MilHeliport = MIL_HELIPORT,
    AmbHeliport = AMB_HELIPORT,
    Gliderfield = GLIDERFIELD,
    UltraLight = ULTRALIGHT,
    HangGlider = HANGGLIDER,
    Parachute = PARACHUTE,
    Balloon = BALLOON,
    Outlanding = OUTLANDING,
    Vor = VOR,
    VorDme = VORDME,
    VorTac = VORTAC,
    Ndb = NDB,
    CompPoint = COMPPOINT,
    AirA = AIR_A,
    AirB = AIR_B,
    AirC = AIR_C,
    AirD = AIR_D,
    AirE = AIR_E,
    WaveWindow = WAVE_WINDOW,
    AirF = AIR_F,
    ControlC = CONTROL_C,
    ControlD = CONTROL_D,
    Danger = DANGER,
    LowFlight = LOW_FLIGHT,
    Restricted = RESTRICTED,
    Prohibited = PROHIBITED,
    Tmz = TMZ,
    GliderSector = GLIDER_SECTOR,
    Obstacle = OBSTACLE,
    LightObstacle = LIGHT_OBSTACLE,
    ObstacleGroup = OBSTACLE_GROUP,
    LightObstacleGroup = LIGHT_OBSTACLE_GROUP,
    Spot = SPOT,
    Isohypse = ISOHYPSE,
    Glacier = GLACIER,
    PackIce = PACK_ICE,
    Border = BORDER,
    City = CITY,
    Village = VILLAGE,
    Landmark = LANDMARK,
    Motorway = MOTORWAY,
    Road = ROAD,
    Railway = RAILWAY,
    AerialRailway = AERIAL_CABLE,
    Lake = LAKE,
    River = RIVER,
    Canal = CANAL,
    Flight = FLIGHT,
    Task = FLIGHT_TASK,
    Trail = TRAIL,
    Railway_D = RAILWAY_D,
    Aerial_Cable = AERIAL_CABLE,
    River_T = RIVER_T,
    Lake_T = LAKE_T,
    Forest = FOREST,
    Turnpoint = TURNPOINT,
    Thermal = THERMAL,
    FlightGroup = FLIGHT_GROUP,
    FAIAreaLow500 = FAI_AREA_LOW,
    FAIAreaHigh500 = FAI_AREA_HIGH,
    EmptyPoint = EMPTY_POINT, // new type for nothing to draw
    objectTypeSize
  /* leave this at the end */
  };
};

bool getPolylineTile(QString fileName, int tileNo, QList<QPolygon>& plList);

QRectF getTileBox(const int tileNo);

// ===========================================================================
// Usage of program
// ===========================================================================

void usage(char * progName)
{
  cerr
      << "KFlog Polyline replacer V1.0, Axel Pauli (axel@kflog.org)"
      << endl
      << "Usage: "
      << basename(progName)
      << " -d <input OSM-polyline-file>" << endl
      << " -m <input KFLog-map-tile-file>" << endl
      << " -o <output kflog-map-tile-file>" << endl
      << " -r <motorway|rail|road> (the item to be replaced)" << endl
      << endl << endl;

  exit(-1);
}

int main(int argc, char *argv[])
{
  if (argc < 9)
    {
      cerr << "Missing mandatory arguments" << endl << endl;
      usage(argv[0]);
    }

  QString dFileName;
  QString mFileName;
  QString oFileName;
  QString replace;

  int c;

  while ((c = getopt(argc, argv, "d:m:o:r:")) != -1)
    {
      switch (c)
        {
        case 'd':
          dFileName = optarg;
          break;
        case 'm':
          mFileName = optarg;
          break;
        case 'o':
          oFileName = optarg;
          break;
        case 'r':
          replace = optarg;

          if( replace != "motorway" && replace != "rail" && replace != "road")
            {
              cerr << "Wrong argument for -r: " << optarg << "!" << endl;
              usage(argv[0]);
            }
          break;
        case '?':
          {
            cerr << "Unrecognized option: -" << optopt << "!" << endl;
            usage(argv[0]);
            break;
          }
        }
    }

  QList<QPolygon> plList;

  bool ok;

  int tileNo = QFileInfo(mFileName).baseName().remove(0, 2).toInt(&ok);

  if( ! ok )
    {
      qWarning() << "Wrong map tile number contained in file name" << mFileName;
      return -1;
    }

  ok = getPolylineTile( dFileName, tileNo, plList );

  if (!ok)
    {
      return -1;
    }

  qDebug() << plList.size() << "polylines are contained in the new map tile.";

  QFile mFile(mFileName);

  if (!mFile.open(QIODevice::ReadOnly))
    {
      qWarning() << "Cannot open input file: " << dFileName;
      return -1;
    }

  QFile oFile(oFileName);

  if (!oFile.open(QIODevice::WriteOnly))
    {
      mFile.close();
      qWarning() << "Cannot open output file: " << oFileName;
      return -1;
    }

  QDataStream inm(&mFile);
  QDataStream out(&oFile);

  // qDebug("reading file %s", pathName.toLatin1().data());

  qint8 loadTypeID;
  quint16 loadSecID, formatID;
  quint32 magic;
  QDateTime createDateTime;

  inm.setVersion(QDataStream::Qt_2_0);
  out.setVersion(QDataStream::Qt_2_0);
  // inm.setVersion( QDataStream::Qt_4_7 );

  inm >> magic;
  out << magic;

  if (magic != KFLOG_FILE_MAGIC)
    {
      qWarning("Wrong magic key %x read! Aborting ...", magic);
      return -1;
    }

  inm >> loadTypeID;
  out << loadTypeID;

  /** Originally, the binary files were mend to come in different flavors.
   * Now, they are all of type 'm'. Use that fact to do check for the
   * compiled or the uncompiled version.
   */
  if (loadTypeID != FILE_TYPE_MAP)
    {
      qWarning("Wrong load type identifier %x read! Aborting ...", loadTypeID);
      return -1;
    }

  // Check the version of the subtype. This can be different for the
  // compiled and the uncompiled version.
  inm >> formatID;
  inm >> loadSecID;
  inm >> createDateTime;

  out << formatID;
  out << loadSecID;
  out << QDateTime::currentDateTimeUtc();

  if (formatID < FILE_VERSION_MAP)
    {
      // to old ...
      qWarning("Map File format too old! (version %d, expecting: %d) "
          "Aborting ...", formatID, FILE_VERSION_MAP);
      return -1;
    }
  else if (formatID > FILE_VERSION_MAP)
    {
      // to new ...
      qWarning("Map File format too new! (version %d, expecting: %d) "
          "Aborting ...", formatID, FILE_VERSION_MAP);
      return -1;
    }

  quint8 lm_typ;
  qint8 sort, elev;
  qint32 lat_temp, lon_temp;
  quint32 locLength = 0;
  QString name;

  int polylines = 0;

  /**
   * Read and write map elements. The map element type to be replaced is read
   * from the binary input file but not more written into the output file.
   */
  while (!inm.atEnd())
    {
      BaseMapElement::objectType typeIn = BaseMapElement::NotSelected;
      inm >> (quint8&) typeIn;

      locLength = 0;
      name = "";

      QPolygon all;
      QPoint single;

      switch (typeIn)
        {
        case BaseMapElement::Motorway:
          {
            if( replace == "motorway" )
              {
                polylines++;
                inm >> locLength;

                for(uint i = 0; i < locLength; i++)
                  {
                    inm >> lat_temp;
                    inm >> lon_temp;
                  }
              }
            else
              {
                out << quint8(typeIn);
                RW_POINT_LIST
              }

            break;
          }

        case BaseMapElement::Road:
          {
            if( replace == "road" )
              {
                polylines++;
                inm >> locLength;

                for(uint i = 0; i < locLength; i++)
                  {
                    inm >> lat_temp;
                    inm >> lon_temp;
                  }
              }
            else
              {
                out << quint8(typeIn);
                RW_POINT_LIST
              }

            break;
          }

        case BaseMapElement::Trail:
        case BaseMapElement::Aerial_Cable:
          out << quint8(typeIn);
          RW_POINT_LIST
          break;

        case BaseMapElement::Railway:
        case BaseMapElement::Railway_D:
          {
            if( replace == "rail" )
              {
                polylines++;
                inm >> locLength;

                for(uint i = 0; i < locLength; i++)
                  {
                    inm >> lat_temp;
                    inm >> lon_temp;
                  }
              }
            else
              {
                out << quint8(typeIn);
                RW_POINT_LIST
              }

            break;
          }

        case BaseMapElement::Canal:
        case BaseMapElement::River:
        case BaseMapElement::River_T:

          out << quint8(typeIn);

          if (formatID >= FILE_FORMAT_ID)
            {
              inm >> name;
              out << name;
            }

          RW_POINT_LIST
          break;

        case BaseMapElement::City:
          {
            out << quint8(typeIn);
            inm >> sort;
            out << sort;

            if (formatID >= FILE_FORMAT_ID)
              {
                inm >> name;
                out << name;
              }

            RW_POINT_LIST
            break;
          }

        case BaseMapElement::Lake:
        case BaseMapElement::Lake_T:
          out << quint8(typeIn);
          inm >> sort;
          out << sort;

          if (formatID >= FILE_FORMAT_ID)
            {
              inm >> name;
              out << name;
            }

          RW_POINT_LIST
          break;

        case BaseMapElement::Forest:
        case BaseMapElement::Glacier:
        case BaseMapElement::PackIce:

          out << quint8(typeIn);
          inm >> sort;
          out << sort;

          if (formatID >= FILE_FORMAT_ID)
            {
              inm >> name;
              out << name;
            }

          RW_POINT_LIST
          break;

        case BaseMapElement::Village:

          out << quint8(typeIn);

          if (formatID >= FILE_FORMAT_ID)
            {
              inm >> name;
              out << name;
            }

          inm >> lat_temp;
          inm >> lon_temp;
          out << lat_temp;
          out << lon_temp;
          break;

        case BaseMapElement::Spot:

          out << quint8(typeIn);

          if (formatID >= FILE_FORMAT_ID)
            {
              inm >> elev;
              out << elev;
            }

          inm >> lat_temp;
          inm >> lon_temp;
          out << lat_temp;
          out << lon_temp;
          break;

        case BaseMapElement::Landmark:

          out << quint8(typeIn);

          if (formatID >= FILE_FORMAT_ID)
            {
              inm >> lm_typ;
              inm >> name;
              out << lm_typ;
              out << name;
            }

          inm >> lat_temp;
          inm >> lon_temp;
          out << lat_temp;
          out << lon_temp;
          break;

        default:

          qWarning("Map type not handled in switch: %d", typeIn);
          break;
          }
    }

  qDebug() << polylines << "polylines are contained in the old map tile.";

  // Add new polylines to the map tile
  for( int i = 0; i < plList.size(); i++ )
    {
      QPolygon entry = plList[i];

      if( replace == "road" )
        {
          out << (quint8) BaseMapElement::Road;
        }
      else if( replace == "motorway" )
        {
          out << (quint8) BaseMapElement::Motorway;
        }
      else if( replace == "rail" )
        {
          out << (quint8) BaseMapElement::Railway;
        }

      out << (quint32) entry.size();

      for(int i = 0; i < entry.size(); i++)
        {
          out << qint32(entry.point(i).x());
          out << qint32(entry.point(i).y());
        }
    }

  mFile.close();
  oFile.close();
}

/**
 * Reads the polyline data from a file and returns them as list.
 *
 * \param fileName Filename of binary file containing polylines. The polylines
 *                 are stored as QPolygonF objects. The coordinates are stored
 *                 as floating point numbers in the WGS84 format in decimal
 *                 degrees.
 *
 * \param tileNo Map tile number, which data are to replace
 *
 * \param plTileList polyline tile list to be filled
 *
 * \return true on success otherwise false
 */
bool getPolylineTile(QString fileName, int tileNo, QList<QPolygon>& plTileList)
{
  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly))
    {
      qWarning() << "Cannot open polyline file: " << fileName;
      return false;
    }

  QList<QPolygonF> plDict;
  QDataStream in(&file);

  // Read in binary data from the file.
  while( ! in.atEnd() )
    {
      QPolygonF pg;
      in >> pg;
      plDict.append(pg);
    }

  file.close();

  QRectF tileBox = getTileBox( tileNo );

  QString txt = QString("TileBox: Lat=%1...%2, lon=%3...%4")
                .arg(tileBox.x())
                .arg(tileBox.x() + tileBox.width())
                .arg(tileBox.y())
                .arg(tileBox.y() + tileBox.height());

  qDebug( "%s", txt.toAscii().data() );

  for( int i = 0; i < plDict.size(); i++ )
    {
      QPolygonF &pline = plDict[i];
      QPolygon pgTile;
      bool first = true;
      QPoint ip;

      for( int j = 0; j < pline.size(); j++ )
        {
          // Check, if polyline has points inside of the requested map tile
          const QPointF& p = pline.at(j);

          if( tileBox.contains( p ) )
            {
              if( first )
                {
                  first = false;

                  if( j > 0 && p != pline.at(j-1) )
                    {
                      // Add the last outside point to the tile, if it is not
                      // yet contained.
                      const QPointF& pp = pline.at(j-1);

                      // Convert coordinates to KFLog format.
                      ip.setX( (int) rint(pp.x() * 600000) );
                      ip.setY( (int) rint(pp.y() * 600000) );
                      pgTile.append(ip);
                    }
                }

              // Add inside point to the tile
              ip.setX( (int) rint(p.x() * 600000) );
              ip.setY( (int) rint(p.y() * 600000) );
              pgTile.append(ip);
            }
          else
            {
              if( first == false )
                {
                  // There was added some data before. Add the first outside
                  // point also to the tile.
                  first = true;

                  // Add outside point to the tile
                  ip.setX( (int) rint(p.x() * 600000) );
                  ip.setY( (int) rint(p.y() * 600000) );
                  pgTile.append(ip);
                }
            }
        }

      if( pgTile.size() == 0 )
        {
          // No points found in map tile
          continue;
        }

      if( pgTile.size() > 0 && pgTile.size() < 2 )
        {
          // Check to see if something has going wrong!
          qDebug() << "Ignoring polyline, has too less points!" << pgTile.size();
          continue;
        }

      plTileList.append(pgTile);
    }

  return true;
}

/**
 * Calculates the bounding box of the given tile number in KFLog coordinates.
 * The returned rectangle used the x-axis as longitude and the y-axis as latitude.
 */
QRectF getTileBox(const int tileNo)
{
  if( tileNo > (180*90) )
    {
      qWarning("Tile %d is out of range", tileNo);
      return QRectF();
    }

  // Positive result means N, negative result means S
  int lat = 90 - ((tileNo / 180) * 2);

  // Positive result means E, negative result means W
  int lon = ((tileNo % 180) * 2) - 180;

  // Tile bounding rectangle starting at upper left corner with:
  // Y: longitude until longitude + 2 degrees
  // X: latitude  until latitude - 2 degrees
  // QRect rect( lon*600000, lat*600000, 2*600000, -2*600000 );
  QRectF rect( lat, lon, -2., 2. );

 return rect;
}
