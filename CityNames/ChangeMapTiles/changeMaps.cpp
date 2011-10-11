/*
 * $Id$
 *
 * changeMaps.cpp
 *
 *  Created on: 10.10.2011
 *      Author: axel
 *
 * This tool changes the city names of a map area. The input parameters are
 * hard coded and must be defined after the beginning of main.
 */

#include <cmath>
#include <unistd.h>
#include <cstdlib>
#include <QtGui>

/**
 * Calculates the map tile number from the passed coordinate. The coordinate
 * format is decimal degree. Positive numbers are N and E, negative numbers
 * are W and S.
 *
 * @param lat Latitude in decimal degree. 90...-90
 * @param lon Longitude in decimal degree. -180...180
 * @return map tile number 0...16199
 */
int mapTileNumber( double lat, double lon )
{
  // check and correct input ranges
  if( lat <= -90 ) lat = -88;
  if( lon >= 180 ) lon = 178;

  int latTile = (90 - (int) ceil(lat) + ((int) ceil(lat) % 2)) * 180 / 2;

  int lonTile = ((int) ceil(lon) + ((int) ceil(lon) % 2) + 180) / 2;

  int tile = lonTile + latTile;

  return tile;
}

int main(int argc, char *argv[])
{
  QString mapDir = "/media/sf_VBoxShare/Karten1/";
  QString backup = "/home/axel/MapData/backup/";
  QString newMap = "/home/axel/MapData/new/";
  QString cityDict = "allCountries2.dic";
  QString cityReplaceTool = "./cityReplace";

  if( access( cityReplaceTool.toLatin1().data(), X_OK ) == -1 )
    {
      qWarning() << "Cannot access City replace tool:" << cityReplaceTool;
      return -1;
    }

  // Europa: 72N-34N, 24W-32E
  // Südafrika: 36S-16S, 10E-36E
  // Nordamerika: 72N-14N, 168W-52W
  for( int i = 70; i >= 30; i -= 2 ) // Latitude
    {
      for( int j=-20; j <= 36; j += 2 ) // longitude
        {
          // generate map file name. m_<tileNo> e.g. M_04894.kfl
          int tile = mapTileNumber( i, j );

          QString fn( QString("M_%1.kfl").arg(tile, 5, 10, QChar('0')));

          // qDebug() << "Lat=" << i << "Lon=" << j << "FileName=" << fn;

          QString oldFn = mapDir + fn;
          QString backupFn = backup + fn;
          QString newFn = newMap + fn;

          // Backup map file
          QFile::remove(backupFn);
          bool ok = QFile::copy( oldFn, backupFn );

          if( ! ok )
            {
              qWarning() << "Cannot copy File" << oldFn << "to" << backupFn;
              continue;
            }

          QString cmd = cityReplaceTool + " -d " + cityDict + " -m " +
                        oldFn + " -o " + newFn;

          system( cmd.toAscii().data() );
       }
    }

  return 0;
}

