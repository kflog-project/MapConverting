/*
 * $Id$
 *
 *  Created on: 14.11.2011
 *      Author: axel
 *
 * This file is distributed under the terms of the General Public License.
 *
 * This tool reads a polyline definition file and writes a binary output file,
 * containing all read polylines as QPolygonF objects. The coordinate pairs
 * (latitude/longitude) are encoded as decimal degrees in WGS84 format.
 * Via options additional possibilities can be activated.
 *
 */

using namespace std;

#include <cmath>
#include <iostream>

#include <QtGui>

#include "resource.h"


bool readPolylineFile(QString fileName, QList<QPolygonF>& mwList);

void searchConnections( QList<QPolygonF>& mwList, const bool verbose );

QList<QPolygonF> reducePoints( const double range, QList<QPolygonF>& mwList, const bool verbose );

double getBearingWgs( const QPointF& p1, const QPointF& p2 );

double distP(double lat1, double lon1, double lat2, double lon2);

// ===========================================================================
// Usage of program
// ===========================================================================

void usage(char * progName)
{
  cerr
      << "KFlog Polyline dictionary creator V1.0, Axel Pauli (axel@kflog.org)"
      << endl
      << "Usage: "
      << basename(progName) << endl
      << "  -i <input> (polyline definition file name)" << endl
      << "  -o <output> (polyline binary file name)" << endl
      << " [-c <meters>] (movement corridor in meters)" << endl
      << " [-s] (search polyline connections)" << endl
      << " [-v] (verbose)" << endl
      << endl << endl;

  exit(-1);
}

int main(int argc, char *argv[])
{
  if (argc < 5)
    {
      cerr << "Missing mandatory arguments!" << endl << endl;
      usage(argv[0]);
    }

  QString corridor;
  QString iFileName;
  QString oFileName;
  bool verbose = false;
  bool search = false;

  int c;

  while ((c = getopt(argc, argv, "i:o:c:sv")) != -1)
    {
      switch (c)
        {
        case 'c':
          corridor = optarg;
          break;
        case 'i':
          iFileName = optarg;
          break;
        case 'o':
          oFileName = optarg;
          break;
        case 's':
          search = true;
          break;
        case 'v':
          verbose = true;
          break;
        case '?':
          {
            cerr << "Unrecognized option: -" << optopt << "!" << endl;
            usage(argv[0]);
            break;
          }
        }
    }

  if( iFileName.isEmpty() || oFileName.isEmpty() )
    {
      cerr << "Missing mandatory arguments!" << endl << endl;
      usage(argv[0]);
    }

  QList<QPolygonF> mwList;

  bool ok = readPolylineFile(iFileName, mwList);

  if( ! ok )
    {
      return -1;
    }

  qDebug() << mwList.size() << "polyline(s) read from WGS84 polyline file";

  if( search )
    {
      searchConnections( mwList, verbose );
    }

  if( ! corridor.isEmpty() )
    {
      // Corridor filter shall be applied. Input is in meters.
      double range = corridor.toDouble(&ok);

      if( ok )
        {
          mwList = reducePoints( range, mwList, verbose );
        }
    }

  QFile oFile(oFileName);

  if ( !oFile.open(QIODevice::WriteOnly) )
    {
      qWarning() << "Cannot open output file: " << oFileName;
      return -1;
    }

  // Save all polygons into a file.
  QDataStream out(&oFile);

  for( int i = 0; i < mwList.size(); i++ )
    {
      out << mwList[i];
    }

  oFile.close();
  return 0;
}

/**
 * Reads the polyline text file and creates a list of polylines.
 *
 * \param fileName File name of input data file
 *
 * \param mwList polyline list to be filled
 *
 * \return true on success otherwise false
 */
