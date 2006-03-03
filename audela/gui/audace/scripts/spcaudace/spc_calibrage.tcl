
# source $audace(rep_scripts)/spcaudace/spc_calibrage.tcl
# spc_fits2dat lmachholz_centre.fit
# buf1 load lmachholz_centre.fit


####################################################################
#  Procedure de calcul de dispersion moyenne
#
# Auteur : Benjamin MAUCLAIRE
# Date creation : 27-02-2005
# Date modification : 27-02-2005
# Arguments : liste des lambdas, naxis1
####################################################################

proc spc_dispersion_moy { { lambdas ""} } {
    # Dispersion du spectre :
    set naxis1 [llength $lambdas]
    set l1 [lindex $lambdas 1]
    set l2 [lindex $lambdas [expr int($naxis1/10)]]
    set l3 [lindex $lambdas [expr int(2*$naxis1/10)]]
    set l4 [lindex $lambdas [expr int(3*$naxis1/10)]]
    set dl1 [expr ($l2-$l1)/(int($naxis1/10)-1)]
    set dl2 [expr ($l4-$l3)/(int($naxis1/10)-1)]
    set xincr [expr 0.5*($dl2+$dl1)]
    return $xincr
}
#****************************************************************#



####################################################################
#  Procedure de conversion d'�talonnage en longueur d'onde d'ordre 2
#
# Auteur : Benjamin MAUCLAIRE
# Date creation : 29-01-2005
# Date modification : 29-01-05 / 09-12-05 / 26-12-05
# Arguments : fichier .fit du profil de raie spatial pixel1 lambda1 pixel2 lambda2
####################################################################

proc spc_calibre2 { args } {

  global conf
  global audace
  global profilspc
  global captionspc

  if {[llength $args] == 5} {
    set filespc [ lindex $args 0 ]
    set pixel1 [ lindex $args 1 ]
    set lambda1 [ lindex $args 2 ]
    set pixel2 [ lindex $args 3 ]
    set lambda2 [ lindex $args 4 ]
    
    #--- R�cup�re la liste "spectre" contenant 2 listes : pixels et intensites
    #set spectre [ openspcncal "$filespc" ]
    #-- Modif faite le 26/12/2005
    set spectre [ spc_fits2data "$filespc" ]
    set intensites [lindex $spectre 0]
    set naxis1 [lindex $spectre 1]

    #--- Calcul des parametres spectraux
    set deltax [expr 1.0*($pixel2-$pixel1)]
    set dispersion [expr 1.0*($lambda2-$lambda1)/$deltax]
    ::console::affiche_resultat "La dispersion vaut : $dispersion Angstroms/pixel\n"
    set lambda0 [expr 1.0*($lambda1-$dispersion*$pixel1)]
    #set xcentre [expr int($lambda0+0.5*($dispersion*$naxis1)-1)]

    #--- Initialisation des mots clefs du fichier fits de sortie
    # setkwd [list "mot-clef" "valeur" [string, int, float] "commentaire" "unite"]
    #buf$audace(bufNo) setkwd [list "NAXIS1" "$naxis1" int "" ""]
    #-- Longueur d'onde de d�part
    buf$audace(bufNo) setkwd [list "CRVAL1" "$lambda0" float "" "Angstrom"]
    #-- Dispersion
    buf$audace(bufNo) setkwd [list "CDELT1" "$dispersion" float "" "Angstrom/pixel"]
    #-- Longueur d'onde centrale FAUX
    #buf$audace(bufNo) setkwd [list "CRPIX1" "$xcentre" int "" "Angstrom"]
    #-- Corrdonn�e repr�sent�e sur l'axe 1 (ie X)
    buf$audace(bufNo) setkwd [list "CTYPE1" "Wavelength" string "" ""]

    buf$audace(bufNo) save $audace(rep_images)/l${filespc}
    ::console::affiche_resultat "Spectre �talonn� sauv� sous l${filespc}.\n"
  } else {
    ::console::affiche_erreur "Usage: spc_calibre2 fichier_fits_du_profil x1 lambda1 x2 lambda2\n\n"
  }
}
#****************************************************************#



####################################################################
#  Procedure de conversion d'�talonnage en longueur d'onde d'ordre 2
#
# Auteur : Benjamin MAUCLAIRE
# Date creation : 29-01-2005
# Date modification : 29-01-05 / 09-12-05 / 26-12-05
# Arguments : fichier .fit du profil de raie spatial pixel1 lambda1 pixel2 lambda2
####################################################################

