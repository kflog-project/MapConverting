#! /usr/bin/tclsh
#
#


################################################################################
##
## Einstellen des Bereiches und der Quell-Dateien
##

## Grass-Raster-Karte, die gekachelt werden soll
set aktuelleKachel "90S_90E"

## Raster-Karte mit 0m-Werten, mit der $aktuelleKachel verschnitten wird
## Sie muss größer sein, als $aktuelleKachel
set aktuelleNullKachel "null_90S_90E"

## Bereich der Breitengrade ($rowMax muss 2° unterhalb der Grenze enden)
set rowMin -90
set rowMax -52

## Bereich der Längengrade ($colMax muss 2° vor der Grenze enden)
set colMin 90
set colMax 178

##
##
################################################################################

if {[lsearch [array names env] "LOCATION"] == -1} {
    puts "\$LOCATION nicht definiert"
    # Mit Fehlercode beenden
    exit 1
}

if {[lsearch [array names env] "LOC_TARGET"] == -1} {
    puts "\$LOC_TARGET nicht definiert"
    # Mit Fehlercode beenden
    exit 1
}

set locationPath $env(LOCATION)
set locTargetPath $env(LOC_TARGET)


########
##
## Kachelung westlich 0° und östlich 0° muss getrennt ablaufen, ebenso nördlich und südlich
## des Äquators. Das Erweitern der Elemente klappt bei Kacheln an 0° Nord bzw. 0° Ost nicht !!!
##
########

set row $rowMin

set levelList {10 25 50 75 100 150 200 250 300 350 400 450 500 600 700 800 900 1000 \
 	       1250 1500 1750 2000 2250 2500 2750 3000 3250 3500 3750 4000 \
 	       4250 4500 4750 5000 5250 5500 5750 6000 6250 6500 6750 7000 \
 	       7250 7500 7750 8000 8250 8500 8750}

set logFileName "/dev/null"

set logFile [open $logFileName w]

