#!/usr/bin/perl -w

########################################################################
##
##   dafifImportMain.pl
##
##   This file is part of dafifImport.
##
########################################################################
##
##   Copyright (c):  2002 by Florian Ehinger, Heiner Lamprecht
##
##   This file is distributed under the terms of the General Public
##   Licence. See the file COPYING for more information.
##
##   $Id$
##
########################################################################

########################################################################
##
## This program is used to read the ascii-files of dafif and writes the
## data into the database.
##
## We are currently using editon 6 of DAFIF!
##
##
########################################################################


#
#  Die entpackten Dateien müssen vorliegen!
#
#

use strict;
use DBI;

#
# Was soll alles eingelesen werden??

my $prepare = 1;  # umwandeln der FILEX Dateien in FILEX.txt
my $airports = 1;
my $airspace = 1;
my $special_airspace = 1;


foreach (@ARGV) {
   if( $_ eq '-no_prepare')
     {
       $prepare = 0;
     }
   if( $_ eq '-no_airspace')
     {
       $airspace = 0;
     }
   if( $_ eq '-no_special_airspace')
     {
       $special_airspace = 0;
     }
   if( $_ eq '-no_airfield')
     {
       $airports = 0;
     }
   if( $_ eq '-help' || $_ eq '--help' || $_ eq '-h')
     {
        print("
This program imports the downloaded DAFIF files in a local database!

Options:
  -no_prepare      Don't convert the DAFIF FILEX files in a parable format!
                   Use this option if there are already FILEX.txt files.
  -no_airspace     Create no airspace files. Current files will not be touched!
  -no_special_air  Create no special_airspace files. Current files will not be touched!
  -no_airfield     Create no airfield files. Current files will not be touched!
  -help            This message ;-)

(C) The KFLog-Team march 2004

");
        exit;
     }
}







##
## Configuring the db-connection:
my $dbName = "dafif";
my $dbUser = "heiner";

##
## The directory containing the data-files:
my $dataSourceDir = "/data/KartenDaten/DAFIF_6/Dafif/FULLALL/";

my $fileCount = 0;
my $zeile;

#
# Reads the files and writes them with linebreaks
#
sub prepareFiles() {
  while($fileCount < 10) {
    print("Bearbeite Datei FILE$fileCount ...\n");

    open(INPUT, "$dataSourceDir/FILE$fileCount") || die("Fehler beim Öffnen von $dataSourceDir/FILE$fileCount");
    open(OUTPUT, ">$dataSourceDir/FILE$fileCount.txt") || die("Fehler beim Anlegen von $dataSourceDir/FILE$fileCount.txt");;

    while(read(INPUT, $zeile, 142)) {
      print OUTPUT $zeile;
      print OUTPUT "\n";
    }

    close(OUTPUT);
    close(INPUT);

    $fileCount += 1;
  }
}

if($prepare eq 'true') {
  prepareFiles;
}


my $country;
my $elevation;
my $icao;
my $id_string;
my $lat;
my $lon;
my $name;
my $type;
my $rw_id;
my $length;
my $width;
my $surface;
my $lat_h;
my $lon_h;
my $lat_l;
my $lon_l;
my $head_h;
my $head_l;
my $closed;
my $nav_id;
my $key;
my $frequency;
my $comm_name;
my $class;
my $class_except;
my $level;
my $upper_alt;
my $lower_alt;
my $remarks;
my $segment_id;
my $shape;
my $derivation;
my $lat_1;
my $lat_2;
my $lat_0;
my $lon_1;
my $lon_2;
my $lon_0;
my $rad_1;
my $rad_2;
my $bearing_1;
my $bearing_2;
my $times;
my $sector;

my $st_delete;

#
#  Datenbank
#

my $database = DBI->connect("dbi:mysql:$dbName",$dbUser,undef) || die("Verbindungsfehler!");

#
# L?schen aller alten Eintr?ge. Nur "country" bleibt bestehen!
#



#################################################################################################
#
#       FLUGPL?TZE
#
#       File0
#
#################################################################################################

