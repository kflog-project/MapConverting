#!/usr/bin/perl -w

########################################################################
##
##   gnsDataBase2KFLog.pl
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
## This program is used to export point data from the database into the
## KFLog ascii format
##
########################################################################

#use strict;
use DBI;

#use Math::Round;
#use Math::Trig;


##
## Configuring the db-connection:
my $dbName = "gns";
my $dbUser = "florian";

##
## The output directory containing the data-files:
my $outDir = "/home/florian/Entwicklung/Punktdatenbank/ascii/";


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



my @dbZeile;

my $t;
my $select;


$select = $database->prepare("
SELECT lat,lon,fullname,pc,shortform
FROM gns
WHERE dsg='PPL'
AND lat>48
AND lat<49
AND lon>'8.5'
AND lon<10
GROUP BY uni");




# Daten auslesen
$select->execute() || die("Fehler !!!");

# Datei öffnen
open(OUT, ">$outDir/gns.out") || die("Fehler beim öffnen der Datei!!!");

$count = 0;
while(@dbZeile = $select->fetchrow_array)
  {
 	# Zeilenweise einlesen
    $count++;



    # Datenfelder
    $lat   = $dbZeile[0];
    $lon   = $dbZeile[1];
    $name  = $dbZeile[2];
    $pc    = $dbZeile[3];
#    $dim   = $dbZeile[4];
    $short = $dbZeile[4];

    #print ;
    print OUT "[NEW]\n";
    print OUT "TYPE=43\n";
#    $lon = $lon * 10000000;
#    $lat = $lat * 10000000;
    print OUT "AT $lat $lon\n";
    print OUT "NAME=$name\n";
    print OUT "ABBREV=$short\n";
    print OUT "IMPORTANCE=$pc\n";
    print OUT "[END]\n";

  }

close(OUT);

print "fertig!\n $count Elemente erstellt!\n";


#
# Wandelt eine Position aus dem GNS ins KFLog Format
#
sub convertPosition
  {
    # Argumente: $pos: Position
    #
    my $pos = $_[0];
    # Position parsen

    $pos =~ s/\//0/g;

    if( $pos =~ /^[NS][0-9]{8}$/ )
      {
	$pos =
	  (substr($pos,1,2) * 600000 +
	   substr($pos,3,2) * 10000 +
	   substr($pos,5,4) / 6000 * 10000 + 1);

	if($_[0] =~ /^S/)
	  {
#	    print "Wir sind im Süden !!";
	    $pos = $pos * -1;
	  }
      }
    elsif( $pos =~ /^[EW][0-9]{9}/ )
      {

	$pos =
	  (substr($pos,1,3) * 600000 +
	   substr($pos,4,2) * 10000 +
	   substr($pos,6,4) / 6000* 10000 + 1);

	if($_[0] =~ /^W/)
	  {
	    $pos = $pos * -1;
	  }
      }
    else
      {
	print "#Fehler";
	print "--> $pos";
	return "0";
      }
    return round($pos);
}

