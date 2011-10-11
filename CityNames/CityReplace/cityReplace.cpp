/*
 * $Id$
 *
 * cityReplace.cpp
 *
 *  Created on: 06.10.2011
 *      Author: axel
 *
 * This tool replaces the city names of a KFlog map tile. It uses as input
 * a city dictionary created by using a city name tool. If no replace of a
 * city name is possible, the old name is taken.
 */

using namespace std;

#include <cmath>
#include <iostream>
#include <QtGui>
#include <cstdio>
#include <climits>

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

struct City
{
  double lat;
  double lon;
  QString name;
};

struct CityEntry
{
  QString name;
  QPolygon polygon;
  qint8 sort;
};


class BaseMapElement
{
public:
  enum objectType
  {
    NotSelected = NOT_SELECTED,
    IntAirport = INT_AIRPORT, Airport = AIRPORT, MilAirport = MIL_AIRPORT, CivMilAirport = CIVMIL_AIRPORT,
    Airfield = AIRFIELD, ClosedAirfield = CLOSED_AIRFIELD, CivHeliport = CIV_HELIPORT,
    MilHeliport = MIL_HELIPORT, AmbHeliport = AMB_HELIPORT, Gliderfield = GLIDERFIELD, UltraLight = ULTRALIGHT,
    HangGlider = HANGGLIDER, Parachute = PARACHUTE, Balloon = BALLOON, Outlanding = OUTLANDING, Vor = VOR,
    VorDme = VORDME, VorTac = VORTAC, Ndb = NDB, CompPoint = COMPPOINT,
    AirA = AIR_A, AirB = AIR_B, AirC = AIR_C, AirD = AIR_D, AirE = AIR_E, WaveWindow = WAVE_WINDOW,
    AirF = AIR_F, ControlC = CONTROL_C, ControlD = CONTROL_D, Danger = DANGER,
    LowFlight = LOW_FLIGHT, Restricted = RESTRICTED, Prohibited = PROHIBITED, Tmz = TMZ, GliderSector = GLIDER_SECTOR, Obstacle = OBSTACLE,
    LightObstacle = LIGHT_OBSTACLE, ObstacleGroup = OBSTACLE_GROUP, LightObstacleGroup = LIGHT_OBSTACLE_GROUP,
    Spot = SPOT, Isohypse = ISOHYPSE, Glacier = GLACIER, PackIce = PACK_ICE, Border = BORDER, City = CITY,
    Village = VILLAGE, Landmark = LANDMARK, Highway = HIGHWAY, Road = ROAD, Railway = RAILWAY,
    AerialRailway = AERIAL_CABLE, Lake = LAKE, River = RIVER, Canal = CANAL, Flight = FLIGHT, Task = FLIGHT_TASK,
    Trail = TRAIL, Railway_D = RAILWAY_D, Aerial_Cable = AERIAL_CABLE, River_T = RIVER_T, Lake_T = LAKE_T,
    Forest = FOREST, Turnpoint = TURNPOINT, Thermal = THERMAL,
    FlightGroup = FLIGHT_GROUP, FAIAreaLow500 = FAI_AREA_LOW, FAIAreaHigh500 = FAI_AREA_HIGH,
    EmptyPoint = EMPTY_POINT,  // new type for nothing to draw
    objectTypeSize /* leave this at the end */
  };

  /**
   * The five types of elevation-data used in the maps.
   */
  enum elevationType {NotSet, MSL, GND, FL, STD, UNLTD};
};

// ===========================================================================
// Usage of program
// ===========================================================================

void usage( char * progName )
{
  cerr << "KFlog City name replacer V1.0, created by Axel Pauli (axel@kflog.org)" << endl
       << "Usage: " << basename(progName)
       << " -d <input City-dict-file> -m <input KFLog-map-file> -o <output kflog-map-file> -v verbose"
       << endl << endl;

  exit(-1);
}