proc spc_calibre2c { args } {

  global conf
  global audace
  global profilspc
  global captionspc

  if {[llength $args] == 8} {
    set filespc [ lindex $args 0 ]
    set pixel1a [ lindex $args 1 ]
    set pixel1b [ lindex $args 2 ]
    set lambda1 [ lindex $args 3 ]
    set pixel2a [ lindex $args 4 ]
    set pixel2b [ lindex $args 5 ]
    set lambda2 [ lindex $args 6 ]
    set typeraie [ lindex $args 7 ]
    
    #--- R�cup�re la liste "spectre" contenant 2 listes : pixels et intensites
    #set spectre [ openspcncal "$filespc" ]
    #-- Modif faite le 26/12/2005
    set spectre [ spc_fits2data "$filespc" ]
    set intensites [lindex $spectre 0]
    set naxis1 [lindex $spectre 1]

    #-- D�termine le centre gaussien de la raie 1 et 2
    buf$audace(bufNo) load "$audace(rep_images)/${filespc}"
      if { $typeraie == "a" } {
	  buf$audace(bufNo) mult -1
      }
    set listcoords [list $pixel1a 1 $pixel1b 1]
    set pixel1 [lindex [ buf$audace(bufNo) fitgauss $listcoords ] 1]
    set listcoords [list $pixel2a 1 $pixel2b 1]
    set pixel2 [lindex [ buf$audace(bufNo) fitgauss $listcoords ] 1] 
      ::console::affiche_resultat "Centre des raies 1 : $pixel1 et raie 2 : $pixel2\n"
      #-- Redresse le spectre a l'endroit s'il avait ete invers� pr�c�dement
      if { $typeraie == "a" } {
	  buf$audace(bufNo) mult -1
      }

    #--- Calcul des parametres spectraux
    set deltax [expr 1.0*($pixel2-$pixel1)]
    set dispersion [expr 1.0*($lambda2-$lambda1)/$deltax]
    ::console::affiche_resultat "La dispersion vaut : $dispersion Angstroms/pixel\n"
    set lambda0 [expr 1.0*($lambda1-$dispersion*$pixel1)]
    #set xcentre [expr int($lambda0+0.5*($dispersion*$naxis1)-1)]

    #--- Initialisation des mots clefs du fichier fits de sortie
    # setkwd [list "mot-clef" "valeur" [string, int, float] "commentaire" "unite"]
    #buf$audace(bufNo) setkwd [list "NAXIS1" "$naxis1" int "" ""]
    #-- Longueur d'onde de d�part
    buf$audace(bufNo) setkwd [list "CRVAL1" "$lambda0" float "" "Angstrom"]
    #-- Dispersion
    buf$audace(bufNo) setkwd [list "CDELT1" "$dispersion" float "" "Angstrom/pixel"]
    #-- Longueur d'onde centrale FAUX
    #buf$audace(bufNo) setkwd [list "CRPIX1" "$xcentre" int "" "Angstrom"]
    #-- Corrdonn�e repr�sent�e sur l'axe 1 (ie X)
    buf$audace(bufNo) setkwd [list "CTYPE1" "Wavelength" string "" ""]

    buf$audace(bufNo) save $audace(rep_images)/l${filespc}
    ::console::affiche_resultat "Spectre �talonn� sauv� sous l${filespc}.\n"
  } else {
    ::console::affiche_erreur "Usage: spc_calibre2 fichier_fits_du_profil x1 lambda1 x2 lambda2\n\n"
  }
}
#****************************************************************#


####################################################################
#  Procedure de conversion d'�talonnage en longueur d'onde d'ordre 2
#
# Auteur : Benjamin MAUCLAIRE
# Date creation : 29-01-2005
# Date modification : 29-01-2005 / 09-12-2005
# Arguments : fichier .fit du profil de raie spatial
####################################################################

