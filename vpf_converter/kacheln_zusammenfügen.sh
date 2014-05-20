#! /usr/bin/tclsh

#
# Script zum Zusammenf�gen von mehreren KFlog Ascii Kacheln
#   vorm konvertieren ins Bin�rformat m�ssen alle Kacheln der einzelnen
#   Element Typen mit diesem Skript zu einer Kachel zusammengef�gt werden
#   
#   inVerzeichnisse z.B. city/ river/ etc.
# 
# <kachelverzeichnis> <outVerzeichnis> <inVerzeichnis> <inVerzeichnis>

if {$argc < 3} {
  puts ""
  puts "Optionen: <kachelDir> <outDir> <inDir_1> <inDir_2> ...."
  puts ""
  puts "Im kachelDir m�ssen alle leere Kacheln stehen!"
  return
}

cd [lindex $argv 0]


foreach el [lsort [glob *.out]] {
  puts $el





  set inListe ""
  foreach inDir [lrange $argv 2 end] {

    if {[file exists $inDir$el]} {
        set inListe "$inListe $inDir$el"
    }
  }

  if {[llength $inListe] > 0} {
    eval exec cat  $inListe > [lindex $argv 1]$el
  }



###### Kacheln leeren
#    eval exec echo "" > $el

}