while {$row <= $rowMax} {
    set col $colMin

    while {$col <= $colMax} {
	##
	## Nord-/Südrand des Ausschnitts:
	if {$row >= 0} {
	    ## Nordhalbkugel
	    if {$row > 87} {
		## Ausschnitt berührt den Nordpol
		set nord "90:00:00N"
		set nordRand "90:00:00N"
	    } else {
		set nord "[expr $row + 2]:03:00N"
		set nordRand "[expr $row + 2]:05:00N"
	    }

	    if {$row > 1} {
		set sued "[expr $row - 1]:57:00N"
		set suedRand "[expr $row - 1]:55:00N"
	    } else {
		## Ausschnitt berührt den Äquator
		set sued "00:03:00S"
		set suedRand "00:05:00S"
	    }
	} else {
	    ## Südhalbkugel
	    if {$row > -3} {
		## Ausschnitt berührt den Äquator
		set nord "00:03:00N"
		set nordRand "00:05:00N"
	    } else {
		set nord "[expr ( -1 * $row ) - 3]:57:00S"
		set nordRand "[expr ( -1 * $row ) - 3]:55:00S"
	    }

	    if {$row < -89} {
		## Ausschnitt berührt den Südpol
		set sued "89:59:30S"
		set suedRand "90:00:00S"
	    } else {
		set sued "[expr ( -1 * $row ) ]:03:00S"
		set suedRand "[expr ( -1 * $row ) ]:05:00S"
	    }
	}

	##
	## Ostrand des Ausschnitts:
	if {$col >= 0} {
	    ## Östlich 0°
	    if {$col > 177} {
		## 178°E bis 180°E
		set ost "179:57:00W"
		set ostRand "179:55:00W"
	    } else {
		set ost "[expr $col + 2]:03:00E"
		set ostRand "[expr $col + 2]:05E"
	    }

	    if {$col > 0} {
		## Östlich 2°E
		set west "[expr $col - 1]:57:00E"
		set westRand "[expr $col - 1]:55E"
	    } else {
		set west "00:03:00W"
		set westRand "00:05:00W"
	    }
	} else {
	    ## Westlich 0°
	    if {$col < -3} {
		set ost "[expr ( -1 * $col ) - 3 ]:57:00W"
		set ostRand "[expr ( -1 * $col ) - 3 ]:55:00W"
	    } else {
		## 0°W bis 2°W
		set ost "00:03:00E"
		set ostRand "00:05:00E"
	    }

	    if {$col > -180} {
		set west "[expr ( -1 * $col ) ]:03:00W"
		set westRand "[expr ( -1 * $col ) ]:05:00W"
	    } else {
		## 0°W bis 2°W
		set west "179:57:00E"
		set westRand "179:55:00E"
	    }
	}

	## Westlich 2°W
	#set west "[expr $col + 2]:03:00W"
	#set ost "[expr $col - 1]:57:00W"
	## 0°W bis 2°W
	#set west "02:03W"
	#set ost "00:03E"

	## Östlich 2°E
	## Westlich 2°W
	#set westRand "[expr $col + 2]:10W"
	#set ostRand "[expr $col - 1]:50W"
	## 0°W bis 2°W
	#set westRand "02:10W"
	#set ostRand "00:10E"
	## 0°E bis 2°E

	## Setzen der Region:
	exec g.region n=$nord s=$sued w=$west e=$ost

	## Rasterkarte erzeugen
	catch { exec r.mapcalc neueKachel = $aktuelleKachel }

	## Vergrößern der Region um einen Rand von 1min.
	exec g.region n=$nordRand s=$suedRand w=$westRand e=$ostRand

	## Rasterkarte erzeugen
	exec r.patch -q input=neueKachel,$aktuelleNullKachel output=gesamtKachel

	exec d.erase color=aqua
	catch { exec d.rast -o map=gesamtKachel }

	if {$row < 0} {
	    set rowName [expr -1 * $row]
	    set latName "S_"
	} else {
	    set rowName $row
	    set latName "N_"
	}

	if {$col < 0} {
	    set colName [expr -1 * $col]
	    set lonName "W"
	} else {
	    set colName $col
	    set lonName "E"
	}

	## Höhenlinien erzeugen und exportieren
	set dirName "[expr $rowName]$latName[expr $colName]$lonName"

	catch { exec mkdir "$locTargetPath/$dirName" }

	set rangeFileName "$locationPath/cell_misc/neueKachel/range"

	if ![file exists $rangeFileName] {
	    puts "Wertebereich konnte nicht ermittelt werden:\n    $rangeFileName"
	    # Mit Fehlercode beenden
	    exit 1
	}

	set rangeFile [open $rangeFileName r]
	gets $rangeFile zeile
	set minLevel [lindex $zeile 0]
	set maxLevel [lindex $zeile 1]

	close $rangeFile

	if {$minLevel == ""} { set minLevel 0 }
	if {$maxLevel == ""} { set maxLevel 0 }

	set trueMinLevel $minLevel

	## Was ist die kleinste Höhenstufe, die kleiner als $trueMinLevel ist ?
	foreach height $levelList {
	    if {$height < $trueMinLevel} {
		set minLevel $height
	    } else {
		break
	    }
	}

	puts [format "Kachel: %.2d°00'00\" <-> %.2d°00'00\" / %.3d°00'00\" <-> %.3d°00'00\"  Höhenlinien: %4d m -> %5d m (%4d m)" \
		  $row [expr $row + 2] $col [expr $col + 2] $minLevel $maxLevel $trueMinLevel]

 	foreach height $levelList {
 	    if {$height <= $maxLevel && $height >= $minLevel} {
 		set outFileName "$dirName.$height"

 		#puts $logFile "Erzeuge Höhenlinie $height"
 		flush $logFile
 		## Ermitteln der Höhenlinien (Fehlermeldungen werden umgeleitet)
		exec r.contour -q input=gesamtKachel output=$outFileName levels=$height 2>> $logFileName
# 		exec r.contour -q input=neueKachel output=$outFileName levels=$height 2>> $logFileName

		exec d.vect color=brown map=$outFileName

 		## Exportieren der Höhenlinien
 		exec v.out.ascii input=$outFileName   output=$outFileName

		## Konvertieren zu KFLog-Karten
		catch { exec converter input=$outFileName mode=5 N=[expr $rowName + 2] S=$rowName \
			    W=$colName E=[expr $colName + 2] }

		## Löschen der Vektor-Datei
		catch { exec g.remove vect=$outFileName }

 		## Konvertieren der Datei
		#exec priv.isohyps.convert input=$outFileName
		if {$height == 0} {
		    exec mv /data/KartenDaten/KFLog-Karten/$outFileName.out /data/KartenDaten/KFLog-Karten/grenzen_gekachelt/
		} else {
		    exec mv /data/KartenDaten/KFLog-Karten/$outFileName.out /data/KartenDaten/KFLog-Karten/$dirName/
		}
 	    }
 	}

	## Jetzt ist für jede Höhenlinie und Kachel eine *.out-Datei erzeugt worden.
	## Die werden nun in je eine Datei pro Kachel zusammengefügt
	catch { eval exec cat [glob /data/KartenDaten/KFLog-Karten/$dirName/*.out] > /data/KartenDaten/KFLog-Karten/höhenlinien_gekachelt/$dirName.out }

	## Anschliessend werden KFLog-Binär-Karten erzeugt
	puts $dirName
	catch { exec iso-bin $dirName }

	incr col 2
    }

    incr row 2
}

#puts "$row / $col"
exit

# Rasterkarte erzeugen
exec r.mapcalc neueKachel = ausgangskarte

# Region vergrößern
exec g.region.sh n=$nord s=$sued w=$west e=$ost

# Rasterkarte erzeugen
exec r.mapcalc gesamtKachel = (neueKachel + hintergrund)

# Höhenlinien erzeugen

## Ermitteln der Höhenlinien (Fehlermeldungen werden umgeleitet)
exec r.contour -q input=$baseFileName output=$outFileName levels=$height 2>> $logFileName

## Exportieren der Höhenlinien
exec v.out.ascii input=$outFileName   output=$outFileName