proc spc_calibre3 { args } {

  global conf
  global audace
  global profilspc
  global captionspc

  if {[llength $args] == 7} {
    set filespc [ lindex $args 0 ]
    set pixel1 [ lindex $args 1 ]
    set lambda1 [ lindex $args 2 ]
    set pixel2 [ lindex $args 3 ]
    set lambda2 [ lindex $args 4 ]
    set pixel3 [ lindex $args 5 ]
    set lambda3 [ lindex $args 6 ]

    # R�cup�re la liste "spectre" contenant 2 listes : pixels et intensites
    #-- Modif faite le 26/12/2005
    set spectre [ spc_fits2data "$filespc" ]
    set intensites [lindex $spectre 0]
    set naxis1 [lindex $spectre 1]

    # Calcul des parametres spectraux
    set deltax [expr $x2-$x1]
    set dispersion [expr ($lambda2-$lambda1)/$deltax]
    ::console::affiche_resultat "La dispersion lin�aire vaut : $dispersion Angstroms/Pixel.\n"
    set lambda_0 [expr $lambda1-$dispersion*$x1]

    # Calcul les coefficients du polyn�me interpolateur de Lagrange : lambda=a*x^2+b*x+c
    set a [expr $lambda1/(($x1-$x2)*($x1-$x2))+$lambda2/(($x2-$x1)*($x2-$x3))+$lambda3/(($x3-$x1)*($x3-$x2))]
    set b [expr -$lambda1*($x3+$x2)/(($x1-$x2)*($x1-$x2))-$lambda2*($x3+$x1)/(($x2-$x1)*($x2-$x3))-$lambda3*($x1+$x2)/(($x3-$x1)*($x3-$x2))]
    set c [expr $lambda1*$x3*$x2/(($x1-$x2)*($x1-$x2))+$lambda2*$x3*$x1/(($x2-$x1)*($x2-$x3))+$lambda3*$x1*$x2/(($x3-$x1)*($x3-$x2))]
    ::console::affiche_resultat "$a, $b et $c\n"

    # set dispersionm [expr (sqrt(abs($b^2-4*$a*$c)))/$a]
    #set dispersionm [expr abs([ dispersion_moy $intensites $naxis1 ]) ]
    # Calcul les valeurs des longueurs d'ondes associees a chaque pixel
    set len [expr $naxis1-2]
    for {set x 1} {$x<=$len} {incr x} {
	lappend lambdas [expr $a*$x*$x+$b*$x+$c]
    }

    # Affichage du polynome :
    set file_id [open "$audace(rep_images)/polynome.txt" w+]
    for {set x 1} {$x<=$len} {incr x} {
	set lamb [lindex $lambdas [expr $x-1]]
	puts $file_id "$x $lamb"
    }
    close $file_id

    # Calcul la disersion moyenne en faisant la moyenne des ecarts entre les lambdas : GOOD ! 
    set dispersionm 0
    for {set k 0} {$k<[expr $len-1]} {incr k} {
	set l1 [lindex $lambdas $k]
	set l2 [lindex $lambdas [expr $k+1]]
	set dispersionm [expr 0.5*($dispersionm+0.5*($l2-$l1))]
    }
    ::console::affiche_resultat "La dispersion non lin�aire vaut : $dispersionm Angstroms/Pixel.\n"

    set lambda0 [expr $a+$b+$c]
    set lcentre [expr int($lambda0+0.5*($dispersionm*$naxis1)-1)]

    # Initialisation des mots clefs du fichier fits de sortie
    # setkwd [list "mot-clef" "valeur" [string, int, float] "commentaire" "unite"]
    #buf$audace(bufNo) setkwd [list "NAXIS1" "$naxis1" int "" ""]
    # Longueur d'onde de d�part
    buf$audace(bufNo) setkwd [list "CRVAL1" "$lambda0" float "" "Angstrom"]
    # Dispersion
    #buf$audace(bufNo) setkwd [list "CDELT1" "$dispersionm" float "" "Angtrom/pixel"]
    buf$audace(bufNo) setkwd [list "CDELT1" "$dispersion" float "" "Angtrom/pixel"]
    # Longueur d'onde centrale
    buf$audace(bufNo) setkwd [list "CRPIX1" "$lcentre" int "" "Angstrom"]
    # Type de dispersion : LINEAR...
    buf$audace(bufNo) setkwd [list "CTYPE1" "NONLINEAR" string "" ""]

    buf$audace(bufNo) save $audace(rep_images)/l${filespc}
    ::console::affiche_resultat "Spectre �talonn� souv� sous l${filespc}.\n"
  } else {
    ::console::affiche_erreur "Usage: spc_calibre2 fichier_fits_du_profil x1 lambda1 x2 lambda2 x3 lambda3\n\n"
  }
}
#****************************************************************************