bool readPolylineFile(QString fileName, QList<QPolygonF>& mwList)
{
  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly))
    {
      qWarning() << "Cannot open input file: " << fileName;
      return false;
    }

  QTextStream in(&file);

  /*
    L  42
     6.02994332   51.86150315
     6.03443202   51.86138736
  */

  bool suppress = true;
  int lineNo = 0;

  while (!in.atEnd())
    {
      QString line = in.readLine().trimmed();
      lineNo++;

      if( line.isEmpty() )
        {
          continue;
        }

      if( ! line.startsWith("L  ") )
        {
          if( ! suppress )
            {
              QString txt = QString("Error line %1: Expecting a 'L' here: %2").arg(lineNo).arg(line);
              qDebug() << txt;
            }

          continue;
        }

      suppress = false;

      QStringList list = line.split( " ", QString::SkipEmptyParts );

      if( list.size() != 2 )
        {
          QString txt = QString("Error line %1: Expecting length info here: %2").arg(lineNo).arg(line);
          qDebug() << txt;
          continue;
        }

      bool ok;

      // extract polygon element member count
      int members = list[1].toInt( &ok );

      if( ! ok )
        {
          continue;
        }

      QPolygonF pg;

      // read all polygon members
      for( int i = 0; i < members && in.atEnd() == false; i++ )
        {
          line = in.readLine().trimmed();
          lineNo++;

          QStringList ll = line.split( " ", QString::SkipEmptyParts );

          if( ll.size() != 2 )
            {
              // To be sure, do a check and report errors.
              QString txt = QString("Error line %1: Expecting 2 coordinates here: %2").arg(lineNo).arg(line);
              qDebug() << txt;
              continue;
            }

          QPointF p;

          bool ok1, ok2;
          p.setX( ll[1].toFloat( &ok1 ));
          p.setY( ll[0].toFloat( &ok2 ));

          if( !ok1 || ! ok2 )
            {
              QString txt = QString("Error line %1: Expecting 2 floating point numbers here: %2").arg(lineNo).arg(line);
              qDebug() << txt;
              continue;
            }

          pg.append(p);
        }

      //qDebug() << "Add polygon:" << pg.size() << "Members:" << members;
      mwList.append( pg );
    }

  file.close();
  return true;
}

/**
 * Try to find missing connections at the beginning and at the end of each polyline.
 */
void searchConnections( QList<QPolygonF>& mwList, const bool verbose )
{
  // Minimum distance in meters to another point in another polyline to make
  // a connection to it.
  const double MinDistance = 1500.0;

  // Minimum distance in meters between first and last point of a polyline,
  // to be considered in connection search.
  const double LineDistance = 3000.0;

  int ignored = 0;

  for( int i = 0; i < mwList.size(); i++ )
    {
      QPolygonF& pline = mwList[i];

      if( pline.size() < 2 )
        {
          continue;
        }

      QPointF& pf = pline.first();
      QPointF& pl = pline.last();

      //qDebug() << "Start" << pf << "End" << pl;

      double meters = distP( pf.x(), pf.y(), pl.x(), pl.y() );

      if( meters < LineDistance )
        {
          // ignore to short segments.
          // qDebug() << "Ignoring segment size:" << pline.size() << "Dist=" << meters << "m";
          ignored++;
          continue;
        }

      double distf = 9999999.;
      double distl = 9999999.;

      QPointF plof, plol;

      // Try to find a connection point in another polyline.
      for( int k = 0; k < mwList.size(); k++ )
        {
          if( i == k )
            {
              // do not use the polyline to be checked
              continue;
            }

          QPolygonF &plo = mwList[k];

          for( int l = 0; l < plo.size(); l++ )
            {
              const QPointF& po = plo.at(l);

              double df = 0.0;
              double dl = 0.0;

              if( distf > 0.0 )
                {
                  // calculate distance to first check point
                  df = distP( pf.x(), pf.y(), po.x(), po.y() );
                }

              if( distl > 0.0 )
                {
                  // calculate distance to last check point
                  dl = distP( pl.x(), pl.y(), po.x(), po.y() );
                }

              if( df < distf )
                {
                  distf = df;
                  plof = po;
                }

              if( dl < distl )
                {
                  distl = dl;
                  plol = po;
                }
            }

          // Break because both distances are zero
          if( distf == 0.0 && distl == 0 )
            {
              break;
            }
        }

      // Check, if we found points in the near
      if( distf > 0.0 && distf <= MinDistance )
        {
          // Add the near point as new first point
          pline.insert(0, plof);

          if( verbose )
            {
              qDebug() << "i=" << i << "New first=" << distf << "m" << "LineLength=" << meters << "m";
            }
        }

      if( distl > 0.0 && distl <= MinDistance )
        {
          // Add the near point as new last point
          pline.append(plol);

          if( verbose )
            {
              qDebug() << "i=" << i << "New last=" << distl << "m" << "LineLength=" << meters << "m";
            }
        }
    }

  if( verbose )
    {
      qDebug() << ignored << "segments ignored!";
    }
}

/**
 * Try to reduce the point size of a polyline.
 *
 * \param range Corridor beside the course line in meters.
 *
 * \param mwList list of polylines to be processed.
 *
 * \return list with reduced polylines.
 */