int main(int argc, char *argv[])
{
  if( argc < 7 )
    {
      cerr << "Missing mandatory arguments" << endl << endl;
      usage(argv[0]);
    }

  QString dFileName;
  QString mFileName;
  QString oFileName;

  bool verbose = false;

  int c;

  while( (c = getopt(argc, argv, "d:m:o:v")) != -1 )
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
        case 'v':
          verbose = true;
          break;
        case '?':
            {
              cerr << "Unrecognized option: -" << optopt << "!" << endl;
              return -1;
            }
        }
    }

  QFile dFile(dFileName);

  if (! dFile.open(QIODevice::ReadOnly) )
    {
      cerr << "Cannot open input file: " << dFileName.toLatin1().data() << endl;
      return -1;
    }

  QFile mFile(mFileName);

  if (! mFile.open(QIODevice::ReadOnly ) )
    {
      dFile.close();
      cerr << "Cannot open input file: " << dFileName.toLatin1().data() << endl;
      return -1;
    }

  QFile oFile(oFileName);

  if (! oFile.open(QIODevice::WriteOnly ) )
    {
      dFile.close();
      mFile.close();
      cerr << "Cannot open output file: " << oFileName.toLatin1().data() << endl;
      return -1;
    }

  QDataStream ind(&dFile);
  QDataStream inm(&mFile);
  QDataStream out(&oFile);

  QList<City> cityDictList;

  while (! ind.atEnd() )
    {
      struct City city;

      ind >> city.lat;
      ind >> city.lon;
      ind >> city.name;

      cityDictList.append(city);
    }

  dFile.close();

  if( verbose )
    {
      qDebug() << cityDictList.size() << "entries read from City dictionary";
    }

    qint8 loadTypeID;
    quint16 loadSecID, formatID;
    quint32 magic;
    QDateTime createDateTime;

    inm.setVersion( QDataStream::Qt_2_0 );
    out.setVersion( QDataStream::Qt_2_0 );
    // inm.setVersion( QDataStream::Qt_4_7 );

    inm >> magic;
    out << magic;

    if ( magic != KFLOG_FILE_MAGIC )
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
        qWarning("Wrong load type identifier %x read! Aborting ...",
                 loadTypeID );
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

    if ( formatID < FILE_VERSION_MAP)
      {
        // to old ...
        qWarning("Map File format too old! (version %d, expecting: %d) "
                 "Aborting ...", formatID, FILE_VERSION_MAP );
        return -1;
      }
    else if (formatID > FILE_VERSION_MAP)
      {
        // to new ...
        qWarning("Map File format too new! (version %d, expecting: %d) "
                 "Aborting ...", formatID, FILE_VERSION_MAP );
        return -1;
      }

    quint8 lm_typ;
    qint8 sort, elev;
    qint32 lat_temp, lon_temp;
    quint32 locLength = 0;
    QString name;

    QList<CityEntry> cityList;

    while ( ! inm.atEnd() )
      {
        BaseMapElement::objectType typeIn = BaseMapElement::NotSelected;
        inm >> (quint8&)typeIn;

        locLength = 0;
        name = "";

        QPolygon all;
        QPoint single;

        switch (typeIn)
          {
          case BaseMapElement::Highway:
            out << quint8(typeIn);
            RW_POINT_LIST
            break;

          case BaseMapElement::Road:
          case BaseMapElement::Trail:
            out << quint8(typeIn);
            RW_POINT_LIST
            break;

          case BaseMapElement::Aerial_Cable:
          case BaseMapElement::Railway:
          case BaseMapElement::Railway_D:
            out << quint8(typeIn);
            RW_POINT_LIST
            break;

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
              inm >> sort;

              if (formatID >= FILE_FORMAT_ID)
                {
                   inm >> name;
                }

              READ_POINT_LIST

              struct CityEntry entry;

              entry.name = name;
              entry.polygon = all;
              entry.sort = sort;

              cityList.append( entry );
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

    if( verbose )
      {
        qDebug() << "City list contains" << cityList.size() << "elements";
      }

    for( int i = 0; i < cityList.size(); i++ )
      {
        struct CityEntry entry = cityList[i];
        int lastDist = INT_MAX;
        QString newName;

        // An offset can be added to the bounding rectangle to get a better
        // match. Sometime the city polygon lays outside of the city coordinates.
        int delta = 1; // nautical miles

        // calculate degrees
        int dx = abs((int) rint( (delta / (cos(double(entry.polygon.boundingRect().x()) / 600000. * M_PI / 180))) / 60. * 600000. ));
        int dy = (int) rint(delta / 60. * 600000.);

        QRect pgRect( entry.polygon.boundingRect().x() - dx,
                      entry.polygon.boundingRect().y() - dy,
                      entry.polygon.boundingRect().width() + 2*dx,
                      entry.polygon.boundingRect().height() + 2*dy );

        QPoint polyCenterRect( pgRect.x() + pgRect.width() / 2,
                               pgRect.y() + pgRect.height() / 2 );

//        qDebug() << "Replace:" << entry.name
//                 << "PRect=" << entry.polygon.boundingRect()
//                 << "PgRect" << pgRect
//                 << "PRectCenter=" << polyCenterRect
//                 << double(polyCenterRect.x()) / 600000.0
//                 << double(polyCenterRect.y()) / 600000.0;

        for( int j = 0; j < cityDictList.size(); j++ )
          {
            struct City city = cityDictList[j];

            int lat = (int) rint(city.lat * 600000.0);
            int lon = (int) rint(city.lon * 600000.0);
            QString cname = city.name;
            QPoint pos(lat, lon);

            // The nearest match to the center of the polygon is taken.
            if( pgRect.contains( pos ) )
              {
                int dist = (pos - polyCenterRect).manhattanLength();

                if( dist < lastDist )
                  {
                    newName = cname;
                    lastDist = dist;
                    //qDebug() << "BoundingRect Old:" << entry.name << "New:" << cname << "Dist:" << dist;
                  }
              }
          }

        if( newName.isEmpty() )
          {
            qDebug() << "SecId=" << loadSecID << "Using old name:" << entry.name;
            newName = entry.name;

            // city name
            newName = newName.toLower();

            QChar lastChar(' ');

            // convert city names to upper-lower
            for( int i=0; i < newName.length(); i++ )
              {
                if( lastChar == ' ' )
                  {
                    newName.replace( i, 1, newName.mid(i,1).toUpper() );
                  }

                lastChar = newName[i];
              }
          }

        if( verbose )
          {
            qDebug() << "Assigned: Old=" << entry.name << "New=" << newName << "Dist:" << lastDist;
          }

        out << (quint8) BaseMapElement::City;
        out << qint8(0);  // sort
        out << newName;
        out << (quint32) entry.polygon.size();

        for(int i = 0; i < entry.polygon.size(); i++)
          {
            out << qint32(entry.polygon.point(i).x());
            out << qint32(entry.polygon.point(i).y());
          }
     }

    mFile.close();
    oFile.close();
    return 0;
}