####################################################################
#  Procedure d'�talonnage en longueur d'onde � partir de la dispersion et d'une raie
#
# Auteur : Benjamin MAUCLAIRE
# Date creation : 16-08-2005
# Date modification : 16-08-2005
# Arguments : profil de raie.fit, pixel, lambda, dispersion
####################################################################

proc spc_calibred { args } {

  global conf
  global audace
  global profilspc
  global captionspc

  if {[llength $args] == 4} {
    set filespc [ lindex $args 0 ]
    set pixel1 [ lindex $args 1 ]
    set lambda1 [ lindex $args 2 ]
    set dispersion [ lindex $args 3 ]

    # R�cup�re la liste "spectre" contenant 2 listes : pixels et intensites
    set spectre [ openspcncal $filespc ]
    set intensites [lindex $spectre 0]
    #set naxis1 [lindex $spectre 1]
    buf$audace(bufNo) load $audace(rep_images)/$filespc
    set naxis1 [lindex [buf$audace(bufNo) getkwd "NAXIS1"] 1]
      ::console::affiche_resultat "$naxis1\n"
      
    # Calcul des parametres spectraux
    set lambda0 [expr $lambda1-$dispersion*$pixel1]
    set xcentre [expr int($lambda0+0.5*($dispersion*$naxis1)-1.0)]

    # Initialisation des mots clefs du fichier fits de sortie
    # setkwd [list "mot-clef" "valeur" [string, int, float] "commentaire" "unite"]
    #buf$audace(bufNo) setkwd [list "NAXIS1" "$naxis1" int "" ""]
    # Longueur d'onde de d�part
    buf$audace(bufNo) setkwd [list "CRVAL1" "$lambda0" int "" "Angstrom"]
    # Dispersion
    buf$audace(bufNo) setkwd [list "CDELT1" "$dispersion" float "" "Angtrom/pixel"]
    # Longueur d'onde centrale
    buf$audace(bufNo) setkwd [list "CRPIX1" "$xcentre" int "" "Angstrom"]
    # Type de dispersion : LINEAR...
    buf$audace(bufNo) setkwd [list "CTYPE1" "LINEAR" string "" ""]

    #buf$audace(bufNo) save $audace(rep_images)/l${filespc}
    buf$audace(bufNo) save l${filespc}
    ::console::affiche_resultat "Spectre �talonn� souv� sous l${filespc}\n"
  } else {
    ::console::affiche_erreur "Usage: spc_calibre2 fichier_fits_du_profil x1 lambda1 dispersion\n\n"
  }
}
#****************************************************************#


##########################################################
# Calcul la r�ponse intrumentale et l'enregistre
#
# Auteur : Benjamin MAUCLAIRE
# Date de cr�ation : 02-09-2005
# Date de mise � jour : 02-09-2005
# Arguments : fichier .fit du profil de raie, profil de raie de r�f�rence
# Remarque : effectue le d�coupage, r��chantillonnage puis la division 
##########################################################

