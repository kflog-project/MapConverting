/*********************************************************************
 *
 * vpf_importer.cpp
 *
 * (c) Heiner Lamprecht, Florian Ehinger June 2002
 *     Based on VPF Lib from ....
 *
 * Importiert VPF-Datenbanken und erzeugt ASCII-Kartendateien.
 *
 * vpf_importer <DataBase> <Library>
 *
 *
 * Struktur:
 *
 *   DataBase (nur eine)
 *      '->   Library (mehrere)
 *               '->    Coverage (mehrere)
 *                          '->     Feature (optional, mehrere)
 *
 ********************************************************************/

#include <iostream>
#include <fstream>
#include <stdio.h>

// #include <qstring.h>

#include <vpf.hxx>

/**
 * Get a printable name for a value type.
 */
static const char *
get_value_type_name (VpfValue::Type type)
{
  switch (type) {
  case VpfValue::EMPTY:
    return "empty";
  case VpfValue::TEXT:
    return "text";
  case VpfValue::INT:
    return "integer";
  case VpfValue::REAL:
    return "real number";
  case VpfValue::POINTS:
    return "coordinate array";
  case VpfValue::DATE:
    return "date and time";
  case VpfValue::CROSSREF:
    return "cross-tile reference";
  default:
    throw VpfException("Unknown value type");
  }
}

// Schreibt einen Punkt auf stdout
static void dump_point(const VpfPoint &p, ofstream &stream)
{
  stream << (int)( p.y * 600000 ) << ' ' << (int)( p.x * 600000 ) << endl;
}


static void dump_line(const VpfLine &l, ofstream &stream)
{
  int nPoints = l.getPointCount();
  for (int i = 0; i < nPoints; i++)
    dump_point(l.getPoint(i), stream);
}


static void dump_contour(const VpfContour &c, ofstream &stream)
{
  int nPoints = c.getPointCount();

  for (int i = 0; i < nPoints; i++)
    {
      dump_point(c.getPoint(i), stream);
    }
}


static void dump_polygon(const VpfPolygon &poly, ofstream &stream, VpfTable &table,
                          int rowLoop)
{
  int nContours = poly.getContourCount();

  for (int i = 0; i < nContours; i++)
    {
      if(i > 0)
        {
          stream << "[END]\n";
          stream << "[NEW]\n";

          // Header ausgeben
          for(int colLoop = 0; colLoop < table.getColumnCount(); colLoop++)
            {
              stream << table.getColumnDecl(colLoop).getName() << "="
                      << table.getValue(rowLoop, colLoop) << endl;
            }
        }

      dump_contour(poly.getContour(i), stream);
    }
}


static void dump_label(const VpfLabel &label, ofstream &stream)
{
  const VpfPoint p = label.getPoint();
  stream << p.x << ' ' << p.y << ' ' << label.getText() << endl;
}


void dump_table_header(const VpfTable &table, ofstream &outFile)
{

  // Table-Header ausgeben ...
  outFile << "#\n";
  outFile << "# Table-Description:\n";
  outFile << "#\n";
  outFile << "#    Path: " << table.getPath() << endl;
  outFile << "#    Description: \"" << table.getDescription() << '"' << endl;
  outFile << "#    Documentation file: " << table.getDocFileName() << endl;
  outFile << "#    Total columns: " << table.getColumnCount() << endl;
  outFile << "#    Total rows: " << table.getRowCount() << endl;
  outFile << "#\n";

  // Spalten-Beschreibung ausgeben ...
  outFile << "# Column-Description:\n";
  outFile << "#\n";
  for (int i = 0; i < table.getColumnCount(); i++)
    {
      const VpfColumnDecl &column = table.getColumnDecl(i);
      outFile << "#  Column " << i << " Declaration: " << endl;
      outFile << "#    Name: " << column.getName() << endl;
      outFile << "#    Type: " << get_value_type_name(column.getValueType()) << endl;
      outFile << "#    Element count: " << column.getElementCount() << endl;
      outFile << "#    Key type: " << column.getKeyType() << endl;
      outFile << "#    Description: " << column.getDescription() << endl;
      outFile << "#    Value description table: "
              << column.getValueDescriptionTable() << endl;
      outFile << "#    Thematic index name: "
              << column.getThematicIndexName() << endl;
      outFile << "#    Narrative table: "
              << column.getNarrativeTable () << endl;
    }
  outFile << "#\n";
}

