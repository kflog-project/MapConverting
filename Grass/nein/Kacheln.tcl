#! /usr/bin/tclsh		
set outFileName [file rootname [lindex $argv 0]].out
set inFileName [lindex $argv 0]

#Öffnen der Dateien
set inFile [ open $inFileName r ]
set outfile [ open "$outFileName" w ]

## fürs Kacheln
set is_in 0
set is_out 0
set w_grenz [expr 8 * 60 * 10000]
set o_grenz [expr $w_grenz + 600 * 10000]
set s_grenz [expr 49 * 60 * 10000 + 30 * 10000]
set n_grenz [expr 50 * 60 * 10000]

set isOpen 0

while {[gets $inFile zeile] >= 0} {
    if {[regexp {^WEST.*} $zeile] || [regexp {^EAST.*} $zeile] ||
	[regexp {^SOUTH.*} $zeile] || [regexp {^NORTH.*} $zeile] || 
	[regexp {^HOEHE.*} $zeile]} {
	puts $outfile "$zeile"
    } elseif {[lindex $zeile 0] == "\[NEW\]"} {
	puts $outfile "$zeile"
	puts $outfile "TYPE=70"
    } elseif {[lindex $zeile 0] == "\[END\]"} {
	puts $outfile "$zeile"
    } else {
	# Die Positionsangabe muss konvertiert werden ...
	#     Die Angaben liegen in Grad vor, gebraucht werden Bogenminuten
	#
#	set latitude [expr round([lindex $zeile 0] * 60 * 10000)]
	#	set longitude [expr round([lindex $zeile 1] * 60 * 10000)]

	set latitude [lindex $zeile 0]
	set longitude [lindex $zeile 1]

	#Kacheln
	#
	#
#	puts "$longitude -- $w_grenz   $o_grenz"
#	puts "$latitude  -- $s_grenz   $n_grenz"
	if {($latitude > $s_grenz && $latitude < $n_grenz) && 
	    ($longitude > $w_grenz && $longitude < $o_grenz)} {

	    if {$is_out == 1} {
		puts $outfile "$latitude_b $longitude_b"
		if {$isOpen == 1} {
		    # Element ist offen
		    puts $oufile "[END]"
		    puts "wurde geschlossen ..."
		    isOpen == 0
		}
		set is_out 0
	    } else {
		puts $outfile "$latitude $longitude"
	    }
#	    puts "ja"
	    set is_in 1
	} else {
	    #Auserhalb der Kachel

	    if {$is_in == 1} {
		puts $outfile "$latitude $longitude"
		set is_in 0
	    }
	    set is_out 1
	}
	set latitude_b $latitude
	set longitude_b $longitude
	
    }
}

# Das letzte Objekt muss in beiden Dateien noch beendet werden
puts "fertig"
#puts $outfile "\[END\]"

# Alles ist korrekt gelaufen :-)
exit 0