proc spc_rinstrum { args } {

   global audace
   global conf

   if {[llength $args] == 2} {
       set infichier_mes [ lindex $args 0 ]
       set infichier_ref [ lindex $args 1 ]
       set fichier_mes [ file rootname $infichier_mes ]
       set fichier_ref [ file rootname $infichier_ref ]

       # R�cup�re les caract�ristiques des 2 spectres
       buf$audace(bufNo) load $fichier_mes
       set naxis1a [lindex [buf$audace(bufNo) getkwd "NAXIS1"] 1]
       set xdeb1 [lindex [buf$audace(bufNo) getkwd "CRVAL1"] 1]
       set disper1 [lindex [buf$audace(bufNo) getkwd "CDELT1"] 1]
       set xfin1 [ expr $xdeb1+$naxis1a*$disper1*1.0 ]
       buf$audace(bufNo) load $fichier_ref
       set naxis1b [lindex [buf$audace(bufNo) getkwd "NAXIS1"] 1]
       set xdeb2 [lindex [buf$audace(bufNo) getkwd "CRVAL1"] 1]
       set disper2 [lindex [buf$audace(bufNo) getkwd "CDELT1"] 1]
       set xfin2 [ expr $xdeb2+$naxis1b*$disper2*1.0 ]

       # S�lection de la bande de longueur d'onde du spectre de r�f�rence
       ## Le spectre de r�f�rence est suppos� avoir une plus large bande de lambda
       set ${fichier_ref}_sel [ spc_select $fichier_ref $xdeb1 $xfin1 ]
       # R��chantillonnage du spectre de r�f�rence : c'est un choix.
       ## Que disp1 < disp2 ou disp2 < disp1, la dispersion finale sera disp1
       set ${fichier_ref}_sel_rech [ spc_echant ${fichier_ref}_sel $disp1 ]
       file delete ${fichier_ref}_sel$conf(extension,defaut)
       # Calcul la r�ponse intrumentale : RP=spectre_mesure/spectre_ref
       buf$audace(bufNo) load $fichier_mes
       buf$audace(bufNo) div ${fichier_ref}_sel_rech 1.0
       buf$audace(bufNo) bitpix float
       buf$audace(bufNo) save reponse_intrumentale
       ::console::affiche_resultat "S�lection sauv�e sous ${fichier}_sel$conf(extension,defaut)\n"
       return ${fichier}_sel$conf(extension,defaut)
   } else {
       ::console::affiche_erreur "Usage: spc_rinstrum fichier .fit du profil de raie, profil de raie de r�f�rence\n\n"
   }
}
##########################################################


##########################################################
# Procedure de normalisation de profil de raie
#
# Auteur : Benjamin MAUCLAIRE
# Date de cr�ation : 15-08-2005
# Date de mise � jour : 15-08-2005
# Arguments : fichier .fit du profil de raie, largeur de raie (optionnelle)
##########################################################

proc spc_norma1 { args } {

   global audace
   global conf
   set pourcent 0.95

   if {[llength $args] == 2} {
     set infichier [ lindex $args 0 ]
     set lraie [lindex $args 1 ]
     set fichier [ file rootname $infichier ]
     # buf$audace(bufNo) load "$audace(rep_images)/$fichier"
     buf$audace(bufNo) load $fichier
     buf$audace(bufNo) imaseries "BACK kernel=$lraie threshold=$pourcent"
     buf$audace(bufNo) bitpix float
     buf$audace(bufNo) save ${fichier}_norm$conf(extension,defaut)
     ::console::affiche_resultat "Profil normalis� sauv� sous ${fichier}_norm$conf(extension,defaut)\n"
   } elseif {[llength $args] == 1} {
     set fichier [ lindex $args 0 ]
     set lraie 20
     # buf$audace(bufNo) load "$audace(rep_images)/$fichier"
     buf$audace(bufNo) load $fichier
     buf$audace(bufNo) imaseries "BACK kernel=$lraie threshold=$pourcent div"
     buf$audace(bufNo) bitpix float
     buf$audace(bufNo) save ${fichier}_norm$conf(extension,defaut)
     ::console::affiche_resultat "Profil normalis� sauv� sous ${fichier}_norm$conf(extension,defaut)\n"
   } else {
     ::console::affiche_erreur "Usage : spc_norma nom_fichier ?largeur de raie?\n\n"
   }
}
#*****************************************************************#


####################################################################
# Procedure de normalisation de profil de raie
#
# Auteur : Benjamin MAUCLAIRE
# Date creation : 15-12-2005
# Date modification : 15-12-2005
# Arguments : fichier .fit du profil de raie normalis�
####################################################################

proc spc_autonorma_051215b { args } {

    global audace
    global conf
    set extsp ".dat"

    if {[llength $args] == 1} {
	set fichier [ lindex $args 0 ]
	set nom_fichier [ file rootname $fichier ]
	#--- Ajustement de degr� 2 pour d�ter�iner un continuum
	set coordonnees [spc_ajust $fichier 1]
	set nom_continuum [ spc_data2fits ${nom_fichier}_conti $coordonnees ]

	#set nx [llength [lindex $coordonnees 0]]
	#set ny [llength [lindex $coordonnees 1]]
	#::console::affiche_resultat "Nb points x : $nx ; y : $ny\n"
	
	#--- Normalisation par division
	buf$audace(bufNo) load $audace(rep_images)/$fichier
	buf$audace(bufNo) div $audace(rep_images)/$nom_continuum 1
	#buf$audace(bufNo) bitpix float
	#buf$audace(bufNo) save $audace(rep_images)/${nom_fichier}_norm

	#-- Effacement des fichiers temporaires
	#file delete $audace(rep_images)/${nom_fichier}_continuum$conf(extension,defaut)
    } else {
	::console::affiche_erreur "Usage : spc_autonorma nom_profil_de_raies\n\n"
    }
}