QList<QPolygonF> reducePoints( const double range,
                               QList<QPolygonF>& mwList,
                               const bool verbose )
{
  QPointF startP;
  QPointF lastP;
  double startBearing;
  QList<QPolygonF> retList;
  QList<QPointF> ignoreList;

  int sum = 0;

  for( int i = 0; i < mwList.size(); i++ )
    {
      const QPolygonF& plIn = mwList.at(i);
      QPolygonF plOut;

      for( int j = 0; j < plIn.size(); j++ )
        {
          const QPointF& p = plIn.at(j);

          if( j == 0 )
            {
              // Set start point for filtering
              startP = p;
              plOut.append(p);
              continue;
            }

          if( j == 1 )
            {
              startBearing = getBearingWgs( startP, p );
              lastP = p;
              continue;
            }

          // calculate bearing from start point
          double bearing = getBearingWgs( startP, p );

          // calculate distance from start point
          double dist = distP(startP.x(), startP.y(), p.x(), p.y());

          // calculate sector angle from start point.
          const double delta = atan( range / dist );

          if( fabs( startBearing - bearing ) < delta )
            {
              // Ignore point, it is laying inside the corridor.
              ignoreList.append(p);
              lastP = p;
              continue;
            }

#if 0
          printf( "j=%d, Start=%.1f, Entf=%.0f, Kurs=%.1f, Winkel=%.1f, Diff=%.1f°\n",
                   j, startBearing, dist, bearing, delta,
                   fabs( startBearing - bearing) * 180.0 / M_PI );
#endif
          // Add points from the ignore list to get a better turn
          int start = ignoreList.size() - 2;

          if( start < 0 )
            {
              start = 0;
            }

          if( start < ignoreList.size() )
            {
              // qDebug() << "IgnoreList" << ignoreList.size() << "is used";
            }

          for( int k=start; k < ignoreList.size(); k++ )
            {
              // plOut.append( ignoreList.at(k) );
            }

          ignoreList.clear();

          startBearing = bearing;
          plOut.append(lastP);
          startP = lastP;
          lastP = p;
        }

      if( plOut.size() > 0 && lastP != plOut.last() )
        {
          // Add last start point to the segment list.
          plOut.append(lastP);
        }

      if( plOut.size() < 2 )
        {
          qWarning() << "Polyline" << i << "has only" << plOut.size() << "points!";
          continue;
        }

      if( plIn.size() - plOut.size() )
        {
          if( verbose )
            {
              qDebug() << "Points=" << plIn.size()
                       << "Taken=" << plOut.size()
                       << "Reduced=" << (plIn.size() - plOut.size());
            }

          sum += plIn.size() - plOut.size();
        }

      retList.append( plOut );
    }

  qDebug() << "Corridor=" << range << "m," << sum << "points are removed.";

  return retList;
}

/**
   Calculate the bearing from point p1 to point p2 from WGS84
   coordinates to avoid distortions caused by projection to the map.
*/
double getBearingWgs( const QPointF& p1, const QPointF& p2 )
{
  // Bogenmass = Grad * PI / 180
  const double pi_180 = M_PI / 180.0;

  double dx = p2.x() - p1.x(); // latitude
  double dy = p2.y() - p1.y(); // longitude

  // compute latitude distance in meters
  double latDist = dx * MILE_kfl / 10000.; // b

  // compute latitude average
  double latAv = ( ( p2.x() + p1.x() ) / 2.0);

  // compute longitude distance in meters
  double lonDist = dy * cos( pi_180 * latAv ) * MILE_kfl / 10000.; // a

  // compute angle
  double angle = asin( fabs(lonDist) / hypot( latDist, lonDist ) );

  // double angleOri = angle;

  // assign computed angle to the right quadrant
  if (dx >= 0 && dy < 0)
    {
      angle = (2 * M_PI) - angle;
    }
  else if (dx <= 0 && dy <= 0)
    {
      angle = M_PI + angle;
    }
  else if (dx < 0 && dy >= 0)
    {
      angle = M_PI - angle;
    }

  return angle;
}

/**
 * Calculates the distance between two given points in meters according to Pythagoras.
 * For longer distances the result is often greater as calculated by great circle.
 * Not more used in Cumulus.
 */
double distP(double lat1, double lon1, double lat2, double lon2)
{
  static const double pi_180 = M_PI / 180.0;

  double dlat = lat1 - lat2;
  double dlon = lon1 - lon2;

  // lat is used to calculate the earth-radius. We use the average here.
  // Otherwise, the result would depend on the order of the parameters.
  double lat = ( lat1 + lat2 ) / 2.0;

  // Distance calculation according to Pythagoras
  double dist = RADIUS * hypot (pi_180 * dlat, pi_180 * cos( pi_180 * lat ) * dlon);

  return dist;
}
