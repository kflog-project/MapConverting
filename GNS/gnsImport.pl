#!/usr/bin/perl -w

########################################################################
##
##   gnsImportMain.pl
##
##   This file is part of gnsImport.
##
########################################################################
##
##   Copyright (c):  2003 Florian Ehinger
##
##   This file is distributed under the terms of the General Public
##   Licence. See the file COPYING for more information.
##
##
########################################################################

########################################################################
##
## This program is used to read the ascii-files of the GNS-Database and
## writes the data into the database.
##
########################################################################


#use strict;
use DBI;


##
## Configuring the db-connection:
my $dbName = "gns";
my $dbUser = "florian";

##
## The directory containing the data-files:
my $dataSourceDir = "/home/florian/Entwicklung/Punktdatenbank/";

my $fileCount = 0;
my $zeile;

my $rc;
my $ufi;
my $uni;
my $dd_lat;
my $dd_long;
my $dms_lat;
my $dms_long;
my $utm;
my $jog;
my $fc;
my $dsg;
my $pc;
my $cc1;
my $adm1;
my $adm2;
my $dim;
my $cc2;
my $nt;
my $lc;
my $short_form;
my $generic;
my $sort_name;
my $full_name;
my $full_name_nd;
my $modify_date;


#
#  Database
#

my $database = DBI->connect("dbi:mysql:$dbName",$dbUser,undef) || die("Verbindungsfehler!");

#
# Löschen aller alten Einträge. Nur "country" bleibt bestehen!
#
$database->prepare("DELETE * FROM gns") || die("Could not delete entries!");


#
# erstmal nur die Datei für Deutschland importieren
#
print("Importiere Datei...\n");
my $st_gns = $database->prepare("INSERT INTO gns VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)") || die("GNS_MAIN fehler");


open(INPUT,"$dataSourceDir/gm.txt") || die("Could not open file");
my $n = 0;
while (<INPUT>) {
  $zeile = $_;

  $n++;



# Zeile einlesen
  ($rc, $ufi, $uni, $dd_lat, $dd_long, $dms_lat, $dms_long, $utm, $jog, $fc, $dsg, $pc, $cc1, $adm1, $adm2, $dim, $cc2, $nt, $lc, $short_form, $generic, $sort_name, $full_name, $full_name_nd, $modify_date) = split(/\t/,$zeile);


  if ($rc =~ /^RC/) {
#    print("------------> next\n");
    next;
  }

#
#  $rc		RegionCode
#  $ufi		UniqueFeatureIdentifier
#  $uni		Unique Name Identifier
#  $dd_lat	Latitude (decimal)
#  $dd_long	Longiutude (decimal)
#  $fc		Feature Classification
#  $dsg		Feature Designation Code
#  $pc		Populated Place Classification
#  $cc1		Country Code
#  $dim		Dimension: Elevation or Population
#  $nt		NameType
#  $lc		Language Code
#  $short_form	Shortform of the name
#  $full_name	Name

  $st_gns->execute($rc, $ufi, $uni, $dd_lat, $dd_long, $fc, $dsg, $pc, $cc1, $dim, $nt, $lc, $short_form, $full_name) || die $database->errstr;
}

close(INPUT);

print "$n Elemente importiert\n";

exit;

