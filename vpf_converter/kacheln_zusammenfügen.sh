#! /usr/bin/tclsh

#
# Script zum Zusammenf�gen von mehreren KFlog Ascii Kacheln
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