if($airports eq 'true')
  {
    print("Importiere Flugplätze...\n");
    open(INPUT,"$dataSourceDir/FILE0.txt") || die("Could not open $dataSourceDir/FILE0.txt");

    ## löschen der alten Eintr?ge
    print("lösche Flugplatztabellen...\n");
    $st_delete = $database->prepare("DELETE FROM air_nav") || die("air_nav: Could not delete entries!");
    $st_delete->execute() || die $database->errstr;
    $st_delete = $database->prepare("DELETE FROM airport") || die("airport: Could not delete entries!");
    $st_delete->execute() || die $database->errstr;
    $st_delete = $database->prepare("DELETE FROM airport_comm") || die("airport_comm: Could not delete entries!");
    $st_delete->execute() || die $database->errstr;
    $st_delete = $database->prepare("DELETE FROM runway") || die("runway: Could not delete entries!");
    $st_delete->execute() || die $database->errstr;


    my $st_airport = $database->prepare("INSERT INTO airport VALUES (?,?,?,?,?,?,?,?)") || die("Airport fehler");

    my $st_runway_a = $database->prepare("INSERT INTO runway (ID_STRING,RW_ID,LENGTH,WIDTH,SURFACE,LATITUDE_H,LONGITUDE_H) VALUES (?,?,?,?,?,?,?)") || die("runway fehler");

    my $st_runway_b = $database->prepare("UPDATE runway SET LATITUDE_L=?,LONGITUDE_L=?,HEADING_H=?,HEADING_L=?,CLOSED=? WHERE ID_STRING=? AND RW_ID=?") || die("runway_b fehler");

    my $st_navaid = $database->prepare("INSERT INTO air_nav VALUES (?,?,?,?,?,?)") || die("navaid fehler");

    my $st_communication = $database->prepare("INSERT INTO airport_comm VALUES (?,?,?,?)") || die("comm fehler");

    while (<INPUT>) {
      $zeile = $_;
      if (substr($zeile, 0, 1) =~ m/\^/) {
	next;
      }

      if (substr($zeile, 0, 2) eq "01") {

	print(".");
	#
	# AIRPORT-RECORD
	#
	$icao = substr($zeile, 3, 4);
	$id_string = substr($zeile, 7, 7);  # Airport Identification
	$name = substr($zeile, 14, 38);
	$country = substr($zeile, 54, 4);
	$lat = substr($zeile, 83, 9);
	$lon = substr($zeile, 92, 10);
	$elevation = substr($zeile, 102, 5);
	$type = substr($zeile, 107, 1);

	$st_airport->execute($id_string,$icao,$name,$country,$lat,$lon,$elevation,$type) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "02") {
	#
	# RUNWAY
	#
	$id_string = substr($zeile, 7, 7);
	$rw_id = substr($zeile, 14, 3);
	$length = substr($zeile, 28, 5);
	$width = substr($zeile, 33, 5);
	$surface = substr($zeile, 38, 3);
	$lat_h = substr($zeile, 67, 9);
	$lon_h = substr($zeile, 76, 10);

	$st_runway_a->execute($id_string,$rw_id,$length,$width,$surface,$lat_h,$lon_h) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "03") {
	#
	# RUNWAY
	#
	$id_string = substr($zeile, 7, 7);
	$rw_id = substr($zeile, 14, 3);
	$lat_l = substr($zeile, 39, 9);
	$lon_l = substr($zeile, 48, 10);
	$head_h = substr($zeile, 99, 4);
	$head_l = substr($zeile, 103, 4);
	$closed = substr($zeile, 104, 1);

	$st_runway_b->execute($lat_l,$lon_l,$head_h,$head_l,$closed,$id_string,$rw_id) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "15") {
	#
	# AIRPORT NAVAID
	#
	$id_string = substr($zeile, 7, 7);
	$nav_id = substr($zeile, 14, 4);
	$type = substr($zeile, 18, 1);
	$country = substr($zeile, 19, 2);
	$key = substr($zeile, 21, 2);
	$name = substr($zeile, 23, 38);

	$st_navaid->execute($id_string,$nav_id,$type,$country,$key,$name) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "16") {
	#
	# AIRPORT COMMUNICATION
	#
	$id_string = substr($zeile, 7, 7);
	$type = substr($zeile, 14, 4);
	$name = substr($zeile, 18, 20);
	$frequency = substr($zeile, 40, 9);

	$st_communication->execute($id_string,$type,$name,$frequency) || die $database->errstr;
      }
    }

    close(INPUT);
  }




