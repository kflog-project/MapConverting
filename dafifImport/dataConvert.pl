#!/usr/bin/perl -w

#
#          Erstellt aus der Dafif Datenbank
#          KFlog Luftraum Dateien im Ascii Format
#
#
#

##############################################################
#
#  Parameter:
#   -no_airspace   : keine Lufträume Konvertieren
#   -no_airfield  : keine Flugplätze konvertieren
#
##############################################################

use strict;
use DBI;

use Math::Round;
use Math::Trig;

my $airspace = 1;
my $airfield = 1;

foreach (@ARGV) {
   if( $_ eq '-no_airspace')
     {
       $airspace = 0;
     }
   if( $_ eq '-no_airfield')
     {
       $airfield = 0;
     }
   if( $_ eq '-help' || $_ eq '--help' || $_ eq '-h')
     {
        print("
This program creates KFLog Ascii Airspace and Airfield files out of the DAFIF Database!

Options:
   -no_airspace    Create no airspace files. Current files will not be touched!
   -no_airfield    Create no airfield files. Current files will not be touched!
   -help           This message ;-)

(C) The KFLog-Team march 2004

");
        exit;
     }
}




#
# Länder
#
my $country_code = '';



my $cta_count = 0;
my $cta_count_gesamt = 0;

my $country_name;
my $shape;
my $lat_0;
my $lon_0;
my $lat_1;
my $lon_1;
my $lat_2;
my $lon_2;
my $radius;
my $start;
my $ende;

my $id_string;
my $type;
my $class;
my $level;
my $lower_alt;
my $upper_alt;
my $name;
my $l_type;
my $u_type;
my $ignore;
my $sector;
my $selectHandle;
my $select_a;


##
## Configuring the db-connection:
my $dbName = "dafif";
my $dbUser = "heiner";

##
## The directory containing the data-files:
my $dataOutDir = "/data/KartenDaten/KFLog-Karten/dafif_out/";


##
##  Datenbank
##

my $database = DBI->connect("dbi:mysql:$dbName",$dbUser,undef) || die("Verbindungsfehler!");

my @dbZeile;
my @dbZeile_a;

my $t;
my $select;

