#
# Fichier : satel.tcl
# Description : Outil pour calculer les positions precises de satellites avec les TLE
# Auteur : Alain KLOTZ
# Mise à jour $Id: satel.tcl,v 1.7 2010-09-04 21:34:36 alainklotz Exp $
#
# source satel.tcl
# utiliser le temps UTC
#
# --- Pour telecharger les TLEs
# satel_update
# --- Pour calculer une position d'un satellite
# satel_coords "jason 2" 2010-05-23T20:12:31
# --- Pour rechercher un satellite au voisinage d'une position
# satel_nearest_radec 22h07m34s25 +60d17m33s0 2010-05-23T20:12:31
#
# source satel.tcl ; satel_coords "iridium 82" 2010-05-23T20:01:07
# "JASON 2 (OSTM)" 22h07m34s25 +60d17m33s0 J2000.0 1.0000 20.47 +20.45
# source satel.tcl ; satel_nearest_radec 22h07m34s25 +60d17m33s0 2010-05-23T20:12:31

proc satel_nearest_radec { ra dec {date now} {home ""} } {
   set sepanglemin 360
   set satelnames [satel_names]
   set ra [mc_angle2deg $ra]
   set dec [mc_angle2deg $dec 90]
   set nsat [llength $satelnames]
   set k 0
   set ksat 0
   foreach satelname $satelnames {
      if {$k==10} {
         if {[info exists resmin]==0} {
            catch {::console::affiche_resultat "$ksat / $nsat\n"}
         } else {
            catch {::console::affiche_resultat "$ksat / $nsat sepmin=$sepanglemin [string trim [lindex [lindex $resmin 0] 0]]\n"}
         }
         set k 0
      }
      incr k
      incr ksat
      set satname [lindex $satelname 0]
      set ficname [lindex $satelname 1]
      #::console::affiche_resultat "$ksat, satel_ephem \"$satname\" $date $home\n"
      set err [catch {satel_ephem \"$satname\" $date $home} res]
      #::console::affiche_resultat "$ksat, err=$err res=$res\n"
      if {$err==1} {
         continue
      }
      if {$res==""} {
         continue
      }
      set res [lindex $res 0]
      if {[info exists resmin]==0} {
         set resmin $res
      }
      set name [string trim [lindex [lindex $res 0] 0]]
      set rasat [lindex $res 1]
      set decsat [lindex $res 2]
      set ill [lindex $res 6]
      set azim [lindex $res 8]
      set gise [expr $azim+180]
      if {$gise>360} {
         set gise [expr $gise-360]
      }
      set elev [lindex $res 9]
      set err [catch {mc_sepangle $ra $dec $rasat $decsat} resang]
      if {$err==1} {
         continue
      }
      set sepangle [lindex $resang 0]
      #::console::affiche_resultat "       sepangle=$sepangle sepanglemin=$sepanglemin\n"
      if {$sepangle<$sepanglemin} {
         set resmin $res
         set sepanglemin $sepangle
      }
   }
   set name [string trim [lindex [lindex $resmin 0] 0]]
   set rasat [mc_angle2hms [lindex $resmin 1] 360 zero 2 auto string]
   set decsat [mc_angle2dms [lindex $resmin 2] 90 zero 1 + string]
   set ill [lindex $resmin 6]
   set azim [lindex $resmin 8]
   set gise [expr $azim+180]
   if {$gise>360} {
      set gise [expr $gise-360]
   }
   set elev [lindex $resmin 9]
   set res "$sepanglemin \"$name\" $ra $dec J2000.0 $ill [format %.5f $gise] [format %+.5f $elev]\n"
   return $res
}

# source "$audace(rep_install)/gui/audace/satel.tcl" ; satel_transit ISS sun now 10
proc satel_transit { satelname objename date1 dayrange {home ""} } {
   global audace
   if {$home==""} {
      set home $::audace(posobs,observateur,gps)
   }
   set date $date1
   set res [satel_ephem $satelname $date $home]
   if {$res==""} {
      error "$res"
   }
   # --- calcule le temps revolution synodique
   set satinfo [satel_names $satelname]
   set name [lindex [lindex $satinfo 0] 0]
   set tle [lindex [lindex $satinfo 0] 1]
   set satfile [ file join $::audace(rep_userCatalog) tle $tle ]
   set f [open $satfile r]
   set lignes [split [read $f] \n]
   set n [llength $lignes]
   set tle1 ""
   set tle2 ""
   for {set k 0} {$k<$n} {incr k} {
      set ligne [lindex $lignes $k]
      set nam [string trim $ligne]
      if {$nam!=$name} {
         continue
      }
      set tle1 [lindex $lignes [expr $k+1]]
      set tle2 [lindex $lignes [expr $k+2]]
   }
   ::console::affiche_resultat "tle1=$tle1\n"
   ::console::affiche_resultat "tle2=$tle2\n"
   set incl [lindex $tle2 2]
   set revperday [lindex $tle2 7]
   set daymin 1436.
   set tsat [expr $daymin/$revperday]
   set tter $daymin
   if {$incl<90} {
      set sign -1
   } else {
      set sign 1
   }
   set tsyn [expr 1./(1./$tsat+1.*$sign/$tter)/1440.]
   ::console::affiche_resultat "revperday=$revperday $tsyn $incl\n"
   # ---- recherche la premiere conjonction satel-sun
   set sun_conjonctions ""
   set sun_transits ""
   set date1 [mc_date2jd $date1]
   set date11 [mc_date2jd $date1]
   set date22 [expr $date1+$dayrange]
   set ddate1 [expr $tsyn*1.1]
   set supersortie 0
   while {$supersortie==0} {   
      set date2 [expr $date1+$ddate1]
      for {set k 0} {$k<10} {incr k} {
         set date $date1
         set range [expr $date2-$date1]
         set dt [expr $range/10.]
         set sortie 0
         set datemin $date1
         set sepmin 360.
         set sepmax 0.
         #::console::affiche_resultat "----------------- $k\n[mc_date2iso8601 $date1] [mc_date2iso8601 $date2] [expr $dt*1440]\n"
         while {$sortie==0} {
            set res [mc_ephem $objename $date {ra dec altitude} -topo $home]
            set res_sun  [lindex $res 0]
            set ra_sun [lindex $res_sun 0]
            set dec_sun [lindex $res_sun 1]
            set elev_sun [lindex $res_sun 2]
            #::console::affiche_resultat "satel_ephem $satelname $date $home\n"
            set res [satel_ephem $satelname $date $home]
            #::console::affiche_resultat "OK \n"
            set res [lindex $res 0]
            set name [string trim [lindex [lindex $res 0] 0]]
            set ra [lindex $res 1]
            set dec [lindex $res 2]
            set elev [lindex $res 9]
            set sepangle_sun  [lindex [mc_sepangle $ra $dec $ra_sun $dec_sun] 0]
            #::console::affiche_resultat "[mc_date2iso8601 $date] $sepangle_sun $sepmin\n"
            if {$sepangle_sun<$sepmin} {
               set sepmin $sepangle_sun
               set datemin $date
               set elevmin $elev
            }
            if {$sepangle_sun>$sepmax} {
               set sepmax $sepangle_sun
            }
            #::console::affiche_resultat "A date=$date [mc_date2iso8601 $date]\n"
            set date [mc_datescomp $date + $dt]
            #::console::affiche_resultat "B date=$date [mc_date2iso8601 $date]\n"
            if {$date>$date2} {
               set sortie 1
               break
            }
         }
         #::console::affiche_resultat "*** [mc_date2iso8601 $datemin] $sepmin\n"
         set date1 [expr $datemin-2*$dt]
         set date2 [expr $datemin+2*$dt]
         set dsep [expr $sepmax-$sepmin]
         if {$dsep<0.5} {
            set sortie 2
            break
         }
         if {$dt<[expr 1./86400]} {
            set sortie 22
            break
         }
      }
      if {($sepmin<1.)&&($elevmin>0)} {
         lappend sun_transits "$datemin $sepmin $elevmin"
      }
      append sun_conjonctions "[mc_date2iso8601 $datemin] $sepmin ($elevmin)\n"
      ::console::affiche_resultat "Conjonction [mc_date2iso8601 $datemin] $sepmin ($elevmin)\n"
      set date1 [expr $datemin+$tsyn]
      set ddate1 [expr $tsyn*0.1]
      if {$datemin>$date22} {
         set sortie 3
         break
      }
   }
   return [list $sun_transits $sun_conjonctions]
}

proc satel_coords { {satelname "ISS"} {date now} {home ""} } {
   set res [satel_ephem $satelname $date $home]
   if {$res==""} {
      error "$res"
   }
   set res [lindex $res 0]
   set name [string trim [lindex [lindex $res 0] 0]]
   set ra [mc_angle2hms [lindex $res 1] 360 zero 2 auto string]
   set dec [mc_angle2dms [lindex $res 2] 90 zero 1 + string]
   set ill [lindex $res 6]
   set azim [lindex $res 8]
   set gise [expr $azim+180]
   if {$gise>360} {
      set gise [expr $gise-360]
   }
   set elev [lindex $res 9]
   set res "\"$name\" $ra $dec J2000.0 $ill [format %.5f $gise] [format %+.5f $elev]\n"
   return $res
}

proc satel_ephem { {satelname "ISS"} {date now} {home ""} } {
   set res [lindex [satel_names \"$satelname\" 1] 0]
   if {$res==""} {
      error "Satellite \"$satelname\" not found in current TLEs."
   }
   set satname [lindex $res 0]
   set satfile [ file join $::audace(rep_userCatalog) tle [lindex $res 1] ]
   set datfile [ file mtime [ file join $::audace(rep_userCatalog) tle [lindex $res 1] ] ]
   set dt [expr ([clock seconds]-$datfile)*86400]
   #::console::affiche_resultat "Update = $dt jours\n"
   if {$home==""} {
      set home $::audace(posobs,observateur,gps)
   }
   #::console::affiche_resultat "mc_tle2ephem $date \"$satfile\" $home -name \"$satname\" -sgp 4\n"
   set res [mc_tle2ephem $date $satfile $home -name $satname -sgp 4 ] ; # -coord {ra dec}
   return $res
}

# Return the list of NAMES+FILE for a given satelname
proc satel_names { {satelname ""} {nbmax ""} } {
   set tlefiles [ glob -nocomplain [ file join $::audace(rep_userCatalog) tle *.txt ] ]
   set texte ""
   set nsat 0
   if {$nbmax==""} {
      set nbmax 100000
   }
   set satelname [string trim [string trim [string toupper $satelname] \"]]
   foreach tlefile $tlefiles {
      set f [open $tlefile r]
      set lignes [split [read $f] \n]
      close $f
      set k 0
      foreach ligne $lignes {
         if {[string length $ligne]>2} {
            if {$k==0} {
               set name [string trim $ligne]
               if {$satelname!=""} {
                  set k [string first $satelname $name]
                  #::console::affiche_resultat "k=$k\n"
                  #dddd
               } else {
                  set k 0
               }
               if {($k>=0)&&($nsat<=$nbmax)} {
                  lappend texte [list $name [file tail $tlefile]]
                  incr nsat
               }
            }
            incr k
            if {$k==3} {
               set k 0
            }
         }
      }
   }
   return $texte
}

# Return all TLE filenames stored in AudeLA
proc satel_tlefiles { } {
   set tlefiles [ glob -nocomplain [ file join $::audace(rep_userCatalog) tle *.txt ] ]
   set texte ""
   foreach tlefile $tlefiles {
      append texte "[file tail $tlefile] "
   }
   return $texte
}

# Update TLE files in AudeLA
proc satel_update { {server celestrack} } {
   set t0 [clock seconds]
   if {$server=="celestrack"} {
      set elemfiles { amateur.txt classfd.txt cubesat.txt dmc.txt education.txt engineering.txt geo.txt geodetic.txt glo-ops.txt globalstar.txt goes.txt gorizont.txt gps-ops.txt intelsat.txt iridium.txt military.txt molniya.txt musson.txt nnss.txt noaa.txt orbcomm.txt other-comm.txt other.txt radar.txt raduga.txt resource.txt sarsat.txt science.txt stations.txt tdrss.txt tle-new.txt visual.txt weather.txt x-comm.txt }
      #set elemfiles { amateur.txt classfd.txt }
      set ntot 0
      foreach elemfile $elemfiles {
         set url "http://celestrak.com/NORAD/elements/$elemfile"
         catch {::console::affiche_resultat "Download $url\n"}
         set err [catch {satel_download $url} msg]
         if {$err==1} {
            catch {::console::affiche_resultat " Problem: $msg.\n"}
         } else {
            set texte ""
            set n 0
            set lignes [split $msg \n]
            foreach ligne $lignes {
               if {[string length $ligne]>2} {
                  append texte "$ligne\n"
               }
               incr n
            }
            set n [expr $n/3]
            incr ntot $n
            file mkdir [ file join $::audace(rep_userCatalog) tle ]
            set err [catch {
               set fic [ file join $::audace(rep_userCatalog) tle $elemfile ]
               set f [open $fic w]
               puts -nonewline $f $texte
               close $f
            } msg]
            if {$err==1} {
               catch {::console::affiche_resultat " Problem: $msg.\n"}
            } else {
               catch {::console::affiche_resultat " $n satellites in $elemfile\n"}
            }
         }
      }
      catch {::console::affiche_resultat "A total of $ntot satellites elements are downloaded in [ file join $::audace(rep_userCatalog) tle ]\n"}
   } else {
      error "Server not known. Servers are: celestrack."
   }
   set dt [expr [clock seconds]-$t0]
   catch {::console::affiche_resultat "Done in $dt seconds.\n\n"}
   return $ntot
}

# Download one TLE file and return the contents
proc satel_download { {url http://celestrak.com/NORAD/elements/stations.txt} } {
   set err [catch {
      package require http
      set token [::http::geturl $url]
      upvar #0 $token state
      set html_text $state(body)
   } msg]
   if {$err==0} {
      if {[string first "<!DOCTYPE" $html_text]<0} {
         return $html_text
      } else {
         error "File not found in server"
      }
   } else {
      error $msg
   }
}