#################################################################################################
#
# AIRSPACE
#
#################################################################################################

if($airspace eq 'true')
  {
    #
    #  FILE 05
    #
    ## erstmal die ben?tigten Tabellen leeren
    $st_delete = $database->prepare("DELETE FROM airspace;") || die("Airspace delete fehler");
    $st_delete->execute() || die $database->errstr;

    $st_delete = $database->prepare("DELETE FROM airspace_segment_a;") || die("Airspace_segment_a delete fehler");
    $st_delete->execute() || die $database->errstr;

    $st_delete = $database->prepare("DELETE FROM airspace_segment_b;") || die("Airspace_segment_b delete fehler");
    $st_delete->execute() || die $database->errstr;

    $st_delete = $database->prepare("DELETE FROM airspace_segments;") || die("Airspace_segments delete fehler");
    $st_delete->execute() || die $database->errstr;
    #### Tabellen geleert ###


    print("Importiere Luftr?ume...\n");
    open(INPUT,"$dataSourceDir/FILE5.txt") || die("Fehler beim ?ffnen $dataSourceDir/FILE5.txt");

    my $st_airspace_a = $database->prepare("INSERT INTO airspace (ID_STRING, ICAO, TYPE, NAME, COMM_NAME, FREQUENCY,CLASS,CLASS_EXEPTION) VALUES (?,?,?,?,?,?,?,?)") || die("Airport fehler");
    my $st_airspace_b = $database->prepare("UPDATE airspace SET LEVEL=?, UPPER_ALT=?, LOWER_ALT=?, CLASS_REMARKS=? WHERE ID_STRING=?") || die("Airport fehler");

    my $st_boundary_seg_a = $database->prepare("INSERT INTO airspace_segment_a  VALUES (?,?,?,?,?,?,?,?)") || die("runway fehler");

    my $st_boundary_seg_b = $database->prepare("INSERT INTO airspace_segment_b  VALUES (?,?,?,?,?,?,?,?)") || die("runway fehler");



    while (<INPUT>) {
      $zeile = $_;
      if (substr($zeile, 0, 1) =~ m/\^/) {
	next;
      }

      if (substr($zeile, 0, 2) eq "01") {
	#
	# Boundary Record a
	#
	$icao = substr($zeile, 3, 4);
	$id_string = substr($zeile, 7, 7);
	$type = substr($zeile, 14, 2);
	$name = substr($zeile, 16, 38);
	$comm_name = substr($zeile, 98, 20);
	$frequency = substr($zeile, 118, 9);
	$class = substr($zeile, 127, 1);
	$class_except = substr($zeile, 128, 1);

	$st_airspace_a->execute($id_string,$icao,$type,$name,$comm_name,$frequency,$class,$class_except) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "02") {
	#
	# BOUNDARY b
	#
	$id_string = substr($zeile, 7, 7);
	$level = substr($zeile, 23, 1);
	$upper_alt = substr($zeile, 24, 10);
	$lower_alt = substr($zeile, 34, 9);
	$remarks = substr($zeile, 43, 80);

	$st_airspace_b->execute($level,$upper_alt,$lower_alt,$remarks,$id_string) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "03") {
	#
	# BOUNDARY-SEG a
	#
	$id_string = substr($zeile, 7, 7);
	$segment_id = substr($zeile, 14, 5);
	$shape = substr($zeile, 19, 1);
	$derivation = substr($zeile, 20, 1);
	$lat_1 = substr($zeile, 78, 9);
	$lon_1 = substr($zeile, 87, 10);
	$lat_2 = substr($zeile, 97, 9);
	$lon_2 = substr($zeile, 106, 10);

	$st_boundary_seg_a->execute($id_string,$segment_id,$shape,$derivation,$lat_1,$lon_1,$lat_2,$lon_2) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "04") {
	#
	# BOUNDARY SEG b
	#
	$id_string = substr($zeile, 7, 7);
	$segment_id = substr($zeile, 14, 5);
	$lat_0 = substr($zeile, 19, 9);
	$lon_0 = substr($zeile, 28, 10);
	$rad_1 = substr($zeile, 38, 5);
	$rad_2 = substr($zeile, 43, 5);
	$bearing_1 = substr($zeile, 48, 4);
	$bearing_2 = substr($zeile, 52, 4);

	$st_boundary_seg_b->execute($id_string,$segment_id,$lat_0,$lon_0,$rad_1,$rad_2,$bearing_1,$bearing_2) || die $database->errstr;
      }
    }

    close(INPUT);

    my $st_merge = $database->prepare("
insert into airspace_segments
 (ID_STRING,
  SEGMENT_ID,
  SHAPE,
  LAT_1,
  LON_1,
  LAT_2,
  LON_2,
  LAT_0,
  LON_0,
  RADIUS)
SELECT
  airspace_segment_a.id_string,
  airspace_segment_a.segment_id,
  airspace_segment_a.shape,
  airspace_segment_a.lat_1,
  airspace_segment_a.lon_1,
  airspace_segment_a.lat_2,
  airspace_segment_a.lon_2,
  airspace_segment_b.lat_0,
  airspace_segment_b.lon_0,
  airspace_segment_b.radius_1
FROM
  airspace_segment_a,
  airspace_segment_b
WHERE
  airspace_segment_a.id_string=airspace_segment_b.id_string
AND
  airspace_segment_a.segment_id=airspace_segment_b.segment_id") || die("airspace merge fehler");
    $st_merge->execute() || die $database->errstr;

}


#################################################################################################
#
# SPECIAL USE AIRSPACE
#
#################################################################################################

if($special_airspace eq 'true')
  {
    print("Importiere \"Special\" Luftr?ume...\n");

    ## erstmal die ben?tigten Tabellen leeren
    my $st_delete = $database->prepare("DELETE FROM airspace_special;") || die("Airspace_special delete fehler");
    $st_delete->execute() || die $database->errstr;

    $st_delete = $database->prepare("DELETE FROM airspace_special_segment_a;") || die("Airspace_special_segment_a delete fehler");
    $st_delete->execute() || die $database->errstr;

    $st_delete = $database->prepare("DELETE FROM airspace_special_segment_b;") || die("Airspace_special_segment_b delete fehler");
    $st_delete->execute() || die $database->errstr;

    $st_delete = $database->prepare("DELETE FROM airspace_special_segments;") || die("Airspace_special_segments delete fehler");
    $st_delete->execute() || die $database->errstr;
    #### Tabellen geleert ###

    open(INPUT,"$dataSourceDir/FILE6.txt") || die("Fehler beim ?ffnen $dataSourceDir/FILE6.txt");
;


    my $st_su_airspace_a = $database->prepare("INSERT INTO airspace_special (ID_STRING, SECTOR, ICAO, NAME, TYPE) VALUES (?, ?,?,?,?)") || die("Airport fehler");
    my $st_su_airspace_b = $database->prepare("UPDATE airspace_special SET COMM_NAME=?, FREQUENCY=?, LEVEL=?, UPPER_ALT=?, LOWER_ALT=?, TIMES=? WHERE ID_STRING=? AND SECTOR=?") || die("Airport fehler");

    my $st_sua_boundary_seg_a = $database->prepare("INSERT INTO airspace_special_segment_a  VALUES (?,?,?,?,?,?,?,?,?)") || die("runway fehler");

    my $st_sua_boundary_seg_b = $database->prepare("INSERT INTO airspace_special_segment_b  VALUES (?,?,?,?,?,?,?,?,?)") || die("runway fehler");



    while (<INPUT>) {
      $zeile = $_;
      if (substr($zeile, 0, 1) =~ m/\^/) {
	next;
      }

      if (substr($zeile, 0, 2) eq "01") {
	#
	# SUA Boundary Record a
	#
	$icao = substr($zeile, 3, 4);
	$id_string = substr($zeile, 7, 12);
	$sector = substr($zeile, 19, 2);
	$name = substr($zeile, 21, 38);
	$type = substr($zeile, 59, 1);

	$st_su_airspace_a->execute($id_string,$sector,$icao,$name,$type) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "02") {
	#
	# SUA BOUNDARY b
	#
	$id_string = substr($zeile, 7, 12);
	$sector = substr($zeile, 19, 2);
	$comm_name = substr($zeile, 21, 20);
	$frequency = substr($zeile, 41, 9);
	$level = substr($zeile, 59, 1);
	$upper_alt = substr($zeile, 60, 10);
	$lower_alt = substr($zeile, 70, 9);
	$times = substr($zeile, 79, 38);

	$st_su_airspace_b->execute($comm_name,$frequency,$level,$upper_alt,$lower_alt,$times,$id_string,$sector) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "03") {
	#
	# SUA BOUNDARY-SEG a
	#
	$id_string = substr($zeile, 7, 12);
	$sector = substr($zeile, 19, 2);
	$segment_id = substr($zeile, 21, 5);
	$shape = substr($zeile, 26, 1);
	$derivation = substr($zeile, 27, 1);
	$lat_1 = substr($zeile, 85, 9);
	$lon_1 = substr($zeile, 94, 10);
	$lat_2 = substr($zeile, 104, 9);
	$lon_2 = substr($zeile, 113, 10);

	$st_sua_boundary_seg_a->execute($id_string,$sector,$segment_id,$shape,$derivation,$lat_1,$lon_1,$lat_2,$lon_2) || die $database->errstr;
      } elsif (substr($zeile, 0, 2) eq "04") {
	#
	# SUA BOUNDARY SEG b
	#
	$id_string = substr($zeile, 7, 12);
	$sector = substr($zeile, 19, 2);
	$segment_id = substr($zeile, 21, 5);
	$lat_0 = substr($zeile, 26, 9);
	$lon_0 = substr($zeile, 35, 10);
	$rad_1 = substr($zeile, 45, 5);
	$rad_2 = substr($zeile, 50, 5);
	$bearing_1 = substr($zeile, 55, 4);
	$bearing_2 = substr($zeile, 59, 4);

	$st_sua_boundary_seg_b->execute($id_string,$sector,$segment_id,$lat_0,$lon_0,$rad_1,$rad_2,$bearing_1,$bearing_2) || die $database->errstr;
      }
    }

    close(INPUT);

    my $st_sua_merge = $database->prepare("
insert into airspace_special_segments
 (ID_STRING,
  SECTOR,
  SEGMENT_ID,
  SHAPE,
  LAT_1,
  LON_1,
  LAT_2,
  LON_2,
  LAT_0,
  LON_0,
  RADIUS)
SELECT
  airspace_special_segment_a.id_string,
  airspace_special_segment_a.sector,
  airspace_special_segment_a.segment_id,
  airspace_special_segment_a.shape,
  airspace_special_segment_a.lat_1,
  airspace_special_segment_a.lon_1,
  airspace_special_segment_a.lat_2,
  airspace_special_segment_a.lon_2,
  airspace_special_segment_b.lat_0,
  airspace_special_segment_b.lon_0,
  airspace_special_segment_b.radius_1
FROM
  airspace_special_segment_a,
  airspace_special_segment_b
WHERE
  airspace_special_segment_a.id_string=airspace_special_segment_b.id_string
AND
  airspace_special_segment_a.sector=airspace_special_segment_b.sector
AND
  airspace_special_segment_a.segment_id=airspace_special_segment_b.segment_id") || die("sua merge fehler");
    $st_sua_merge->execute() || die $database->errstr;
}


print("Ende\n");
## Okay, everything is said and done ...
exit 0;






#insert into airspace_special_segments
# (ID_STRING,
#  SECTOR,
#  SEGMENT_ID,
#  SHAPE,
#  LAT_1,
#  LON_1,
#  LAT_2,
#  LON_2,
#  LAT_0,
#  LON_0,
#  RADIUS)
#SELECT
#  airspace_special_segment_a.id_string,
#  airspace_special_segment_a.sector,
#  airspace_special_segment_a.segment_id,
#  airspace_special_segment_a.shape,
#  airspace_special_segment_a.lat_1,
#  airspace_special_segment_a.lon_1,
#  airspace_special_segment_a.lat_2,
#  airspace_special_segment_a.lon_2,
#  airspace_special_segment_b.lat_0,
#  airspace_special_segment_b.lon_0,
#  airspace_special_segment_b.radius_1
#FROM
#  airspace_special_segment_a,
#  airspace_special_segment_b
#WHERE
#  airspace_special_segment_a.id_string=airspace_special_segment_b.id_string
#AND
#  airspace_special_segment_a.sector=airspace_special_segment_b.sector
#AND
#  airspace_special_segment_a.segment_id=airspace_special_segment_b.segment_id;
