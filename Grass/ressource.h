/*
 * Definiert Macros zur Festlegung des Datei-Headers von KFLog-Karten
 *
 * Name der Ausgabedatei:
 *
 *   X_iiii.kfl
 *
 *
 *   X:    Typ der enthaltenen Daten
 *
 *         "G" für 0-Meter-Linie ("Ground")
 *         "T" für restliche Höhenlinien ("Terrain")
 *         "M" für allgemeine Karteninformationen
 *
 *   iiii: Kennzahl der Kachel, Angabe vierstellig
 *         (Zahl liegt zwischen 0 und 8190)
 *
 *   Typkennung und Kachel-ID werden ebenfalls im Header der Datei angegeben.
 *
 *
 * Anfang der Datei:
 *
 *   Byte     Typ       Inhalt         Bedeutung
 *  -----------------------------------------------------------------
 *   0        belong    0x404b464c     KFLog-mapfile       ("@KFL")
 *   >4       byte      0x47           grounddata          ("G")
 *                      0x54           terraindata         ("T")
 *                      0x4d           mapdata             ("M")
 *                      0x41           aeronautical data   ("A")
 *   >5       Q_UINT16  <>             Version des Dateiformates
 *   >7       Q_UINT16  <>             ID der Kachel
 *   >9       8byte     <>             Zeitstempel der Erzeugung
 */

#define KFLOG_FILE_MAGIC   0x404b464c

#define TYPE_GROUND     0x47
#define TYPE_TERRAIN    0x54
#define TYPE_MAP        0x4d
#define TYPE_AERO       0x41
#define FORMAT_VERSION  100

/************************************************************************************
 *
 * Festlegung der Typ-Kennungen der Kartenelemente (ACHTUNG: Reihenfolge geändert!).
 *
 * Typ                 alt     neu
 * ---------------------------------
 * NotSelected        |  0   |   0
 *
 * IntAirport         |  1   |   1
 * Airport            |  2   |   2
 * MilAirport         |  3   |   3
 * CivMilAirport      |  4   |   4
 * Airfield           |  5   |   5
 * ClosedAirfield     |  6   |   6
 * CivHeliport        |  7   |   7
 * MilHeliport        |  8   |   8
 * AmbHeliport        |  9   |   9
 * Glidersite         | 10   |  10
 * UltraLight         | 11   |  11
 * HangGlider         | 12   |  12
 * Parachute          | 13   |  13
 * Ballon             | 14   |  14
 * Outlanding         | 15   |  15
 *
 * VOR                | 16   |  16
 * VORDME             | 17   |  17
 * VORTAC             | 18   |  18
 * NDB                | 19   |  19
 * CompPoint          | 35   |  20
 *
 * AirA               | --   |  21
 * AirB               | --   |  22
 * AirC               | 20   |  23
 * AirCtemp           | 21   |  --   ("temp" als Option bei allen Lufträumen)
 * AirD               | 22   |  24
 * AirDtemp           | 23   |  --
 * AirElow            | 25   |  25
 * AirEhigh           | 26   |  26
 * AirF               | 27   |  27
 * ControlC           | --   |  28
 * ControlD           | 24   |  29
 * Danger             | 29   |  30
 * LowFlight          | 30   |  31
 * Restricted         | 28   |  32
 * TMZ                | --   |  33
 *
 * Obstacle           | 31   |  34
 * LightObstacle      | 32   |  35
 * ObstacleGroup      | 33   |  36
 * LightObstacleGroup | 34   |  37
 * Spot               | 65   |  38
 *
 * Isohypse           | 70   |  39
 * Glacier            | 67   |  40
 *
 * Border             | --   |  41
 *
 * HugeCity           | 36   |  42   (nur eine Stadtgrösse)
 * BigCity            | 37   |  --
 * MidCity            | 38   |  --
 * SmallCity          | 39   |  --
 * Village            | 40   |  43
 *
 * Oiltank            | 41   |  44   (nur noch "Landmarke")
 * Factory            | 42   |  --
 * Castle             | 43   |  --
 * Church             | 44   |  --
 * Tower              | 45   |  --
 *
 * Highway            | 46   |  45
 * HighwayEntry       | 47   |  --   (LM)
 * MidRoad            | 48   |  46   (nur eine Strassengrösse)
 * SmallRoad          | 49   |  --
 * RoadBridge         | 50   |  --   (LM)
 * RoadTunnel         | 51   |  --   (LM)
 *
 * Railway            | 52   |  47
 * RailwayBridge      | 53   |  --   (LM)
 * Station            | 54   |  --   (LM)
 * AerialRailway      | 55   |  48
 *
 * Coast              | 56   |  --
 * BigLake            | 57   |  49   (nur eine See-Größe)
 * MidLake            | 58   |  --
 * SmallLake          | 59   |  --
 * BigRiver           | 60   |  50   (nur eine Fluss-Größe)
 * MidRiver           | 61   |  --
 * SmallRiver         | 62   |  --
 * Canal              | --   |  51
 *
 * Dam                | 63   |  --   (LM)
 * Lock               | 64   |  --   (LM)
 * Pass               | 66   |  --   (LM)
 *
 * WayPoint           | 68   |  --   (als Flag bei SinglePoint)
 *
 * Flight             | 69   |  52
 *
 * Task               | 71   |  53
 *
 * In einigen ASCII-Dateien taucht der Typ 71 für Kanäle auf,
 * ausserdem gibt es einen Typ für Grenzen in Seen!
 *
 *===============================================================================
 *
 * Infos:
 *
 * Lufträume:  Ober-/Untergrenze(ntyp), Name, ID, temp
 *
 * Flugplätze: Position, Name, Kennung, [Bahninfos (Länge, Richtung, Belag)],
 *             Höhe, Frequenz, vdf
 *
 * Glidersite: zusätzlich Startart
 *
 */

#define CITY            42
#define HIGHWAY         45
#define ROAD            46
#define RAILWAY         47
#define LAKE            49
#define RIVER           50
#define CANAL           51