int main(int argc, char** argv)
{
  cerr << "Starte mit einlesen\n";
  if(argc <= 3)
    {
      cerr << "Usage: vpf_importer <DataBase> <Library> [<Coverage(id)>] [<Feature(id)>]\n";
      return 1;
    }

  // Zielverzeichnis der Kartendateien ...
  string outDir((string)"/data/KartenDaten/VPF-Daten/heiner/" + argv[2] + "/");

  VpfRectangle bounds;
  bounds.minX = -180.0;
  bounds.maxX = 180.0;

  bounds.minY = -90.0;
  bounds.maxY = 90.0;

  VpfDataBase db( argv[1] );

  cout << "Number of libraries: " << db.getLibraryCount() << endl;

  VpfLibrary lib( db.getLibrary(argv[2]) );

  cout << "Number of coverages: " << lib.getCoverageCount() << endl;

  int coverCount = lib.getCoverageCount();

  cout << "<->\n";

  int cStart = 0, fStart = 0;
  int cEnd, fEnd;

  if(argc > 3)
    {
      sscanf(argv[3], "%d", &cStart);
      cEnd = cStart + 1;
    }
  if(argc > 4)
    {
      sscanf(argv[4], "%d", &fStart);
      fEnd = fStart + 1;
    }

  if(cEnd == 0 || cEnd > coverCount)
    {
      cEnd = coverCount;
    }

  for(int cLoop = cStart; cLoop < cEnd; cLoop++)
    {
      VpfCoverage cover(lib.getCoverage(cLoop));
      int featureCount = cover.getFeatureCount();

      cout << "Coverage: " << cover.getName() << " (" << cLoop << ") Features: "
           << cover.getFeatureCount() << endl;

      if(fEnd == 0 || fEnd > cover.getFeatureCount())
        {
          fEnd = cover.getFeatureCount();
        }

      for(int fLoop = fStart; fLoop < fEnd; fLoop++)
        {
          VpfFeature feature(cover.getFeature(fLoop));
          string fileName((string)argv[1] + "/" + (string)argv[2] + "/" +
              (string)cover.getName() + "/" + (string)feature.getName());

          // Ausgabe-Datei öffnen:
          string outFileName((string)cover.getName() + "_" + (string)feature.getName());
          string idFileName = outDir + "/IDS/" + outFileName + ".id";
          outFileName = outDir + outFileName;

          ofstream outFile;
          ifstream idFile;

          cout << "\n";
          cout << "  Öffne Ausgabedatei: " << outFileName << " ...";

          idFile.open(idFileName.c_str(), ios::in);
          if(!idFile)
            {
              cerr << "FEHLER! bei ID File\n" << idFileName.c_str() << endl;
              continue;
            }

          outFile.open(outFileName.c_str(), ios::out | ios::app );
          if(!outFile)
            {
              cerr << "FEHLER!\n";
              return 1;
            }
          else
            {
              cout << "fertig\n";
            }

          cout << endl;
          cout << "  Bearbeite " << cover.getName() << " (" << cLoop << ") "
               << feature.getName() << " (" << fLoop << ")\n";

          switch(feature.getTopologyType())
            {
              case VpfFeature::POINT:
                fileName += ".pft";
                break;
              case VpfFeature::LINE:
                fileName += ".lft";
                break;
              case VpfFeature::POLYGON:
                fileName += ".aft";
                break;
              case VpfFeature::LABEL:
                fileName += ".tft";
                break;
              default:
                throw VpfException("Unsupported topology type");
            }

          cout << "Lade VpfTable...\n";
          VpfTable table(fileName);
          cout << "           ... fertig!\n";

          dump_table_header(table, outFile);

          char p[20];
          int id(-1), oldID(-1), rowLoop;

          int rowCount = table.getRowCount();
          int colCount = table.getColumnCount();
          int topologyType = feature.getTopologyType();


          while(!idFile.eof())
            {
              idFile.getline(p, 20);
              sscanf(p, "%d", &id);

              if(id == oldID) continue;

              oldID = id;

              rowLoop = id - 1;

/*
            }

          for(int rowLoop = 0; rowLoop < table.getRowCount(); rowLoop++)
            {
*/
              outFile << "[NEW]\n";

              std::string header = "";
              for(int colLoop = 0; colLoop < colCount; colLoop++)
                {
                  outFile << table.getColumnDecl(colLoop).getName() << "="
                          << table.getValue(rowLoop, colLoop) << endl;

                }
              try
                {
                  cout << "\r";
                  cout << "id: ";
                  cout.width(10);
                  cout << id << " von ";
                  cout.width(10);
                  cout << rowCount << " ( ";
                  cout.precision(1);
                  cout.width(4);
                  cout.setf(ios::fixed);
                  cout << (float)((float)id / (float)rowCount * 100) << "%)";


                  switch(topologyType)
                    {
                      case VpfFeature::POINT:
                        dump_point(feature.getPoint(rowLoop), outFile);
                        break;
                      case VpfFeature::LINE:
                        dump_line(feature.getLine(rowLoop), outFile);
                        break;
                      case VpfFeature::POLYGON:
                        dump_polygon(feature.getPolygon(rowLoop), outFile, table, rowLoop);
                        break;
                      case VpfFeature::LABEL:
                        dump_label(feature.getLabel(rowLoop), outFile);
                        break;
                      default:
                        throw VpfException("Unsupported topology type");
                    }
                }
              catch(VpfException &e)
                {
                  cerr << "Fehler: " << e.getMessage() << endl;
                }

              outFile << "[END]\n";

            }

          outFile.close();
        }
    }

    cout << endl;
  return 0;

}