$select = $database->prepare("
SELECT country,code
FROM country ORDER BY code");

$select->execute() || die("Fehler !!!");
while(@dbZeile_a = $select->fetchrow_array)
  {
    $country_name = $dbZeile_a[0];
    $country_name =~ s/\ /\_/g;
    $country_name =~ s/\//\_/g;
print "$country_name\n";
    $country_code = $dbZeile_a[1];

    if( $airspace )
      {
        # Airspace Datei öffnen
        print "Airspace\n";
        $cta_count_gesamt += $cta_count;
        $cta_count = 0;
        open(OUT, ">$dataOutDir$country_name.out") || die("Fehler beim öffnen der Ausgabe Datei!!!");

        erstelle_luftraum('true');  # special airspaces
        erstelle_luftraum('false'); # airspaces

        close(OUT);
      }

#print "CTA AREAs: ";
#print "$cta_count \n";

    if( $airfield )
      {
        print "airfields\n";
        # Airport Datei Öffnen
        open(OUT_AIRPORT, ">$dataOutDir/Airport/$country_name.out") || die("Fehler beim öffnen der Airport Datei!!!");

        erstelle_airport();  # Flugplätze

        close(OUT_AIRPORT);
      }

  }
#print "CTA AREAs: ";
#print "$cta_count_gesamt \n";


#
# Wandelt eine Position aus dem Dafif ins KFlog Format
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

#
# Gibt den Kurs zwischen zwei Positionen an
#
# Kurs von P0 nach P1
sub getBearing
  {
    # Argumente: $lat_0 $lon_0
    #            $lat_1 $lon_1
    #
    my $lat_0 = convertPosition($_[0]);
    my $lon_0 = convertPosition($_[1]) + 180 * 600000;
    my $lat_1 = convertPosition($_[2]);
    my $lon_1 = convertPosition($_[3]) + 180 * 600000;

    my $d_x = ($lon_1 - $lon_0) * cos(deg2rad(($lat_0 + $lat_1) / 2 / 600000));
    my $d_y = $lat_1 - $lat_0;

    if($d_y == "0")
      {
	if($d_x < 0)
	  {
	    return 270;
	  }
	return 90;
      }

    my $kurs = rad2deg(atan($d_x / $d_y));

    if($d_x < 0)
      {
	if($d_y < 0)
	  {
	    # 3. Quartal
	    return 180 + $kurs;
	  }
	# 4. Quartal
	return 360 + $kurs;
      }
    if($d_y < 0)
      {
	# 2. Quartal
	return 180 + $kurs;
      }
    return $kurs;
    # 1. Quartal
  }

sub calcArcPoints
  {
    # Argumente: start,ende,center_lat,center_lon,radius,richtung
    #
    #
    #
    my $startArc;
    my $endArc;
    my $center_lat = $_[2];
    my $center_lon = $_[3];
    my $x;
    my $y;
    my $richtung = $_[5];
    my $radius = $_[4] * 10000 / 100;

    $center_lat = convertPosition($center_lat);
    $center_lon = convertPosition($center_lon);

    if($richtung == '1')
      {
	$startArc = nearest(5, ($_[0]) + 2.5);
	$endArc = nearest(5, ($_[1]) - 2.5);

	while($startArc <= $endArc)
	  {
	    # im Uhrzeigersinn
	    $x = sin(deg2rad($startArc)) * $radius;
	    $y = cos(deg2rad($startArc)) * $radius;
	    $startArc += 5;

	    $y = $center_lat + $y;
	    $x = $center_lon + ($x / cos(deg2rad($center_lat / 600000)));
	    $y = nearest(1, $y) ;
	    $x = nearest(1, $x) ;

	    print OUT "$y $x\n";
	  }
      }
    else
      {
	$startArc = nearest(5, ($_[0]) - 2.5);
	$endArc = nearest(5, ($_[1]) + 2.5);

	while($startArc >= $endArc)
	  {
	    # gegen den Uhrzeigersinn
	    $x = sin(deg2rad($startArc)) * $radius;
	    $y = cos(deg2rad($startArc)) * $radius;
	    $startArc -= 5;

	    $y = $center_lat + $y;
	    $x = $center_lon + ($x / cos(deg2rad($center_lat / 600000)));
	    $y = nearest(1, $y) ;
	    $x = nearest(1, $x) ;

	    print OUT "$y $x\n";
	  }
      }
  }
#
#
sub createArcPoints
  {
    # Argumente: start,ende,center_lat,center_lon,radius,richtung
    #
    #
    #
    if(abs($_[0] - $_[1]) < 5)
      {
	return;
      }

    if(($_[5] == '1') && ($_[0] > $_[1]))
      {
	# Uhrzeigersinn
	calcArcPoints($_[0], 365,  $_[2], $_[3], $_[4], $_[5]);
	calcArcPoints(-5, $_[1], $_[2], $_[3], $_[4], $_[5]);
	return;
      }
    elsif(($_[5] == '-1') && ($_[1] > $_[0]))
      {
	# Uhrzeigersinn
	calcArcPoints($_[0], 365,  $_[2], $_[3], $_[4], $_[5]);
	calcArcPoints(-5, $_[1], $_[2], $_[3], $_[4], $_[5]);
	return;
      }

    calcArcPoints($_[0], $_[1], $_[2], $_[3], $_[4], $_[5]);
  }



sub erstelle_luftraum
  {
    if($_[0] eq 'true')
      {
	$select_a = $database->prepare("
SELECT *
FROM airspace_special,country
WHERE (ICAO>=country.icao_start AND ICAO<=country.icao_end) AND country.code=?");


	$selectHandle = $database->prepare("
SELECT SHAPE,LAT_0,LON_0,LAT_1,LON_1,LAT_2,LON_2,RADIUS
FROM airspace_special_segments
WHERE ID_STRING=? AND SECTOR=?
ORDER BY SEGMENT_ID") || die("Select-Fehler");
      }
    else
      {
	$select_a = $database->prepare("
SELECT *
FROM airspace,country
    WHERE (ICAO>=country.icao_start AND ICAO<=country.icao_end) AND country.code=?");

	$selectHandle = $database->prepare("
SELECT SHAPE,LAT_0,LON_0,LAT_1,LON_1,LAT_2,LON_2,RADIUS
FROM airspace_segments
WHERE ID_STRING=?
ORDER BY SEGMENT_ID") || die("Select-Fehler");
      }

    $select_a->execute($country_code) || die("Fehler !!!");

    while(@dbZeile_a = $select_a->fetchrow_array)
      {

	$ignore = 0;
	if($_[0] eq 'true')
	  {
	    $class = "special";
	    $id_string = $dbZeile_a[0];
	    $sector    = $dbZeile_a[1];
	    #    $icao      = $dbZeile_a[2];
	    $name      = $dbZeile_a[3];
	    $type      = $dbZeile_a[4];
	    #    $comm_name = $dbZeile_a[5];
	    #    $frequency = $dbZeile_a[6];
	    $level     = $dbZeile_a[7];
	    $upper_alt = $dbZeile_a[8];
	    $lower_alt = $dbZeile_a[9];
	    #    $remarks   = $dbZeile_a[10];
	    # Als Name erscheint die Kennung, ggf. der Sektor, sowie der Gebietsname
	    $name = $id_string . $sector . " (" . $name . ")";
	  }
	else
	  {
	    $id_string = $dbZeile_a[0];
	    #    $icao      = $dbZeile_a[1];
	    $type      = $dbZeile_a[2];
	    $name      = $dbZeile_a[3];
	    #    $comm_name = $dbZeile_a[4];
	    #    $frequency = $dbZeile_a[5];
	    $class     = $dbZeile_a[6];
	    $level     = $dbZeile_a[8];
	    $upper_alt = $dbZeile_a[9];
	    $lower_alt = $dbZeile_a[10];
	    #    $remarks   = $dbZeile_a[11];
	  }

	if($name =~ /CTA/)
	  {
	    # CTA Kontrollone in großer Höhe
	    $ignore = 1;
		$cta_count += 1;
	  }


	if($level =~ /H/)
	  {
	    $ignore = 1;
	  }
	elsif($type =~ /(01|02|03|04|05|08|09|10|12)/)
	  {
	    $ignore = 1;

	  }


	# Höhen
	if($lower_alt =~ /(GND|SURFACE|AGL)/)
	  {
	    #GND
	    $l_type = 2;
	  }
	elsif($lower_alt =~ /FL/)
	  {
	    $l_type = 3;
	  }
	elsif($lower_alt =~ /AMSL/)
	  {
	    $l_type = 1;
	  }
	else
	  {
	    # BY BOTAM
	    # Not set
	    $l_type = 0;
	  }

	if($upper_alt =~ /(GND|SURFACE|AGL)/)
	  {
	    #GND
	    $u_type = 2;
	  }
	elsif($upper_alt =~ /FL/)
	  {
	    $u_type = 3;
	  }
	elsif($upper_alt =~ /AMSL/)
	  {
	    $u_type = 1;
	  }
	elsif($upper_alt =~ /UNLTD/)
	  {
	    $u_type = 1;
	  }
	else
	  {
	    # BY BOTAM
	    # Not set
	    $u_type = 0;
	  }

	$lower_alt =~ s/(GND|UNLTD|SURFACE|AGL|FL|AMSL|BY BOTAM)//;
	$upper_alt =~ s/(GND|UNLTD|SURFACE|AGL|FL|AMSL|BY BOTAM)//;
	$lower_alt =~ s/^$/0/;
	$upper_alt =~ s/^$/0/;


	if(($l_type == '3') && ($lower_alt >= '80'))
	  {
	    # Luftraum beginnt über Flugfläche 80!
	    $ignore = 1;
	  }

	# Typen
	if($class eq 'A')
	  {
	    $class = 21;
	    if($type == '07')
	      {
		# ctz
		# print "#====> A Kontrollzone";
	      }
	  }
	elsif($class eq 'B')
	  {
	    $class = 22;
	    if($type == '07')
	      {
		# ctz
		#  print "#====> B Kontrollzone";
	      }
	  }
	elsif($class eq 'C')
	  {
	    $class = 23;
	    if($type == '07')
	      {
		# ctz
		$class = 28;
	      }
	  }
	elsif($class eq 'D')
	  {
	    $class = 24;
	    if($type == '07')
	      {
		# ctz
		$class = 29;
	      }
	  }
	elsif($class eq 'E')
	  {
	    $class = 26;
	    if ($lower_alt == '1000') {
	      $class = 25;
	    } elsif ($lower_alt == '2500') {
	      $ignore = 1;
	    }
	  } elsif ($class eq 'F') {
	    # ctr
	    $class = 27;
	  } elsif ($class eq 'special') {
	    if ($type eq 'D') {
	      $class = 30;
	    } elsif ($type eq 'R') {
	      $class = 32;
	    } else {
	      # nicht bekannter SUA
	      $name = "$name   --  $class";
	      $class = 31;
	    }
	  } else {
#	    print "#========> Unbekannter Luftraum! Typ: $type\n";
	    $name = "$name   --  $class";
	    $class = 31;
	  }

#	$name = "$name -- dafif-type: $type";
	$name = "$name";

	if ($ignore == '0') {

	  print OUT "[NEW]\n";
	  print OUT "TYPE=$class\n";
	  print OUT "ID=$id_string\n";
	  print OUT "LOWER=$lower_alt\n";
	  print OUT "LTYPE=$l_type\n";
	  print OUT "UPPER=$upper_alt\n";
	  print OUT "UTYPE=$u_type\n";
	  print OUT "NAME=$name\n";

	  if ($_[0] eq 'true') {
	    $selectHandle->execute($id_string,$sector) || die("Fehler !!!");
	  } else {
	    $selectHandle->execute($id_string) || die("Fehler !!!");
	  }

	  while (@dbZeile = $selectHandle->fetchrow_array) {
	    $shape = $dbZeile[0];
	    $lat_0 = $dbZeile[1];
	    $lon_0 = $dbZeile[2];
	    $lat_1 = $dbZeile[3];
	    $lon_1 = $dbZeile[4];
	    $lat_2 = $dbZeile[5];
	    $lon_2 = $dbZeile[6];
	    $radius = $dbZeile[7];


	    if ($shape eq 'A') {
	      # Punkt
	      print OUT convertPosition($lat_0);
	      print OUT " ";
	      print OUT convertPosition($lon_0);
	      print OUT "\n";
	    } elsif ($shape =~ /[BHG]/) {
	      # Linien
	      # G = Loxodrome Rhump Line schneidet alle Meridiane im gleichen Winkel !!!!
	      # B = Orthodrome Großkreis muss vermutlich anderst berechnet werden!!!!
	      print OUT convertPosition($lat_1);
	      print OUT " ";
	      print OUT convertPosition($lon_1);
	      print OUT "\n";

	      print OUT convertPosition($lat_2);
	      print OUT " ";
	      print OUT convertPosition($lon_2);
	      print OUT "\n";
	    } elsif ($shape eq 'C') {
	      #Kreis
	      #
	      createArcPoints(-5, 365, $lat_0, $lon_0, $radius, 1);
	    } elsif ($shape =~ /[RL]/) {
	      # Kreisbogen Uhrzeigersinn
	      print OUT convertPosition($lat_1);
	      print OUT " ";
	      print OUT convertPosition($lon_1);
	      print OUT "\n";
	      #		print "--> $lat_1  $lon_1\n";

	      $start = getBearing($lat_0,$lon_0,$lat_1,$lon_1);
	      $ende = getBearing($lat_0,$lon_0,$lat_2,$lon_2);

	      if ($shape eq 'L') {
		createArcPoints($start, $ende, $lat_0, $lon_0, $radius,-1);
	      } else {
		createArcPoints($start, $ende, $lat_0, $lon_0, $radius,1);
	      }

	      print OUT convertPosition($lat_2);
	      print OUT " ";
	      print OUT convertPosition($lon_2);
	      print OUT "\n";

	      #	print "$start  $ende \n";
	    } else {
	      print "sonstiges ";
	      print "$shape \n";
	    }
	  }
	  print OUT "[END]\n";
	}


      }
}

sub erstelle_airport
  {
    my $new_lat;
    my $new_lon;
    my ($icao, $country, $longitude, $latitude, $elevation);
    my ($select_test, $select_runway);
    my ($runway_id, $runway_length, $runway_width, $runway_heading_l,
        $runway_heading_h, $runway_closed, $runway_direction, $runway_surface);

    # Daten auslesen
    $select_test = $database->prepare("
SELECT *
FROM airport,country
WHERE (ICAO>=country.icao_start AND ICAO<=country.icao_end) AND country.code=?");

    $select_test->execute($country_code) || die("Fehler !!!");

    while(@dbZeile = $select_test->fetchrow_array)
      {
        $id_string = $dbZeile[0];
        $icao = $dbZeile[1];
        $name = $dbZeile[2];
        $country = $dbZeile[3];
        $latitude = $dbZeile[4];
        $longitude = $dbZeile[5];
        $elevation = $dbZeile[6];
        $type = $dbZeile[7];


### Daten bearbeiten
        if( $type eq 'A')
          {
            $type = 2;
          }
        elsif( $type eq 'B')
          {
            $type = 4;
          }
        elsif( $type eq 'C')
          {
            $type = 3;
          }
        elsif( $type eq 'D')
          {
            $type = 5;
          }
        else
          {
             ## Fehler
          }

### Ausgabe in Datei
	    print OUT_AIRPORT "[NEW]\n";
        print OUT_AIRPORT "TYPE=$type\n";
        print OUT_AIRPORT "ID=$id_string\n";
        print OUT_AIRPORT "NAME=$name\n";
        print OUT_AIRPORT "ELEVATION=$elevation\n";
        print OUT_AIRPORT "ALIAS=$icao\n";
        $new_lat = convertPosition( $latitude ) / 60000;
        $new_lon = convertPosition( $longitude ) / 60000;
	    print OUT_AIRPORT "AT $new_lat $new_lon\n";

        # Runway Informationen auslesen
        $select_runway = $database->prepare("
SELECT rw_id, length, width, surface, heading_h, heading_l, closed
FROM runway
WHERE  id_string=?");

        $select_runway->execute($id_string) || die("Fehler bei Runway select!!!");

        while(@dbZeile = $select_runway->fetchrow_array)
         {
           $runway_id = $dbZeile[0];
           $runway_length = $dbZeile[1];
           $runway_width = $dbZeile[2];
           $runway_surface = $dbZeile[3];
           $runway_heading_h = substr($dbZeile[4], 0, 2);
           $runway_heading_l = substr($dbZeile[5], 0, 2);
           $runway_closed = $dbZeile[6];

           $runway_direction = "$runway_heading_h/$runway_heading_l";

#      Hard permannent
# ASP	ASPHALT, ASPHALTIC CONCRETE, TAR 	MACADAM, OR BITUMEN BOUND MACADAM 	(INCLUDING ANY OF THESE SURFACE TYPES WITH 	CONCRETE ENDS).
# BRI	BRICK, LAID OR MORTARED.
# CON	CONCRETE.
# COP	COMPOSITE, 50 PERCENT OR MORE 		OF THE RUNWAY LENGTH IS 			PERMANENT.
# PEM	PART CONCRETE, PART ASPHALT, OR PART 		BITUMEN-BOUND MACADAM.
# PER	PERMANENT, SURFACE TYPE UNKNOWN.
#
#     SOFT(NON-PERMANENT)SURFACE
# BIT	BITUMINOUS, TAR OR ASPHALT MIXED IN 		PLACE, OILED.
# CLA	CLAY
# COM	COMPOSITE, LESS THAN 50 PERCENT OF 		THE RUNWAY LENGTH IS PERMANENT.
# COR	CORAL.
# GRE	GRADED OR ROLLED EARTH, GRASS ON 		GRADED EARTH.
# GRS	GRASS OR EARTH NOT GRADED ORROLLED.
# GVL	GRAVEL.
# ICE	ICE.
# LAT	LATERITE.
# MAC	MACADAM - CRUSHED ROCK WATER BOUND.
# MEM	MEMBRANE - PLASTIC OR OTHER COATED 		FIBER MATERIAL.
# MIX	MIX IN PLACE USING NONBITUMIOUS 			BINDERS SUCH AS PORTLAND CEMENT
# PSP	PIECED STEEL PLANKING.
# SAN	SAND, GRADED, ROLLED OR OILED.
# SNO	SNOW.
# U	SURFACE UNKNOWN.


           if( $runway_surface eq 'ASP' || $runway_surface eq 'BRI' ||
               $runway_surface eq 'CON' || $runway_surface eq 'COP' ||
               $runway_surface eq 'PEM' || $runway_surface eq 'PER')
             {
               # befestigt
               $runway_surface = 2;
             }
           elsif( $runway_surface eq 'U')
             {
               # Unbekannt
               $runway_surface = 0;
             }
           else
             {
               # unbefestigt
               $runway_surface = 1;
             }


             print OUT_AIRPORT "RUNWAY $runway_direction $runway_length $runway_surface\n";

         }

        print OUT_AIRPORT "[END]\n";
      }

  }