#proc spc_autonorma_151205 { args } 
proc spc_autonorma { args } {

    global audace
    global conf
    set extsp ".dat"

    if {[llength $args] == 1} {
	set fichier [ lindex $args 0 ]
	set nom_fichier [ file rootname $fichier ]
	#::console::affiche_resultat "F : $fichier ; NF : $nom_fichier\n"
	#--- Ajustement de degr� 2 pour d�ter�iner un continuum
	set coordonnees [spc_ajust $fichier 1]
	#-- vspc_data2fits retourne juste le nom de fichier cr��
	set nom_continuum [ spc_data2fits ${nom_fichier}_conti $coordonnees "double" ]

	#--- Retablissemnt d'une dispersion identique entre continuum et le profil a� normaliser
	buf$audace(bufNo) load $audace(rep_images)/$fichier
	set liste_dispersion [buf$audace(bufNo) getkwd "CDELT1"]
	set dispersion [lindex $liste_dispersion 1]
	set nbunit [lindex $liste_dispersion 2]
	#set unite [lindex $liste_dispersion 3]
	buf$audace(bufNo) load $audace(rep_images)/$nom_continuum
	buf$audace(bufNo) setkwd [list "CDELT1" "$dispersion" $nbunit "" "Angstrom/pixel"]
	buf$audace(bufNo) bitpix float
	buf$audace(bufNo) save $audace(rep_images)/$nom_continuum

	#--- Normalisation par division
	buf$audace(bufNo) load $audace(rep_images)/$fichier
	buf$audace(bufNo) div $audace(rep_images)/$nom_continuum 1
	buf$audace(bufNo) bitpix float
	buf$audace(bufNo) save $audace(rep_images)/${nom_fichier}_norm

	#-- Effacement des fichiers temporaires
	#file delete $audace(rep_images)/${nom_fichier}_continuum$conf(extension,defaut)
    } else {
	::console::affiche_erreur "Usage : spc_autonorma nom_profil_de_raies\n\n"
    }
}
#*****************************************************************#

proc spc_autonorma_131205 { args } {

    global audace
    global conf
    set extsp ".dat"

    if {[llength $args] == 1} {
	set fichier [ lindex $args 0 ]

	# Ajustement de degr� 2 pour d�ter�iner un continuum
	set coordonnees [spc_ajust $fichier 1]
	set lambdas [lindex $coordonnees 0]
	set intensites [lindex $coordonnees 1]
	set len [llength $lambdas]

	#--- Enregistrement du continuum au format fits
	set filename [ file rootname $fichier ]
	##set filename ${fileetalonnespc}_dat$extsp
	set fichier_conti ${filename}_conti$extsp
	set file_id [open "$audace(rep_images)/$fichier_conti" w+]
	for {set k 0} {$k<$len} {incr k} {
	    set lambda [lindex $lambdas $k]
	    set intensite [lindex $intensites $k]
	    #--- Ecrit les couples "Lambda Intensite" dans le fichier de sortie
	    puts $file_id "$lambda\t$intensite"
	}
	close $file_id
	#--- Conversion en fits
	spc_dat2fits $fichier_conti
	#-- Bisarrerie : le continuum fits est inverse gauche-droite
	buf$audace(bufNo) load $audace(rep_images)/${filename}_conti_fit
	buf$audace(bufNo) mirrorx
	buf$audace(bufNo) save $audace(rep_images)/${filename}_conti_fit

	#--- Normalisation par division
	buf$audace(bufNo) load $audace(rep_images)/$fichier
	buf$audace(bufNo) div $audace(rep_images)/${filename}_conti_fit 1
	buf$audace(bufNo) save $audace(rep_images)/${filename}_norm

	#-- Effacement des fichiers temporaires
	file delete $audace(rep_images)/$fichier_conti$extsp
	file delete $audace(rep_images)/${filename}_conti_fit$conf(extension,defaut)
    } else {
	::console::affiche_erreur "Usage : spc_autonorma nom_profil_de_raies\n\n"
    }
}
#*****************************************************************#

