#
# Fichier : confcam.tcl
# Description : Affiche la fenetre de configuration des plugins du type 'camera'
# Mise a jour $Id: confcam.tcl,v 1.94 2007-10-19 22:16:34 robertdelmas Exp $
#

namespace eval ::confCam {

   #
   # confCam::init (est lance automatiquement au chargement de ce fichier tcl)
   # Initialise les variables conf(...) et caption(...)
   # Demarre le plugin selectionne par defaut
   #
   proc init { } {
      global audace caption conf confCam

      #--- Charge le fichier caption
      source [ file join $audace(rep_caption) confcam.cap ]

      #--- initConf
      if { ! [ info exists conf(camera,A,camName) ] } { set conf(camera,A,camName) "" }
      if { ! [ info exists conf(camera,A,start) ] }   { set conf(camera,A,start)   "0" }
      if { ! [ info exists conf(camera,B,camName) ] } { set conf(camera,B,camName) "" }
      if { ! [ info exists conf(camera,B,start) ] }   { set conf(camera,B,start)   "0" }
      if { ! [ info exists conf(camera,C,camName) ] } { set conf(camera,C,camName) "" }
      if { ! [ info exists conf(camera,C,start) ] }   { set conf(camera,C,start)   "0" }
      if { ! [ info exists conf(camera,geometry) ] }  { set conf(camera,geometry)  "670x430+25+45" }

      #--- Charge les plugins des cameras
      source [ file join $audace(rep_plugin) camera audine audine.tcl ]
      source [ file join $audace(rep_plugin) camera hisis hisis.tcl ]
      source [ file join $audace(rep_plugin) camera sbig sbig.tcl ]
      source [ file join $audace(rep_plugin) camera cookbook cookbook.tcl ]
      source [ file join $audace(rep_plugin) camera starlight starlight.tcl ]
      source [ file join $audace(rep_plugin) camera kitty kitty.tcl ]
      source [ file join $audace(rep_plugin) camera webcam webcam.tcl ]
      source [ file join $audace(rep_plugin) camera th7852a th7852a.tcl ]
      source [ file join $audace(rep_plugin) camera scr1300xtc scr1300xtc.tcl ]
      source [ file join $audace(rep_plugin) camera dslr dslr.tcl ]
      source [ file join $audace(rep_plugin) camera andor andor.tcl ]
      source [ file join $audace(rep_plugin) camera fingerlakes fingerlakes.tcl ]
      source [ file join $audace(rep_plugin) camera cemes cemes.tcl ]
      source [ file join $audace(rep_plugin) camera coolpix coolpix.tcl ]

      #--- Charge les fichiers auxiliaires
      uplevel #0 "source \"[ file join $audace(rep_plugin) camera audine obtu_pierre.tcl ]\""
      uplevel #0 "source \"[ file join $audace(rep_plugin) camera audine testaudine.tcl ]\""

      #--- Je charge le package Thread si l'option multitread est activive dans le TCL
      if { [info exists ::tcl_platform(threaded)] } {
         if { $::tcl_platform(threaded)==1 } {
            #--- Je charge le package Thread
            #--- La version minimale 2.6.3 pour disposer de la commande thread::copycommand
            if { ! [catch {package require Thread 2.6.3}]} {
               #--- Je redirige les messages d'erreur vers la procedure ::confCam::dispThreadError
               thread::errorproc ::confCam::dispThreadError
            } else {
               set ::tcl_platform(threaded) 0
            }
         }
      } else {
         set ::tcl_platform(threaded) 0
      }

      #--- Initalise le numero de camera a nul
      set audace(camNo) "0"

      #--- Initalise les listes de cameras
      set confCam(labels) [ list Audine Hi-SIS SBIG CB245 Starlight Kitty WebCam \
         TH7852A SCR1300XTC $caption(dslr,camera) Andor FLI Cemes $caption(coolpix,camera) ]
      set confCam(names) [ list audine hisis sbig cookbook starlight kitty webcam \
         th7852a scr1300xtc dslr andor fingerlakes cemes coolpix ]

      #--- Intialise les variables de chaque camera
      for { set i 0 } { $i < [ llength $confCam(names) ] } { incr i } {
         ::[ lindex $confCam(names) $i ]::initPlugin
      }

      #--- Item par defaut
      set confCam(currentCamItem) "A"

      #--- Initialisation des variables d'echange avec les widgets
      set confCam(geometry)     "$conf(camera,geometry)"
      set confCam(A,visuName)   "visu1"
      set confCam(B,visuName)   "$caption(confcam,nouvelle_visu)"
      set confCam(C,visuName)   "$caption(confcam,nouvelle_visu)"
      set confCam(A,camNo)      "0"
      set confCam(B,camNo)      "0"
      set confCam(C,camNo)      "0"
      set confCam(A,visuNo)     "0"
      set confCam(B,visuNo)     "0"
      set confCam(C,visuNo)     "0"
      set confCam(A,camName)    ""
      set confCam(B,camName)    ""
      set confCam(C,camName)    ""
      set confCam(A,threadNo)   "0"
      set confCam(B,threadNo)   "0"
      set confCam(C,threadNo)   "0"
      set confCam(A,product)    ""
      set confCam(B,product)    ""
      set confCam(C,product)    ""
      set confCam(list_product) ""
   }

   proc dispThreadError { thread_id errorInfo} {
      ::console::disp "thread_id=$thread_id errorInfo=$errorInfo\n"
   }

   #
   # confCam::run
   # Cree la fenetre de choix et de configuration des cameras
   # This = chemin de la fenetre
   # confCam($camItem,camName) = nom de la camera
   #
   proc run { } {
      variable This
      global audace confCam

      set This "$audace(base).confCam"
      createDialog
      set camItem $confCam(currentCamItem)
      if { $confCam($camItem,camName) != "" } {
         select $camItem $confCam($camItem,camName)
         if { [ string compare $confCam($camItem,camName) sbig ] == "0" } {
            ::sbig::SbigDispTemp
         } elseif { [ string compare $confCam($camItem,camName) kitty ] == "0" } {
            ::kitty::KittyDispTemp
         } elseif { [ string compare $confCam($camItem,camName) andor ] == "0" } {
            ::andor::AndorDispTemp
         } elseif { [ string compare $confCam($camItem,camName) fingerlakes ] == "0" } {
            ::fingerlakes::FLIDispTemp
         } elseif { [ string compare $confCam($camItem,camName) cemes ] == "0" } {
            ::cemes::CemesDispTemp
         }
      } else {
         select $camItem audine
      }
   }

   #
   # confCam::startDriver
   # Ouvre les cameras
   #
   proc startDriver { } {
      global conf confCam

      if { $conf(camera,A,start) == "1" } {
         set confCam(A,camName) $conf(camera,A,camName)
         ::confCam::configureCamera "A"
      }
      if { $conf(camera,B,start) == "1" } {
         set confCam(B,camName) $conf(camera,B,camName)
         ::confCam::configureCamera "B"
      }
      if { $conf(camera,C,start) == "1" } {
         set confCam(C,camName) $conf(camera,C,camName)
         ::confCam::configureCamera "C"
      }
   }

   #
   # confCam::stopDriver
   # Ferme toutes les cameras ouvertes
   #
   proc stopDriver { } {
      ::confCam::stopItem A
      ::confCam::stopItem B
      ::confCam::stopItem C
   }

   #
   # confCam::ok
   # Fonction appellee lors de l'appui sur le bouton 'OK' pour appliquer
   # la configuration, et fermer la fenetre de reglage de la camera
   #
   proc ok { } {
      variable This

      $This.cmd.ok configure -relief groove -state disabled
      $This.cmd.appliquer configure -state disabled
      $This.cmd.aide configure -state disabled
      $This.cmd.fermer configure -state disabled
      appliquer
      fermer
   }

   #
   # confCam::appliquer
   # Fonction appellee lors de l'appui sur le bouton 'Appliquer' pour
   # memoriser et appliquer la configuration
   #
   proc appliquer { } {
      variable This
      global confCam

      $This.cmd.ok configure -state disabled
      $This.cmd.appliquer configure -relief groove -state disabled
      $This.cmd.aide configure -state disabled
      $This.cmd.fermer configure -state disabled
      #--- J'arrete la camera
      stopItem $confCam(currentCamItem)
      #--- je copie les parametres de la nouvelle camera dans conf()
      widgetToConf     $confCam(currentCamItem)
      configureCamera  $confCam(currentCamItem)
      $This.cmd.ok configure -state normal
      $This.cmd.appliquer configure -relief raised -state normal
      $This.cmd.aide configure -state normal
      $This.cmd.fermer configure -state normal
   }

   #
   # confCam::afficherAide
   # Fonction appellee lors de l'appui sur le bouton 'Aide'
   #
   proc afficherAide { } {
      variable This
      global confCam

      $This.cmd.ok configure -state disabled
      $This.cmd.appliquer configure -state disabled
      $This.cmd.aide configure -relief groove -state disabled
      $This.cmd.fermer configure -state disabled
      set selectedPluginName [ $This.usr.onglet raise ]
      set pluginTypeDirectory [ ::audace::getPluginTypeDirectory [ $selectedPluginName\::getPluginType ] ]
      set pluginHelp [ $selectedPluginName\::getPluginHelp ]
      ::audace::showHelpPlugin "$pluginTypeDirectory" "$selectedPluginName" "$pluginHelp"
      $This.cmd.ok configure -state normal
      $This.cmd.appliquer configure -state normal
      $This.cmd.aide configure -relief raised -state normal
      $This.cmd.fermer configure -state normal
   }

   #
   # confCam::fermer
   # Fonction appellee lors de l'appui sur le bouton 'Fermer'
   #
   proc fermer { } {
      variable This

      ::confCam::recupPosDim
      $This.cmd.ok configure -state disabled
      $This.cmd.appliquer configure -state disabled
      $This.cmd.aide configure -state disabled
      $This.cmd.fermer configure -relief groove -state disabled
      destroy $This
   }

   #
   # confCam::confAudine
   # Permet d'activer ou de desactiver le bouton Tests pour la fabrication de la camera Audine
   #
   proc confAudine { } {
      variable This
      global audace confCam

      set camItem $confCam(currentCamItem)

      #--- Si la fenetre Test pour la fabrication de la camera est affichee, je la ferme
      if { [ winfo exists $audace(base).testAudine ] } {
         ::testAudine::fermer
      }

      if { [ winfo exists $audace(base).confCam ] } {
         set frm [ $This.usr.onglet getframe audine ]
         if { [ ::confCam::getProduct $confCam($camItem,camNo) ] == "audine" && \
            [ ::confLink::getLinkNamespace $confCam(audine,port) ] == "parallelport" } {
            #--- Bouton Tests pour la fabrication de la camera actif
            $frm.test configure -state normal
         } else {
            #--- Bouton Tests pour la fabrication de la camera inactif
            $frm.test configure -state disabled
         }
      }
   }

   #
   # confCam::confDSLR
   # Permet d'activer ou de desactiver le bouton de configuration des APN (DSLR)
   #
   proc confDSLR { } {
      variable This
      global audace confCam

      set camItem $confCam(currentCamItem)

      #--- Si la fenetre Telecharger l'image pour la fabrication de la camera est affichee, je la ferme
      if { [ winfo exists $audace(base).telecharge_image ] } {
         destroy $audace(base).telecharge_image
      }

      if { [ winfo exists $audace(base).confCam ] } {
         set frm [ $This.usr.onglet getframe dslr ]
         if { [ winfo exists $frm.config_telechargement ] } {
            if { [::confCam::getProduct $confCam($camItem,camNo)] == "dslr" } {
               #--- Bouton de configuration des APN (DSLR)
               $frm.config_telechargement configure -state normal
            } else {
               #--- Bouton de configuration des APN (DSLR)
               $frm.config_telechargement configure -state disabled
            }
         }
         if { $confCam(dslr,longuepose) == "1" } {
            #--- Widgets de configuration de la longue pose actifs
            $frm.configure_longuepose configure -state normal
            $frm.moyen_longuepose configure -state normal
            $frm.longueposelinkbit configure -state normal
            $frm.longueposestartvalue configure -state normal
         } else {
            #--- Widgets de configuration de la longue pose inactifs
            $frm.configure_longuepose configure -state disabled
            $frm.moyen_longuepose configure -state disabled
            $frm.longueposelinkbit configure -state disabled
            $frm.longueposestartvalue configure -state disabled
         }
      }
   }

   #
   # confCam::recupPosDim
   # Permet de recuperer et de sauvegarder la position de la fenetre de configuration de la camera
   #
   proc recupPosDim { } {
      variable This
      global conf confCam

      set confCam(geometry) [ wm geometry $This ]
      set conf(camera,geometry) $confCam(geometry)
   }

   proc createDialog { } {
      variable This
      global audace caption conf confCam

      #---
      if { [ winfo exists $This ] } {
         wm withdraw $This
         wm deiconify $This
         select $confCam(currentCamItem) $confCam($confCam(currentCamItem),camName)
         focus $This
         return
      }
      #---
      toplevel $This
      wm geometry $This $confCam(geometry)
      wm minsize $This 670 430
      wm resizable $This 1 1
      wm deiconify $This
      wm title $This "$caption(confcam,config)"
      wm protocol $This WM_DELETE_WINDOW ::confCam::fermer

      frame $This.usr -borderwidth 0 -relief raised
         #--- Creation de la fenetre a onglets
         set notebook [ NoteBook $This.usr.onglet ]
         for { set i 0 } { $i < [ llength $confCam(names) ] } { incr i } {
            set pluginInfo(os) [ ::[ lindex $confCam(names) $i ]::getPluginOS ]
            foreach os $pluginInfo(os) {
               if { $os == [ lindex $::tcl_platform(os) 0 ] } {
                  fillPage[ lindex $confCam(names) $i ] [$notebook insert end [ lindex $confCam(names) $i ] \
                     -text [ lindex $confCam(labels) $i ] ]
               }
            }
         }
         pack $notebook -fill both -expand 1
      pack $This.usr -side top -fill both -expand 1

      #--- Je recupere la liste des visu
      set list_visu [list ]
      foreach visuNo [::visu::list] {
         lappend list_visu "visu$visuNo"
      }
      lappend list_visu $caption(confcam,nouvelle_visu)
      set confCam(list_visu) $list_visu

      #--- Parametres de la camera A
      frame $This.startA -borderwidth 1 -relief raised
         radiobutton $This.startA.item -anchor w -highlightthickness 0 \
            -text "A :" -value "A" -variable confCam(currentCamItem) \
            -command "::confCam::selectCamItem"
         pack $This.startA.item -side left -padx 3 -pady 3 -fill x
         label $This.startA.camNo -textvariable confCam(A,camNo)
         pack $This.startA.camNo -side left -padx 3 -pady 3 -fill x
         label $This.startA.name -textvariable confCam(A,camName)
         pack $This.startA.name -side left -padx 3 -pady 3 -fill x

         ComboBox $This.startA.visu \
            -width 8          \
            -height [ llength $confCam(list_visu) ] \
            -relief sunken    \
            -borderwidth 1    \
            -editable 0       \
            -textvariable confCam(A,visuName) \
            -values $confCam(list_visu)
         pack $This.startA.visu -side left -padx 3 -pady 3 -fill x
         button $This.startA.stop -text "$caption(confcam,arreter)" -width 7 -command "::confCam::stopItem A"
         pack $This.startA.stop -side left -padx 3 -pady 3 -expand true
         checkbutton $This.startA.chk -text "$caption(confcam,creer_au_demarrage)" \
            -highlightthickness 0 -variable conf(camera,A,start)
         pack $This.startA.chk -side left -padx 3 -pady 3 -expand true
      pack $This.startA -side top -fill x

      #--- Parametres de la camera B
      frame $This.startB -borderwidth 1 -relief raised
         radiobutton $This.startB.item -anchor w -highlightthickness 0 \
            -text "B :" -value "B" -variable confCam(currentCamItem) \
            -command "::confCam::selectCamItem"
         pack $This.startB.item -side left -padx 3 -pady 3 -fill x
         label $This.startB.camNo -textvariable confCam(B,camNo)
         pack $This.startB.camNo -side left -padx 3 -pady 3 -fill x
         label $This.startB.name -textvariable confCam(B,camName)
         pack $This.startB.name -side left -padx 3 -pady 3 -fill x

         ComboBox $This.startB.visu \
            -width 8          \
            -height [ llength $confCam(list_visu) ] \
            -relief sunken    \
            -borderwidth 1    \
            -editable 0       \
            -textvariable confCam(B,visuName) \
            -values $confCam(list_visu)
         pack $This.startB.visu -side left -padx 3 -pady 3 -fill x
         button $This.startB.stop -text "$caption(confcam,arreter)" -width 7 -command "::confCam::stopItem B"
         pack $This.startB.stop -side left -padx 3 -pady 3 -expand true
         checkbutton $This.startB.chk -text "$caption(confcam,creer_au_demarrage)" \
            -highlightthickness 0 -variable conf(camera,B,start)
         pack $This.startB.chk -side left -padx 3 -pady 3 -expand true
      pack $This.startB -side top -fill x

      #--- Parametres de la camera C
      frame $This.startC -borderwidth 1 -relief raised
         radiobutton $This.startC.item -anchor w -highlightthickness 0 \
            -text "C :" -value "C" -variable confCam(currentCamItem) \
            -command "::confCam::selectCamItem"
         pack $This.startC.item -side left -padx 3 -pady 3 -fill x
         label $This.startC.camNo -textvariable confCam(C,camNo)
         pack $This.startC.camNo -side left -padx 3 -pady 3 -fill x
         label $This.startC.name -textvariable confCam(C,camName)
         pack $This.startC.name -side left -padx 3 -pady 3 -fill x

         ComboBox $This.startC.visu \
            -width 8          \
            -height [ llength $confCam(list_visu) ] \
            -relief sunken    \
            -borderwidth 1    \
            -editable 0       \
            -textvariable confCam(C,visuName) \
            -values $confCam(list_visu)
         pack $This.startC.visu -side left -padx 3 -pady 3 -fill x
         button $This.startC.stop -text "$caption(confcam,arreter)" -width 7 -command "::confCam::stopItem C"
         pack $This.startC.stop -side left -padx 3 -pady 3 -expand true
         checkbutton $This.startC.chk -text "$caption(confcam,creer_au_demarrage)" \
            -highlightthickness 0 -variable conf(camera,C,start)
         pack $This.startC.chk -side left -padx 3 -pady 3 -expand true
      pack $This.startC -side top -fill x

      #--- Frame pour les boutons
      frame $This.cmd -borderwidth 1 -relief raised
         button $This.cmd.ok -text "$caption(confcam,ok)" -width 7 -command "::confCam::ok"
         if { $conf(ok+appliquer) == "1" } {
            pack $This.cmd.ok -side left -padx 3 -pady 3 -ipady 5 -fill x
         }
         button $This.cmd.appliquer -text "$caption(confcam,appliquer)" -width 8 -command "::confCam::appliquer"
         pack $This.cmd.appliquer -side left -padx 3 -pady 3 -ipady 5 -fill x
         button $This.cmd.fermer -text "$caption(confcam,fermer)" -width 7 -command "::confCam::fermer"
         pack $This.cmd.fermer -side right -padx 3 -pady 3 -ipady 5 -fill x
         button $This.cmd.aide -text "$caption(confcam,aide)" -width 7 -command "::confCam::afficherAide"
         pack $This.cmd.aide -side right -padx 3 -pady 3 -ipady 5 -fill x
      pack $This.cmd -side top -fill x

      #---
      focus $This

      #--- Raccourci qui donne le focus a la Console et positionne le curseur dans la ligne de commande
      bind $This <Key-F1> { ::console::GiveFocus }

      #--- Mise a jour dynamique des couleurs
      ::confColor::applyColor $This
   }

   #
   #--- Cree une thread dediee a la camera
   #--- Retourne le numero de la thread placee dans la variable confCam(camItem,threadNo)
   #
   proc createThread { camNo bufNo visuNo } {
      global confCam

      #--- Je cree la thread de la camera, si l'option multithread est activee dans le TCL
      if { $::tcl_platform(threaded)==1 } {
         #--- creation dun nouvelle thread
         set threadNo [thread::create ]
         #--- declaration de la variable globale mainThreadNo dans la thread de la camera
         thread::send $threadNo "set mainThreadNo [thread::id]"
         #--- je copie la commande de la camera dans la thread de la camera
         thread::copycommand $threadNo "cam$camNo"
         #--- declaration de la variable globale camNo dans la thread de la camera
         thread::send $threadNo "set camNo $camNo"
         #--- je copie la commande du buffer dans la thread de la camera
         thread::copycommand $threadNo buf$bufNo
      } else {
         set threadNo "0"
      }
      return $threadNo
   }

   #
   # Cree un widget "label" avec une URL du site WEB
   #
   proc createUrlLabel { tkparent title url } {
      global audace color

      label $tkparent.labURL -text "$title" -font $audace(font,url) -fg $color(blue)
      if { $url != "" } {
         bind $tkparent.labURL <ButtonPress-1> "::audace::Lance_Site_htm $url"
      }
      bind $tkparent.labURL <Enter> "$tkparent.labURL configure -fg $color(purple)"
      bind $tkparent.labURL <Leave> "$tkparent.labURL configure -fg $color(blue)"
      return  $tkparent.labURL
   }

   #
   # Fenetre de configuration de Audine
   #
   proc fillPageaudine { frm } {
      global audace caption color conf confCam

      #--- confToWidget
      set confCam(audine,ampli_ccd) [ lindex "$caption(confcam,ampli_synchro) $caption(confcam,ampli_toujours)" $conf(audine,ampli_ccd) ]
      set confCam(audine,can)       $conf(audine,can)
      set confCam(audine,ccd)       $conf(audine,ccd)
      set confCam(audine,foncobtu)  [ lindex "$caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) $caption(confcam,obtu_synchro)" $conf(audine,foncobtu) ]
      set confCam(audine,mirh)      $conf(audine,mirh)
      set confCam(audine,mirv)      $conf(audine,mirv)
      set confCam(audine,port)      $conf(audine,port)
      set confCam(audine,typeobtu)  $conf(audine,typeobtu)

      #--- Creation des differents frames
      frame $frm.frame1 -borderwidth 0 -relief raised
      pack $frm.frame1 -side top -fill both -expand 1

      frame $frm.frame2 -borderwidth 0 -relief raised
      pack $frm.frame2 -side top -fill both -expand 1

      frame $frm.frame3 -borderwidth 0 -relief raised
      pack $frm.frame3 -side top -fill x

      frame $frm.frame4 -borderwidth 0 -relief raised
      pack $frm.frame4 -side bottom -fill x -pady 2

      frame $frm.frame5 -borderwidth 0 -relief raised
      pack $frm.frame5 -in $frm.frame1 -side left -fill both -expand 1

      frame $frm.frame6 -borderwidth 0 -relief raised
      pack $frm.frame6 -in $frm.frame1 -side left -fill both -expand 1

      frame $frm.frame7 -borderwidth 0 -relief raised
      pack $frm.frame7 -in $frm.frame1 -side left -fill both -expand 1

      frame $frm.frame8 -borderwidth 0 -relief raised
      pack $frm.frame8 -in $frm.frame2 -side left -fill both -expand 1

      frame $frm.frame9 -borderwidth 0 -relief raised
      pack $frm.frame9 -in $frm.frame2 -side left -fill both -expand 1 -padx 80

      frame $frm.frame10 -borderwidth 0 -relief raised
      pack $frm.frame10 -in $frm.frame5 -side top -fill both -expand 1

      frame $frm.frame11 -borderwidth 0 -relief raised
      pack $frm.frame11 -in $frm.frame5 -side top -fill both -expand 1

      frame $frm.frame12 -borderwidth 0 -relief raised
      pack $frm.frame12 -in $frm.frame6 -side top -fill both -expand 1

      frame $frm.frame13 -borderwidth 0 -relief raised
      pack $frm.frame13 -in $frm.frame6 -side top -fill both -expand 1

      frame $frm.frame14 -borderwidth 0 -relief raised
      pack $frm.frame14 -in $frm.frame7 -side top -fill both -expand 1

      frame $frm.frame15 -borderwidth 0 -relief raised
      pack $frm.frame15 -in $frm.frame7 -side top -fill both -expand 1

      frame $frm.frame16 -borderwidth 0 -relief raised
      pack $frm.frame16 -in $frm.frame8 -side top -fill both -expand 1

      frame $frm.frame17 -borderwidth 0 -relief raised
      pack $frm.frame17 -in $frm.frame8 -side top -fill both -expand 1

      #--- Definition du port
      label $frm.lab1 -text "$caption(confcam,port_liaison)"
      pack $frm.lab1 -in $frm.frame10 -anchor center -side left -padx 10

      #--- Je constitue la liste des liaisons pour l'acquisition des images
      set list_combobox [ ::confLink::getLinkLabels { "parallelport" "quickaudine" "ethernaude" "audinet" } ]

      #--- Je verifie le contenu de la liste
      if { [llength $list_combobox ] > 0 } {
         #--- si la liste n'est pas vide,
         #--- je verifie que la valeur par defaut existe dans la liste
         if { [lsearch -exact $list_combobox $confCam(audine,port)] == -1 } {
            #--- si la valeur par defaut n'existe pas dans la liste,
            #--- je la remplace par le premier item de la liste
            set confCam(audine,port) [lindex $list_combobox 0]
         }
      } else {
         #--- si la liste est vide, on continue quand meme
      }

      #--- Choix du port ou de la liaison
      ComboBox $frm.port \
         -width 11       \
         -height [ llength $list_combobox ] \
         -relief sunken  \
         -borderwidth 1  \
         -editable 0     \
         -textvariable confCam(audine,port) \
         -values $list_combobox
      pack $frm.port -in $frm.frame10 -anchor center -side right -padx 10

      #--- Bouton de configuration des liaisons
      button $frm.configure -text "$caption(confcam,configurer)" -relief raised \
         -command {
            ::confLink::run ::confCam(audine,port) \
               { "parallelport" "quickaudine" "ethernaude" "audinet" } \
               "- $caption(confcam,acquisition) - $caption(audine,camera)"
         }
      pack $frm.configure -in $frm.frame10 -side right -pady 10 -ipadx 10 -ipady 1 -expand true

      #--- Definition du format du CCD
      label $frm.lab2 -text "$caption(confcam,format_ccd)"
      pack $frm.lab2 -in $frm.frame11 -anchor center -side left -padx 10

      set list_combobox [ list $caption(audine,kaf400) $caption(audine,kaf1600) $caption(audine,kaf3200) ]
      ComboBox $frm.ccd \
         -width 7       \
         -height [ llength $list_combobox ] \
         -relief sunken \
         -borderwidth 1 \
         -editable 0    \
         -textvariable confCam(audine,ccd) \
         -values $list_combobox
      pack $frm.ccd -in $frm.frame11 -anchor center -side right -padx 10

      #--- Miroir en x et en y
      checkbutton $frm.mirx -text "$caption(confcam,miroir_x)" -highlightthickness 0 \
         -variable confCam(audine,mirh)
      pack $frm.mirx -in $frm.frame12 -anchor center -side left -padx 20

      checkbutton $frm.miry -text "$caption(confcam,miroir_y)" -highlightthickness 0 \
         -variable confCam(audine,mirv)
      pack $frm.miry -in $frm.frame13 -anchor center -side left -padx 20

      #--- Fonctionnement de l'ampli du CCD
      label $frm.lab3 -text "$caption(confcam,ampli_ccd)"
      pack $frm.lab3 -in $frm.frame14 -anchor center -side left -padx 10

      set list_combobox [ list $caption(confcam,ampli_synchro) $caption(confcam,ampli_toujours) ]
      ComboBox $frm.ampli_ccd \
         -width 10            \
         -height [ llength $list_combobox ] \
         -relief sunken       \
         -borderwidth 1       \
         -editable 0          \
         -textvariable confCam(audine,ampli_ccd) \
         -values $list_combobox
       pack $frm.ampli_ccd -in $frm.frame14 -anchor center -side right -padx 10

      #--- Modele du CAN
      label $frm.lab4 -text "$caption(confcam,modele_can)"
      pack $frm.lab4 -in $frm.frame15 -anchor center -side left -padx 10

      set list_combobox [ list $caption(audine,can_ad976a) $caption(audine,can_ltc1605) ]
      ComboBox $frm.can \
         -width 10      \
         -height [ llength $list_combobox ] \
         -relief sunken \
         -borderwidth 1 \
         -editable 0    \
         -textvariable confCam(audine,can) \
         -values $list_combobox
      pack $frm.can -in $frm.frame15 -anchor center -side right -padx 10

      #--- Definition du type d'obturateur
      label $frm.lab5 -text "$caption(confcam,type_obtu)"
      pack $frm.lab5 -in $frm.frame16 -anchor center -side left -padx 10

      set list_combobox [ list $caption(audine,obtu_audine) $caption(audine,obtu_audine-) \
         $caption(audine,obtu_i2c) $caption(audine,obtu_thierry) ]
      ComboBox $frm.typeobtu \
         -width 11           \
         -height [ llength $list_combobox ] \
         -relief sunken      \
         -borderwidth 1      \
         -editable 0         \
         -textvariable confCam(audine,typeobtu) \
         -values $list_combobox
      pack $frm.typeobtu -in $frm.frame16 -anchor center -side right -padx 10

      #--- Fonctionnement de l'obturateur
      label $frm.lab6 -text "$caption(confcam,fonc_obtu)"
      pack $frm.lab6 -in $frm.frame17 -anchor center -side left -padx 10

      set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
         $caption(confcam,obtu_synchro) ]
      set confCam(audine,list_foncobtu) $list_combobox
      ComboBox $frm.foncobtu \
         -width 11           \
         -height [ llength $list_combobox ] \
         -relief sunken      \
         -borderwidth 1      \
         -editable 0         \
         -textvariable confCam(audine,foncobtu) \
         -values $list_combobox
      pack $frm.foncobtu -in $frm.frame17 -anchor center -side right -padx 10

      #--- Bouton de test d'une Audine en fabrication
      button $frm.test -text "$caption(confcam,test_fab_audine)" -relief raised \
         -command { ::testAudine::run $::audace(base).testAudine $::confCam(currentCamItem) }
      pack $frm.test -in $frm.frame3 -side top -pady 10 -ipadx 10 -ipady 5 -expand true

      #--- Gestion du bouton actif/inactif
      ::confCam::confAudine

      #--- Site web officiel de l'Audine
      label $frm.lab103 -text "$caption(confcam,site_web_ref)"
      pack $frm.lab103 -in $frm.frame4 -side top -fill x -pady 2

      set labelName [ ::confCam::createUrlLabel $frm.frame4 "$caption(confcam,site_audine)" \
         "$caption(confcam,site_audine)" ]
      pack $labelName -side top -fill x -pady 2
   }

   #
   # Fenetre de configuration des Hi-SIS
   #
   proc fillPagehisis { frm } {
      variable This
      global audace caption color conf confCam

      #--- confToWidget
      set confCam(hisis,delai_a)  $conf(hisis,delai_a)
      set confCam(hisis,delai_b)  $conf(hisis,delai_b)
      set confCam(hisis,delai_c)  $conf(hisis,delai_c)
      set confCam(hisis,foncobtu) [ lindex "$caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) $caption(confcam,obtu_synchro)" $conf(hisis,foncobtu) ]
      set confCam(hisis,mirh)     $conf(hisis,mirh)
      set confCam(hisis,mirv)     $conf(hisis,mirv)
      set confCam(hisis,modele)   [ lsearch "11 22 23 24 33 36 39 43 44 48" "$conf(hisis,modele)" ]
      set confCam(hisis,port)     $conf(hisis,port)
      set confCam(hisis,res)      $conf(hisis,res)

      #--- Creation des differents frames
      frame $frm.frame1 -borderwidth 0 -relief raised
      pack $frm.frame1 -side top -fill x -pady 10

      frame $frm.frame2 -borderwidth 0 -relief raised
      pack $frm.frame2 -side top -fill x -pady 10

      frame $frm.frame3 -borderwidth 0 -relief raised
      pack $frm.frame3 -side top -fill both -expand 1

      frame $frm.frame4 -borderwidth 0 -relief raised
      pack $frm.frame4 -side bottom -fill x -pady 2

      frame $frm.frame5 -borderwidth 0 -relief raised
      pack $frm.frame5 -in $frm.frame3 -side left -fill both -expand 1

      frame $frm.frame6 -borderwidth 0 -relief raised
      pack $frm.frame6 -in $frm.frame3 -side left -fill both -expand 1

      frame $frm.frame7 -borderwidth 0 -relief raised
      pack $frm.frame7 -in $frm.frame5 -side top -fill both -expand 1

      frame $frm.frame8 -borderwidth 0 -relief raised
      pack $frm.frame8 -in $frm.frame5 -side top -fill both -expand 1

      frame $frm.frame9 -borderwidth 0 -relief raised
      pack $frm.frame9 -in $frm.frame7 -side left -fill both -expand 1

      frame $frm.frame10 -borderwidth 0 -relief raised
      pack $frm.frame10 -in $frm.frame7 -side left -fill both -expand 1

      frame $frm.frame11 -borderwidth 0 -relief raised
      pack $frm.frame11 -in $frm.frame9 -side top -fill both -expand 1

      frame $frm.frame12 -borderwidth 0 -relief raised
      pack $frm.frame12 -in $frm.frame9 -side top -fill both -expand 1

      frame $frm.frame13 -borderwidth 0 -relief raised
      pack $frm.frame13 -in $frm.frame6 -side top -fill both -expand 1

      frame $frm.frame14 -borderwidth 0 -relief raised
      pack $frm.frame14 -in $frm.frame6 -side top -fill both -expand 1

      frame $frm.frame15 -borderwidth 0 -relief raised
      pack $frm.frame15 -in $frm.frame6 -side top -fill both -expand 1

      #--- Bouton radio Hi-SIS11
      radiobutton $frm.radio0 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_11)" -value 0 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab0 ] } {
               destroy $frm.lab0 ; destroy $frm.foncobtu
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio0 -in $frm.frame1 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS22
      radiobutton $frm.radio1 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_22)" -value 1 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11         \
                  -height [ llength $list_combobox ] \
                  -relief sunken    \
                  -borderwidth 1    \
                  -editable 0       \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
               pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Resolution
            label $frm.lab2 -text "$caption(confcam,can_resolution)"
            pack $frm.lab2 -in $frm.frame12 -anchor center -side left -padx 10
            set list_combobox [ list $caption(confcam,can_12bits) $caption(confcam,can_14bits) ]
            ComboBox $frm.res \
               -width 7       \
               -height [ llength $list_combobox ] \
               -relief sunken \
               -borderwidth 1 \
               -editable 0    \
               -textvariable confCam(hisis,res) \
               -values $list_combobox
            pack $frm.res -in $frm.frame12 -anchor center -side right -padx 20
            #--- Parametrage des delais
            label $frm.lab3 -text "$caption(confcam,delai_a)"
            pack $frm.lab3 -in $frm.frame13 -anchor center -side left -padx 10
            entry $frm.delai_a -textvariable confCam(hisis,delai_a) -width 3 -justify center
            pack $frm.delai_a -in $frm.frame13 -anchor center -side left
            label $frm.lab4 -text "$caption(confcam,delai_b)"
            pack $frm.lab4 -in $frm.frame14 -anchor center -side left -padx 10
            entry $frm.delai_b -textvariable confCam(hisis,delai_b) -width 3 -justify center
            pack $frm.delai_b -in $frm.frame14 -anchor center -side left
            label $frm.lab5 -text "$caption(confcam,delai_c)"
            pack $frm.lab5 -in $frm.frame15 -anchor center -side left -padx 10
            entry $frm.delai_c -textvariable confCam(hisis,delai_c) -width 3 -justify center
            pack $frm.delai_c -in $frm.frame15 -anchor center -side left
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio1 -in $frm.frame1 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS23
      radiobutton $frm.radio2 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_23)" -value 2 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
            pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio2 -in $frm.frame1 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS24
      radiobutton $frm.radio3 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_24)" -value 3 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
               pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio3 -in $frm.frame1 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS33
      radiobutton $frm.radio4 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_33)" -value 4 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
               pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio4 -in $frm.frame1 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS36
      radiobutton $frm.radio5 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_36)" -value 5 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
            pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio5 -in $frm.frame2 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS39
      radiobutton $frm.radio6 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_39)" -value 6 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
               pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio6 -in $frm.frame2 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS43
      radiobutton $frm.radio7 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_43)" -value 7 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
               pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio7 -in $frm.frame2 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS44
      radiobutton $frm.radio8 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_44)" -value 8 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
               pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio8 -in $frm.frame2 -anchor center -side left -padx 10

      #--- Bouton radio Hi-SIS48
      radiobutton $frm.radio9 -anchor nw -highlightthickness 0 -padx 0 -pady 0 \
         -text "$caption(confcam,hisis_48)" -value 9 -variable confCam(hisis,modele) -command {
            set frm [ $::confCam::This.usr.onglet getframe hisis ]
            if { [ winfo exists $frm.lab2 ] } {
               destroy $frm.lab2 ; destroy $frm.res
               destroy $frm.lab3 ; destroy $frm.delai_a
               destroy $frm.lab4 ; destroy $frm.delai_b
               destroy $frm.lab5 ; destroy $frm.delai_c
            }
            #--- Choix du fonctionnement de l'obturateur
            if { ! [ winfo exists $frm.lab0 ] } {
               label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
               pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 10
               set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
                  $caption(confcam,obtu_synchro) ]
               ComboBox $frm.foncobtu \
                  -width 11           \
                  -height [ llength $list_combobox ] \
                  -relief sunken      \
                  -borderwidth 1      \
                  -editable 0         \
                  -textvariable confCam(hisis,foncobtu) \
                  -values $list_combobox
               pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
            }
            #--- Mise a jour dynamique des couleurs
            ::confColor::applyColor $frm
         }
      pack $frm.radio9 -in $frm.frame2 -anchor center -side left -padx 10

      #--- Definition du port
      label $frm.lab1 -text "$caption(confcam,port)"
      pack $frm.lab1 -in $frm.frame11 -anchor center -side left -padx 10

      #--- Je constitue la liste des liaisons pour l'acquisition des images
      set list_combobox [ ::confLink::getLinkLabels { "parallelport" } ]

      #--- Je verifie le contenu de la liste
      if { [llength $list_combobox ] > 0 } {
         #--- si la liste n'est pas vide,
         #--- je verifie que la valeur par defaut existe dans la liste
         if { [lsearch -exact $list_combobox $confCam(hisis,port)] == -1 } {
            #--- si la valeur par defaut n'existe pas dans la liste,
            #--- je la remplace par le premier item de la liste
            set confCam(hisis,port) [lindex $list_combobox 0]
         }
      } else {
         #--- si la liste est vide, on continue quand meme
      }

      #--- Bouton de configuration des ports et liaisons
      button $frm.configure -text "$caption(confcam,configurer)" -relief raised \
         -command {
            ::confLink::run ::confCam(hisis,port) { parallelport } \
               "- $caption(confcam,acquisition) - $caption(hisis,camera)"
         }
      pack $frm.configure -in $frm.frame11 -anchor center -side left -pady 10 -ipadx 10 -ipady 1 -expand 0

      #--- Choix du port ou de la liaison
      ComboBox $frm.port \
         -width 7        \
         -height [ llength $list_combobox ] \
         -relief sunken  \
         -borderwidth 1  \
         -editable 0     \
         -textvariable confCam(hisis,port) \
         -values $list_combobox
      pack $frm.port -in $frm.frame11 -anchor center -side left -padx 20

      #--- Choix de la resolution et des delais
      if { $confCam(hisis,modele) == "1" } {
         set confCam(hisis,delai_a) $conf(hisis,delai_a)
         set confCam(hisis,delai_b) $conf(hisis,delai_b)
         set confCam(hisis,delai_c) $conf(hisis,delai_c)
         label $frm.lab2 -text "$caption(confcam,can_resolution)"
         pack $frm.lab2 -in $frm.frame12 -anchor center -side left -padx 10
         set list_combobox [ list $caption(confcam,can_12bits) $caption(confcam,can_14bits) ]
         ComboBox $frm.res \
            -width 7       \
            -height [ llength $list_combobox ] \
            -relief sunken \
            -borderwidth 1 \
            -editable 0    \
            -textvariable confCam(hisis,res) \
            -values $list_combobox
         pack $frm.res -in $frm.frame12 -anchor center -side right -padx 20
         label $frm.lab3 -text "$caption(confcam,delai_a)"
         pack $frm.lab3 -in $frm.frame13 -anchor center -side left -padx 10
         entry $frm.delai_a -textvariable confCam(hisis,delai_a) -width 3 -justify center
         pack $frm.delai_a -in $frm.frame13 -anchor center -side left -padx 10
         label $frm.lab4 -text "$caption(confcam,delai_b)"
         pack $frm.lab4 -in $frm.frame14 -anchor center -side left -padx 10
         entry $frm.delai_b -textvariable confCam(hisis,delai_b) -width 3 -justify center
         pack $frm.delai_b -in $frm.frame14 -anchor center -side left -padx 10
         label $frm.lab5 -text "$caption(confcam,delai_c)"
         pack $frm.lab5 -in $frm.frame15 -anchor center -side left -padx 10
         entry $frm.delai_c -textvariable confCam(hisis,delai_c) -width 3 -justify center
         pack $frm.delai_c -in $frm.frame15 -anchor center -side left -padx 10
      } else {
         destroy $frm.lab2
         destroy $frm.res
         destroy $frm.lab3
         destroy $frm.delai_a
         destroy $frm.lab4
         destroy $frm.delai_b
         destroy $frm.lab5
         destroy $frm.delai_c
      }

      #--- Choix des miroir de l'image
      checkbutton $frm.mirx -text "$caption(confcam,miroir_x)" -highlightthickness 0 \
         -variable confCam(hisis,mirh)
      pack $frm.mirx -in $frm.frame10 -anchor w -side top -padx 10 -pady 10
      checkbutton $frm.miry -text "$caption(confcam,miroir_y)" -highlightthickness 0 \
         -variable confCam(hisis,mirv)
      pack $frm.miry -in $frm.frame10 -anchor w -side bottom -padx 10 -pady 10

      #--- Choix du fonctionnement de l'obturateur
      if { $confCam(hisis,modele) != "0" } {
         label $frm.lab0 -text "$caption(confcam,fonc_obtu)"
         pack $frm.lab0 -in $frm.frame8 -anchor center -side left -padx 8
         set list_combobox [ list $caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) \
            $caption(confcam,obtu_synchro) ]
         ComboBox $frm.foncobtu \
            -width 11           \
            -height [ llength $list_combobox ] \
            -relief sunken      \
            -borderwidth 1      \
            -textvariable confCam(hisis,foncobtu) \
            -editable 0         \
            -values $list_combobox
         pack $frm.foncobtu -in $frm.frame8 -anchor center -side left -padx 10
      } else {
         destroy $frm.lab0
         destroy $frm.foncobtu
      }

      #--- Site web officiel des Hi-SIS
      label $frm.lab103 -text "$caption(confcam,site_web_ref)"
      pack $frm.lab103 -in $frm.frame4 -side top -fill x -pady 2

      set labelName [ ::confCam::createUrlLabel $frm.frame4 "$caption(confcam,site_hisis)" \
         "$caption(confcam,site_hisis)" ]
      pack $labelName -side top -fill x -pady 2
   }

   #
   # Fenetre de configuration des SBIG
   #
   proc fillPagesbig { frm } {
      #--- Construction de l'interface graphique
      ::sbig::fillConfigPage $frm
   }

   #
   # Fenetre de configuration de la CB245
   #
   proc fillPagecookbook { frm } {
      #--- Construction de l'interface graphique
      ::cookbook::fillConfigPage $frm
   }

   #
   # Fenetre de configuration des Starlight
   #
   proc fillPagestarlight { frm } {
      #--- Construction de l'interface graphique
      ::starlight::fillConfigPage $frm
   }

   #
   # Fenetre de configuration des Kitty
   #
   proc fillPagekitty { frm } {
      #--- Construction de l'interface graphique
      ::kitty::fillConfigPage $frm
   }

   #
   # Fenetre de configuration des WebCam
   #
   proc fillPagewebcam { frm } {
      global confCam

      #--- Construction de l'interface graphique
      ::webcam::fillConfigPage $frm $confCam(currentCamItem)
   }

   #
   # Fenetre de configuration de la TH7852A d'Yves LATIL
   #
   proc fillPageth7852a { frm } {
      #--- Construction de l'interface graphique
      ::th7852a::fillConfigPage $frm
   }

   #
   # Fenetre de configuration de la SCR1300XTC
   #
   proc fillPagescr1300xtc { frm } {
      #--- Construction de l'interface graphique
      ::scr1300xtc::fillConfigPage $frm
   }

   #
   # Fenetre de configuration des APN (DSLR)
   #
   proc fillPagedslr { frm } {
      global audace caption color conf confCam

      #--- confToWidget
      set confCam(dslr,longuepose)           $conf(dslr,longuepose)
      set confCam(dslr,longueposeport)       $conf(dslr,longueposeport)
      set confCam(dslr,longueposelinkbit)    $conf(dslr,longueposelinkbit)
      set confCam(dslr,longueposestartvalue) $conf(dslr,longueposestartvalue)
      set confCam(dslr,longueposestopvalue)  $conf(dslr,longueposestopvalue)
      set confCam(dslr,statut_service)       $conf(dslr,statut_service)
      set confCam(dslr,mirh)                 $conf(dslr,mirh)
      set confCam(dslr,mirv)                 $conf(dslr,mirv)

      #--- Creation des differents frames
      frame $frm.frame1 -borderwidth 0 -relief raised
      pack $frm.frame1 -side top -fill x

      frame $frm.frame4 -borderwidth 0 -relief raised
      pack $frm.frame4 -side top -fill x

      frame $frm.frame5 -borderwidth 0 -relief raised
      pack $frm.frame5 -in $frm.frame4 -anchor n -side top -fill x

      frame $frm.frame6 -borderwidth 0 -relief raised
      pack $frm.frame6 -in $frm.frame1 -anchor n -side left -fill x

      frame $frm.frame7 -borderwidth 1 -relief solid
      pack $frm.frame7 -in $frm.frame1 -anchor n -side right -fill x

      frame $frm.frame8 -borderwidth 0 -relief raised
      pack $frm.frame8 -in $frm.frame7 -anchor n -side top -fill x

      frame $frm.frame9 -borderwidth 0 -relief raised
      pack $frm.frame9 -in $frm.frame7 -anchor n -side top -fill x

      frame $frm.frame10 -borderwidth 0 -relief raised
      pack $frm.frame10 -in $frm.frame7 -anchor n -side top -fill x

      frame $frm.frame11 -borderwidth 0 -relief raised
      pack $frm.frame11 -in $frm.frame4 -anchor n -side bottom -fill both -expand true

      frame $frm.frame12 -borderwidth 0 -relief raised
      pack $frm.frame12 -side bottom -fill x -pady 2

      #--- Miroir en x et en y
      checkbutton $frm.mirx -text "$caption(confcam,miroir_x)" -highlightthickness 0 \
         -variable confCam(dslr,mirh)
      pack $frm.mirx -in $frm.frame6 -anchor w -side top -padx 20 -pady 10

      checkbutton $frm.miry -text "$caption(confcam,miroir_y)" -highlightthickness 0 \
         -variable confCam(dslr,mirv)
      pack $frm.miry -in $frm.frame6 -anchor w -side top -padx 20 -pady 10

      #--- Je constitue la liste des liaisons pour la longuepose
      set list_combobox [ ::confLink::getLinkLabels { "parallelport" "quickremote" "external" } ]

      #--- Utilisation de la longue pose
      checkbutton $frm.longuepose -text "$caption(confcam,dslr_longuepose)" -highlightthickness 0 \
         -variable confCam(dslr,longuepose) -command { ::confCam::confDSLR }
      pack $frm.longuepose -in $frm.frame8 -anchor w -side left -padx 10 -pady 10

      #--- Je verifie le contenu de la liste
      if { [llength $list_combobox ] > 0 } {
         #--- si la liste n'est pas vide,
         #--- je verifie que la valeur par defaut existe dans la liste
         if { [lsearch -exact $list_combobox $confCam(dslr,longueposeport)] == -1 } {
            #--- si la valeur par defaut n'existe pas dans la liste,
            #--- je la remplace par le premier item de la liste
            set confCam(dslr,longueposeport) [lindex $list_combobox 0]
         }
      } else {
         #--- si la liste est vide
         #--- je desactive l'option longue pose
         set confCam(dslr,longueposeport) ""
         set confCam(dslr,longuepose) 0
         #--- j'empeche de selectionner l'option longue
         $frm.longuepose configure -state disable
      }

      #--- Bouton de configuration des ports et liaisons
      button $frm.configure_longuepose -text "$caption(confcam,configurer)" -relief raised \
         -command {
            ::confCam::configureAPNLinkLonguePose
            ::confLink::run ::confCam(dslr,longueposeport) { parallelport quickremote external } \
               "- $caption(confcam,dslr_longuepose) - $caption(dslr,camera)"
         }
      pack $frm.configure_longuepose -in $frm.frame8 -side left -pady 10 -ipadx 10 -ipady 1

      #--- Choix du port ou de la liaison
      ComboBox $frm.moyen_longuepose \
         -width 13                   \
         -height [ llength $list_combobox ] \
         -relief sunken              \
         -borderwidth 1              \
         -editable 0                 \
         -textvariable confCam(dslr,longueposeport) \
         -values $list_combobox      \
         -modifycmd {
            ::confCam::configureAPNLinkLonguePose
         }
      pack $frm.moyen_longuepose -in $frm.frame8 -anchor center -side left -padx 20

      #--- Choix du numero du bit pour la commande de la longue pose
      label $frm.lab4 -text "$caption(confcam,dslr_longueposebit)"
      pack $frm.lab4 -in $frm.frame9 -anchor center -side left -padx 3 -pady 5

      set list_combobox [ list 0 1 2 3 4 5 6 7 ]
      ComboBox $frm.longueposelinkbit \
         -width 7                     \
         -height [ llength $list_combobox ] \
         -relief sunken               \
         -borderwidth 1               \
         -textvariable confCam(dslr,longueposelinkbit) \
         -editable 0                  \
         -values $list_combobox
      pack $frm.longueposelinkbit -in $frm.frame9 -anchor center -side right -padx 20 -pady 5

      #--- Choix du niveau de depart pour la commande de la longue pose
      label $frm.lab5 -text "$caption(confcam,dslr_longueposestart)"
      pack $frm.lab5 -in $frm.frame10 -anchor center -side left -padx 3 -pady 5

      entry $frm.longueposestartvalue -width 4 -textvariable confCam(dslr,longueposestartvalue) -justify center
      pack $frm.longueposestartvalue -in $frm.frame10 -anchor center -side right -padx 20 -pady 5

      #--- Gestion du Service Windows de detection automatique des APN (DSLR)
      if { $::tcl_platform(platform) == "windows" } {
         checkbutton $frm.detect_service -text "$caption(confcam,dslr_detect_service)" -highlightthickness 0 \
            -variable confCam(dslr,statut_service)
         pack $frm.detect_service -in $frm.frame5 -anchor w -side top -padx 20 -pady 10
      }

      #--- Bouton du choix du telechargement de l'image de l'APN
      button $frm.config_telechargement -text $caption(confcam,dslr_telecharger) -state normal \
         -command { ::dslr::setLoadParameters $confCam($confCam(currentCamItem),visuNo) }
      pack $frm.config_telechargement -in $frm.frame11 -side top -pady 10 -ipadx 10 -ipady 5 -expand true

      #--- Gestion du bouton actif/inactif
      ::confCam::confDSLR

      #--- Site web officiel de GPhoto2
      label $frm.lab104 -text "$caption(confcam,site_web_ref)"
      pack $frm.lab104 -in $frm.frame12 -side top -fill x -pady 2

      set labelName [ ::confCam::createUrlLabel $frm.frame12 "$caption(confcam,site_dslr)" \
         "$caption(confcam,site_dslr)" ]
      pack $labelName -side top -fill x -pady 2
   }

   #
   # Fenetre de configuration de la Andor
   #
   proc fillPageandor { frm } {
      #--- Construction de l'interface graphique
      ::andor::fillConfigPage $frm
   }

   #
   # Fenetre de configuration de la FLI (Finger Lakes Instrumentation)
   #
   proc fillPagefingerlakes { frm } {
      #--- Construction de l'interface graphique
      ::fingerlakes::fillConfigPage $frm
   }

   #
   # Fenetre de configuration de la Cemes
   #
   proc fillPagecemes { frm } {
      #--- Construction de l'interface graphique
      ::cemes::fillConfigPage $frm
   }

   #
   # Fenetre de configuration de la Nikon CoolPix
   #
   proc fillPagecoolpix { frm } {
      #--- Construction de l'interface graphique
      ::coolpix::fillConfigPage $frm
   }

   #
   # confCam::connectCamera
   # Affichage d'un message d'alerte pendant la connexion de la camera au demarrage
   #
   proc connectCamera { } {
      variable This
      global audace caption color

      if [ winfo exists $audace(base).connectCamera ] {
         destroy $audace(base).connectCamera
      }

      toplevel $audace(base).connectCamera
      wm resizable $audace(base).connectCamera 0 0
      wm title $audace(base).connectCamera "$caption(confcam,attention)"
      if { [ info exists This ] } {
         if { [ winfo exists $This ] } {
            set posx_connectCamera [ lindex [ split [ wm geometry $This ] "+" ] 1 ]
            set posy_connectCamera [ lindex [ split [ wm geometry $This ] "+" ] 2 ]
            wm geometry $audace(base).connectCamera +[ expr $posx_connectCamera + 50 ]+[ expr $posy_connectCamera + 100 ]
            wm transient $audace(base).connectCamera $This
         }
      } else {
         wm geometry $audace(base).connectCamera +200+100
         wm transient $audace(base).connectCamera $audace(base)
      }
      #--- Cree l'affichage du message
      label $audace(base).connectCamera.labURL_1 -text "$caption(confcam,connexion_texte1)" \
         -font $audace(font,arial_10_b) -fg $color(red)
      pack $audace(base).connectCamera.labURL_1 -padx 10 -pady 2
      label $audace(base).connectCamera.labURL_2 -text "$caption(confcam,connexion_texte2)" \
         -font $audace(font,arial_10_b) -fg $color(red)
      pack $audace(base).connectCamera.labURL_2 -padx 10 -pady 2

      #--- La nouvelle fenetre est active
      focus $audace(base).connectCamera

      #--- Mise a jour dynamique des couleurs
      ::confColor::applyColor $audace(base).connectCamera
   }

   #----------------------------------------------------------------------------
   # confCam::select
   # Selectionne un onglet en passant le nom (eventuellement) de
   # la camera decrite dans l'onglet
   #----------------------------------------------------------------------------
   proc select { camItem { camName "audine" } } {
      variable This

      $This.usr.onglet raise $camName
   }

   #----------------------------------------------------------------------------
   # confCam::selectCamItem
   # Selectionne un onglet en passant l'item de la camera
   #
   # parametres :
   #    aucun
   #----------------------------------------------------------------------------
   proc selectCamItem { } {
      variable This
      global confCam

      #--- je recupere l'item courant
      set camItem $confCam(currentCamItem)

      #--- je selectionne l'onglet correspondant a la camera de cet item
      ::confCam::select $camItem [ $This.usr.onglet raise ]
   }

   #----------------------------------------------------------------------------
   # confCam::setShutter
   # Procedure de changement de l'obturateur de la camera
   #----------------------------------------------------------------------------
   proc setShutter { camNo shutterState} {
      variable This
      variable private
      global audace caption conf confCam panneau

      #---
      set camProduct [ cam$camNo product ]
      if { $confCam(A,camNo) == $camNo } {
         set camItem "A"
      } elseif { $confCam(B,camNo) == $camNo } {
         set camItem "B"
      } elseif { $confCam(C,camNo) == $camNo } {
         set camItem "C"
      } else {
         set camItem ""
      }
      #---
      set ShutterOptionList [ ::confCam::getPluginProperty $camItem shutterList ]
      set lg_ShutterOptionList [ llength $ShutterOptionList ]
      #---
      if { "$camProduct" != "" } {
         if { [ ::confCam::getPluginProperty $camItem hasShutter ] } {
            incr shutterState
            if { $lg_ShutterOptionList == "3" } {
               if { $shutterState == "3" } {
                  set shutterState "0"
               }
            } elseif { $lg_ShutterOptionList == "2" } {
               if { $shutterState == "3" } {
                  set shutterState "1"
               }
            }
            if { "$camProduct" == "audine" } {
               set conf(audine,foncobtu) $shutterState
            } elseif { "$camProduct" == "hisis" } {
               set conf(hisis,foncobtu) $shutterState
            } elseif { "$camProduct" == "sbig" } {
               set conf(sbig,foncobtu) $shutterState
            } elseif { "$camProduct" == "andor" } {
               set conf(andor,foncobtu) $shutterState
            } elseif { "$camProduct" == "fingerlakes" } {
               set conf(fingerlakes,foncobtu) $shutterState
            } elseif { "$camProduct" == "cemes" } {
               set conf(cemes,foncobtu) $shutterState
            }
            set frm [ $This.usr.onglet getframe $confCam($camItem,camName) ]
            #---
            switch -exact -- $shutterState {
               0  {
                  set confCam($camProduct,foncobtu) $caption(confcam,obtu_ouvert)
                  catch {
                     set ::$camProduct::private(foncobtu) $caption(confcam,obtu_ouvert)
                     $frm.foncobtu configure -height [ llength $ShutterOptionList ]
                     $frm.foncobtu configure -values $ShutterOptionList
                  }
                  cam$camNo shutter "opened"
               }
               1  {
                  set confCam($camProduct,foncobtu) $caption(confcam,obtu_ferme)
                  catch {
                     set ::$camProduct::private(foncobtu) $caption(confcam,obtu_ferme)
                     $frm.foncobtu configure -height [ llength $ShutterOptionList ]
                     $frm.foncobtu configure -values $ShutterOptionList
                  }
                  cam$camNo shutter "closed"
               }
               2  {
                  set confCam($camProduct,foncobtu) $caption(confcam,obtu_synchro)
                  catch {
                     set ::$camProduct::private(foncobtu) $caption(confcam,obtu_synchro)
                     $frm.foncobtu configure -height [ llength $ShutterOptionList ]
                     $frm.foncobtu configure -values $ShutterOptionList
                  }
                  cam$camNo shutter "synchro"
               }
            }
         } else {
            tk_messageBox -title $caption(confcam,pb) -type ok \
               -message $caption(confcam,onlycam+obt)
            return -1
         }
      } else {
         return -1
      }
      return $shutterState
   }

   #----------------------------------------------------------------------------
   # confCam::stopItem
   # Arrete la camera camItem
   #----------------------------------------------------------------------------
   proc stopItem { camItem } {
      global audace caption conf confCam

      if { $confCam($camItem,camName) != "" } {
         set camNo $confCam($camItem,camNo)

         #--- Je supprime la thread de la camera si elle existe
         if { $confCam($camItem,threadNo)!=0 } {
            #--- Je supprime la thread
            thread::release $confCam($camItem,threadNo)
            set confCam($camItem,threadNo) "0"
         }

         #--- Je ferme les ressources specifiques de la camera
         switch -exact -- $confCam($camItem,camName) {
            audine {
               #--- Je ferme la liaison d'acquisition de la camera
               ::confLink::delete $conf(audine,port) "cam$camNo" "acquisition"
               #--- Si la fenetre Test pour la fabrication de la camera est affichee, je la ferme
               if { [ winfo exists $audace(base).testAudine ] } {
                  ::testAudine::fermer
               }
               #--- Gestion des boutons
               ::confCam::confAudine
               #--- Je ferme la camera
               if { $confCam($camItem,camNo) != 0 } {
                 cam::delete $confCam($camItem,camNo)
                 set confCam($camItem,camNo) 0
               }
            }
            hisis {
               #--- Je ferme la liaison d'acquisition de la camera
               ::confLink::delete $conf(hisis,port) "cam$camNo" "acquisition"
               #--- Je ferme la camera
               if { $confCam($camItem,camNo) != 0 } {
                 cam::delete $confCam($camItem,camNo)
                 set confCam($camItem,camNo) 0
               }
            }
            sbig {
               ::sbig::stop $camItem
            }
            cookbook {
               ::cookbook::stop $camItem
            }
            starlight {
               ::starlight::stop $camItem
            }
            kitty {
               ::kitty::stop $camItem
            }
            webcam {
               ::webcam::stop $camItem
            }
            th7852a {
               ::th7852a::stop $camItem
            }
            scr1300xtc {
               ::scr1300xtc::stop $camItem
            }
            dslr {
               #--- Si la fenetre Telechargement d'images est affichee, je la ferme
               if { [ winfo exists $audace(base).telecharge_image ] } {
                  destroy $audace(base).telecharge_image
               }
               #--- Gestion des boutons
               ::confCam::confDSLR
               #--- Je ferme la liaison longuepose
               if { $conf(dslr,longuepose) == 1 } {
                  ::confLink::delete $conf(dslr,longueposeport) "cam$camNo" "longuepose"
               }
               #--- Restitue si necessaire l'etat du service WIA sous Windows
               if { $::tcl_platform(platform) == "windows" } {
                   if { [ cam$camNo systemservice ] != "$conf(dslr,statut_service)" } {
                      cam$camNo systemservice $conf(dslr,statut_service)
                   }
               }
               #--- Je ferme la camera
               if { $confCam($camItem,camNo) != 0 } {
                 cam::delete $confCam($camItem,camNo)
                 set confCam($camItem,camNo) 0
               }
            }
            andor {
               ::andor::stop $camItem
            }
            fli {
               ::fingerlakes::stop $camItem
            }
            cemes {
               ::cemes::stop $camItem
            }
            coolpix {
               ::coolpix::stop $camItem
            }
            default {
               #--- Supprime la camera
               set result [ catch { cam::delete $camNo } erreur ]
               if { $result == "1" } { console::affiche_erreur "$erreur \n" }
            }
         }
      }

      #--- Raz des parametres de l'item
      set confCam($camItem,camNo) "0"
      #--- Je desassocie la camera de la visu
      if { $confCam($camItem,visuNo) != 0 } {
         ::confVisu::setCamera $confCam($camItem,visuNo) "" 0
         set confCam($camItem,visuNo) "0"
      }
      #---
      if { $confCam($camItem,visuNo) == "1" } {
         #--- Mise a jour de la variable audace pour compatibilite
         set audace(camNo) $confCam($camItem,camNo)
      }
      set confCam($camItem,camName) ""
      set confCam($camItem,product) ""
      #--- Je mets a jour la liste des "cam$camNo product" des cameras connectees
      set confCam(list_product) [ list $confCam(A,product) $confCam(B,product) $confCam(C,product) ]
      #--- Sert a la surveillance du Listener de la configuration optique
      set confCam($camItem,super_camNo) $confCam($camItem,camNo)
   }

   #
   # confCam::isReady
   #    Retourne "1" si la camera est demarree, sinon retourne "0"
   #
   #  Parametres :
   #     camNo : Numero de la camera
   #
   proc isReady { camNo } {
      #--- Je verifie si la camera est capable fournir son nom
      set result [ catch { cam$camNo name } ]
      if { $result == 1 } {
         #--- Erreur
         return 0
      } else {
         #--- Camera OK
         return 1
      }
   }

   #
   # confCam::getPluginProperty
   #    Retourne la valeur d'une propriete de la camera
   #
   #  Parametres :
   #     camItem      : Instance de la camera
   #     propertyName : Propriete
   #
   proc getPluginProperty { camItem propertyName } {
      global caption conf confCam

      # binningList :      Retourne la liste des binnings disponibles
      # binningXListScan : Retourne la liste des binnings en x disponibles en mode scan
      # binningYListScan : Retourne la liste des binnings en y disponibles en mode scan
      # hasBinning :       Retourne l'existence d'un binning (1 : Oui, 0 : Non)
      # hasFormat :        Retourne l'existence d'un format (1 : Oui, 0 : Non)
      # hasLongExposure :  Retourne l'existence du mode longue pose (1 : Oui, 0 : Non)
      # hasScan :          Retourne l'existence du mode scan (1 : Oui, 0 : Non)
      # hasShutter :       Retourne l'existence d'un obturateur (1 : Oui, 0 : Non)
      # hasVideo :         Retourne l'existence du mode video (1 : Oui, 0 : Non)
      # hasWindow :        Retourne la possibilite de faire du fenetrage (1 : Oui, 0 : Non)
      # longExposure :     Retourne l'etat du mode longue pose (1: Actif, 0 : Inactif)
      # multiCamera :      Retourne la possibilite de connecter plusieurs cameras identiques (1 : Oui, 0 : Non)
      # shutterList :      Retourne l'etat de l'obturateur (O : Ouvert, F : Ferme, S : Synchro)

      #--- je recherche la valeur par defaut de la propriete
      #--- si la valeur par defaut de la propriete n'existe pas , je retourne une chaine vide
      switch $propertyName {
         binningList      { set result [ list "" ] }
         binningXListScan { set result [ list "" ] }
         binningYListScan { set result [ list "" ] }
         hasBinning       { set result 0 }
         hasFormat        { set result 0 }
         hasLongExposure  { set result 0 }
         hasScan          { set result 0 }
         hasShutter       { set result 0 }
         hasVideo         { set result 0 }
         hasWindow        { set result 0 }
         longExposure     { set result 1 }
         multiCamera      { set result 0 }
         shutterList      { set result [ list "" ] }
         default          { set result "" }
      }

      #--- si aucune camera n'est selectionnee, je retourne la valeur par defaut
      if { $camItem == "" || $confCam($camItem,camName)==""} {
         return $result
      }

      #--- si une camera est selectionnee, je recherche la valeur propre a la camera
      set camNo $confCam($camItem,camNo)
      set result [ ::$confCam($camItem,camName)::getPluginProperty $camItem $propertyName ]
      return $result
   }

   #
   # confCam::getCamNo
   #    Retourne le numero de la camera
   #
   #  Parametres :
   #     camItem : intance de la camera
   #
   proc getCamNo { camItem } {
      global confCam

      #--- si aucune camera n'est selectionnee, je retourne la valeur par defaut
      if { $camItem == "" || $confCam($camItem,camName)==""} {
         set result "0"
      } else {
         set result $confCam($camItem,camNo)
      }

      return $result
   }

   #
   # confCam::getName
   #    Retourne le nom de la camera si la camera est demarree, sinon retourne "0"
   #
   #  Parametres :
   #     camNo : Numero de la camera
   #
   proc getName { camNo } {
      #--- Je verifie si la camera est capable fournir son nom
      set result [ catch { cam$camNo name } camName ]
      #---
      if { $result == 1 } {
         #--- Erreur
         return 0
      } else {
         #--- Camera OK
         return $camName
      }
   }

   #
   # confCam::getProduct
   #    Retourne le nom de la famille de la camera si la camera est demarree, sinon retourne "0"
   #
   #  Parametres :
   #     camNo : Numero de la camera
   #
   proc getProduct { camNo } {
      #--- Je verifie si la camera est capable fournir son nom de famille
      set result [ catch { cam$camNo product } camProduct ]
      #---
      if { $result == 1 } {
         #--- Erreur
         return 0
      } else {
         #--- Camera OK
         return $camProduct
      }
   }

   #
   # confCam::getShutter
   #    Retourne l'etat de l'obturateur
   #    Si la camera n'a pas d'obturateur, retourne une chaine vide
   #  Parametres :
   #     camItem : Instance de la camera
   #
   proc getShutter { camItem  } {
      global conf confCam

      if { [info exists conf($confCam($camItem,camName),foncobtu) ] } {
         return $conf($confCam($camItem,camName),foncobtu)
      } else {
         return ""
      }
   }

   #
   # confCam::getThreadNo
   #    Retourne le numero de la thread de la camera
   #    Si la camera n'a pas de thread associee, la valeur retournee est "0"
   #  Parametres :
   #     camNo : Numero de la camera
   #
   proc getThreadNo { camNo } {
      global confCam

      if { $confCam(A,camNo) == $camNo } {
         set camItem "A"
      } elseif { $confCam(B,camNo) == $camNo } {
         set camItem "B"
      } elseif { $confCam(C,camNo) == $camNo } {
         set camItem "C"
      }
      return $confCam($camItem,threadNo)
   }

   #
   # confCam::configureAPNLinkLonguePose
   #    Positionne la liaison sur celle qui vient d'etre selectionnee pour
   #    la longue pose de la camera APN
   #
   proc configureAPNLinkLonguePose { } {
      global confCam

      #--- Je positionne startvalue par defaut en fonction du type de liaison
      if { [ ::confLink::getLinkNamespace $confCam(dslr,longueposeport) ] == "parallelport" } {
         set confCam(dslr,longueposestartvalue) "0"
         set confCam(dslr,longueposestopvalue)  "1"
      } elseif { [ ::confLink::getLinkNamespace $confCam(dslr,longueposeport) ] == "quickremote" } {
         set confCam(dslr,longueposestartvalue) "1"
         set confCam(dslr,longueposestopvalue)  "0"
      } else {
         set confCam(dslr,longueposestartvalue) "0"
         set confCam(dslr,longueposestopvalue)  "1"
      }
   }

   #
   # confCam::closeCamera
   #  Ferme la camera
   #
   #  Parametres :
   #     camNo : Numero de la camera
   #
   proc closeCamera { camNo } {
      global confCam

      if { $confCam(A,camNo) == $camNo } {
         stopItem "A"
      }
      if { $confCam(B,camNo) == $camNo } {
         stopItem "B"
      }
      if { $confCam(C,camNo) == $camNo } {
         stopItem "C"
      }
   }

   #
   # confCam::configureCamera
   # Configure la camera en fonction des donnees contenues dans le tableau conf :
   # confCam($camItem,camName) -> type de camera employe
   # conf(cam,A,...) -> proprietes de ce type de camera
   #
   proc configureCamera { camItem } {
      variable This
      global audace caption conf confCam confcolor

      #--- Initialisation de la variable erreur
      set erreur "1"

      #--- Je regarde si la camera selectionnee est a connexion multiple, sinon je sors de la procedure
      for { set i 0 } { $i < [ llength $confCam(list_product) ] } { incr i } {
         set product [ lindex $confCam(list_product) $i ]
         if { $product != ""} {
            if { [ winfo exists $audace(base).confCam ] } {
               if { [ string compare $product [ $This.usr.onglet raise ] ] == "0" } {
                  if { $product != "webcam" } {
                     set confCam($camItem,camNo)   "0"
                     set confCam($camItem,camName) ""
                     tk_messageBox -title "$caption(confcam,attention)" -type ok \
                        -message "$caption(confcam,connexion_texte3)"
                     return
                  }
               }
            }
         }
      }

      #--- Affichage d'un message d'alerte si necessaire
      ::confCam::connectCamera

      #--- J'enregistre le numero de la visu associee a la camera
      if { "$confCam($camItem,camName)" != "" } {
         if { $confCam($camItem,visuName) == $caption(confcam,nouvelle_visu) } {
            set visuNo [::confVisu::create]
         } else {
            #--- je recupere le numera de la visu
            scan $confCam($camItem,visuName) "visu%d" visuNo
            #--- je verifie que la visu existe
            if { [lsearch -exact [visu::list] $visuNo] == -1 } {
               #--- si la visu n'existe plus , je la recree
               set visuNo [::confVisu::create]
            }
         }
      } else {
         #--- Si c'est l'ouverture d'une camera au demarrage de Audela
         #--- J'impose la visu :
         if { $camItem == "A" } { set visuNo 1 }
         if { $camItem == "B" } { set visuNo [::confVisu::create] }
         if { $camItem == "C" } { set visuNo [::confVisu::create] }
      }
      set confCam($camItem,visuNo)   $visuNo
      set confCam($camItem,visuName) visu$visuNo

      #--- Remise a jour de la liste des visu
      set list_visu [list ]
      #--- je recherche les visu existantes
      foreach n [::visu::list] {
         lappend list_visu "visu$n"
      }
      #--- j'ajoute la visu "nouvelle"
      lappend list_visu $caption(confcam,nouvelle_visu)
      set confCam(list_visu) $list_visu

      if { [ info exists This ] } {
         if { [ winfo exists $This ] } {
            $This.startA.visu configure -height [ llength $confCam(list_visu) ]
            $This.startA.visu configure -values $confCam(list_visu)
            $This.startB.visu configure -height [ llength $confCam(list_visu) ]
            $This.startB.visu configure -values $confCam(list_visu)
            $This.startC.visu configure -height [ llength $confCam(list_visu) ]
            $This.startC.visu configure -values $confCam(list_visu)
         }
      }

      #--- Je recupere le numero buffer de la visu associee a la camera
      set bufNo [::confVisu::getBufNo $visuNo]

      set catchResult [ catch {
         switch -exact -- $confCam($camItem,camName) {
            hisis {
               if { $conf(hisis,modele) == "11" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS11 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 4096 0
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "22" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS22-[ lindex $conf(hisis,res) 0 ] ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele) ($conf(hisis,res))\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  cam$camNo delayloops $conf(hisis,delai_a) $conf(hisis,delai_b) $conf(hisis,delai_c)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "23" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS23 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "24" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS24 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "33" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS33 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "36" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS36 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "39" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS39 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "43" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS43 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "44" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS44 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               } elseif { $conf(hisis,modele) == "48" } {
                  set camNo [ cam::create hisis $conf(hisis,port) -name Hi-SIS48 ]
                  console::affiche_erreur "$caption(confcam,port_hisis) $conf(hisis,modele)\
                     $caption(confcam,2points) $conf(hisis,port)\n"
                  console::affiche_saut "\n"
                  set confCam($camItem,camNo) $camNo
                  set foncobtu $conf(hisis,foncobtu)
                  switch -exact -- $foncobtu {
                     0 {
                        cam$camNo shutter "opened"
                     }
                     1 {
                        cam$camNo shutter "closed"
                     }
                     2 {
                        cam$camNo shutter "synchro"
                     }
                  }
                  cam$camNo buf $bufNo
                  cam$camNo mirrorh $conf(hisis,mirh)
                  cam$camNo mirrorv $conf(hisis,mirv)
                  ::confVisu::visuDynamix $visuNo 32767 -32768
                  #--- je cree la liaison utilisee par la camera pour l'acquisition
                  set linkNo [ ::confLink::create $conf(hisis,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
               }
            }
            sbig {
               ::sbig::configureCamera $camItem
            }
            cookbook {
               ::cookbook::configureCamera $camItem
            }
            starlight {
               ::starlight::configureCamera $camItem
            }
            kitty {
               ::kitty::configureCamera $camItem
            }
            webcam {
               ::webcam::configureCamera $camItem
            }
            th7852a {
               ::th7852a::configureCamera $camItem
            }
            scr1300xtc {
               ::scr1300xtc::configureCamera $camItem
            }
            dslr {
               #--- Je cree la camera
               #--- Je mets audela_start_dir entre guillemets pour le cas ou le nom du repertoire contient des espaces
               set camNo [ cam::create digicam USB -name DSLR -debug_cam $conf(dslr,debug) -gphoto2_win_dll_dir \"$::audela_start_dir\" ]
               set confCam($camItem,camNo) $camNo
               console::affiche_erreur "$caption(confcam,dslr_name) $caption(confcam,2points)\
                  [ cam$camNo name ]\n"
               console::affiche_saut "\n"
               cam$camNo buf $bufNo
               cam$camNo mirrorh $conf(dslr,mirh)
               cam$camNo mirrorv $conf(dslr,mirv)
               #--- J'arrete le service WIA de Windows
               cam$camNo systemservice 0
               #--- je cree la thread dediee a la camera
               set confCam($camItem,threadNo) [::confCam::createThread $camNo $bufNo $confCam($camItem,visuNo)]
               #--- Parametrage des longues poses
               if { $conf(dslr,longuepose) == "1" } {
                  switch [ ::confLink::getLinkNamespace $conf(dslr,longueposeport) ] {
                     parallelport {
                        #--- Je cree la liaison longue pose
                        set linkNo [ ::confLink::create $conf(dslr,longueposeport) "cam$camNo" "longuepose" "bit $conf(dslr,longueposelinkbit)" ]
                        #---
                        cam$camNo longuepose 1
                        cam$camNo longueposelinkno $linkNo
                        cam$camNo longueposelinkbit $conf(dslr,longueposelinkbit)
                        cam$camNo longueposestartvalue $conf(dslr,longueposestartvalue)
                        cam$camNo longueposestopvalue  $conf(dslr,longueposestopvalue)
                     }
                     quickremote {
                        #--- Je cree la liaison longue pose
                        set linkNo [ ::confLink::create $conf(dslr,longueposeport) "cam$camNo" "longuepose" "bit $conf(dslr,longueposelinkbit)" ]
                        #---
                        cam$camNo longuepose 1
                        cam$camNo longueposelinkno $linkNo
                        cam$camNo longueposelinkbit $conf(dslr,longueposelinkbit)
                        cam$camNo longueposestartvalue $conf(dslr,longueposestartvalue)
                        cam$camNo longueposestopvalue  $conf(dslr,longueposestopvalue)
                     }
                     external {
                        cam$camNo longuepose 2
                     }
                  }
                  #--- j'ajoute la commande de liaison longue pose dans la thread de la camera
                  if { $confCam($camItem,threadNo) != 0 &&  [cam$camNo longueposelinkno] != 0} {
                     thread::copycommand $confCam($camItem,threadNo) "link[cam$camNo longueposelinkno]"
                  }
               } else {
                  #--- Pas de liaison longue pose
                  cam$camNo longuepose 0
               }
               #--- Parametrage du telechargement des images
               set resultUsecf [ catch { cam$camNo usecf $conf(dslr,utiliser_cf) } messageUseCf ]
               if { $resultUsecf == 1 } {
                  #--- si l'appareil n'a pas de carte memoire,
                  #--- je desactive l'utilisation de la carte memoire de l'appareil
                  console::affiche_erreur "$messageUseCf. Unset use memory card."
                  set conf(dslr,utiliser_cf) 0
                  cam$camNo usecf $conf(dslr,utiliser_cf)
               }
               switch -exact -- $conf(dslr,telecharge_mode) {
                  1  {
                     #--- Ne pas telecharger
                     cam$camNo autoload 0
                  }
                  2  {
                     #--- Telechargement immediat
                     cam$camNo autoload 1
                  }
                  3  {
                     #--- Telechargement pendant la pose suivante
                     cam$camNo autoload 0
                  }
               }
               #---
               ::confVisu::visuDynamix $visuNo 4096 -4096
            }
            andor {
               ::andor::configureCamera $camItem
            }
            fingerlakes {
               ::fingerlakes::configureCamera $camItem
            }
            cemes {
               ::cemes::configureCamera $camItem
            }
            coolpix {
               ::coolpix::configureCamera $camItem
            }
            audine {
               if { [ string range $conf(audine,ccd) 0 4 ] == "kaf16" } {
                  set ccd "kaf1602"
               } elseif { [ string range $conf(audine,ccd) 0 4 ] == "kaf32" } {
                  set ccd "kaf3200"
               } else {
                  set ccd "kaf401"
               }
               #--- je cree la camera en fonction de la liaison choisie
               #--- A MODIFER: creer d'abord la liaison, puis la camera audine
               switch [ ::confLink::getLinkNamespace $conf(audine,port) ] {
                  parallelport {
                     set camNo [cam::create audine $conf(audine,port) -name Audine -ccd $ccd ]
                     cam$camNo cantype $conf(audine,can)
                     #--- je cree la liaison utilisee par la camera pour l'acquisition
                     set linkNo [ ::confLink::create $conf(audine,port) "cam$camNo" "acquisition" "bits 1 to 8" ]
                  }
                  quickaudine {
                     set camNo [cam::create quicka $conf(audine,port) -name Audine -ccd $ccd ]
                     cam$camNo delayshutter $conf(quickaudine,delayshutter)
                     cam$camNo speed $conf(quickaudine,canspeed)
                     #--- je cree la liaison utilisee par la camera pour l'acquisition
                     set linkNo [ ::confLink::create $conf(audine,port) "cam$camNo" "acquisition" "" ]
                  }
                  ethernaude {
                     #--- Je verifie si la camera 500 du tutorial EthernAude est connectee, ensuite je la deconnecte
                     foreach camera [ ::cam::list ] {
                        if { $camera == "500" } {
                           tuto_exit
                        }
                     }
                     #---
                     ### set conf(ethernaude,host) [ ::audace::verifip $conf(ethernaude,host) ]
                     set eth_canspeed "0"
                     set eth_canspeed [ expr round(($conf(ethernaude,canspeed)-7.11)/(39.51-7.11)*30.) ]
                     if { $eth_canspeed < "0" } { set eth_canspeed "0" }
                     if { $eth_canspeed > "100" } { set eth_canspeed "100" }
                     if { [ string range $conf(audine,typeobtu) 0 5 ] == "audine" } {
                        #--- L'EthernAude inverse le fonctionnement de l'obturateur par rapport au
                        #--- port parallele, on retablit donc ici le meme fonctionnement
                        if { [ string index $conf(audine,typeobtu) 7 ] == "-" } {
                           set shutterinvert "0"
                        } else {
                           set shutterinvert "1"
                        }
                     }
                     #--- Gestion du mode debug ou non de l'EthernAude
                     if { $conf(ethernaude,debug) == "0" } {
                        if { $conf(ethernaude,ipsetting) == "1" } {
                           #--- Je mets le nom du fichier entre guillemets pour le cas ou le nom du
                           #--- repertoire contient des espaces
                           set camNo [cam::create ethernaude $conf(audine,port) -ip $conf(ethernaude,host) \
                              -canspeed $eth_canspeed -name Audine -shutterinvert $shutterinvert \
                              -ipsetting \"[ file join $audace(rep_install) bin IPSetting.exe ]\" ]
                        } else {
                           set camNo [ cam::create ethernaude $conf(audine,port) -ip $conf(ethernaude,host) \
                              -canspeed $eth_canspeed -name Audine -shutterinvert $shutterinvert ]
                        }
                     } else {
                        if { $conf(ethernaude,ipsetting) == "1" } {
                           #--- Je mets le nom du fichier entre guillemets pour le cas ou le nom du
                           #--- repertoire contient des espaces
                           set camNo [cam::create ethernaude $conf(audine,port) -ip $conf(ethernaude,host) \
                              -canspeed $eth_canspeed -name Audine -shutterinvert $shutterinvert \
                              -ipsetting \"[ file join $audace(rep_install) bin IPSetting.exe ]\" -debug_eth ]
                        } else {
                           set camNo [ cam::create ethernaude $conf(audine,port) -ip $conf(ethernaude,host) \
                              -canspeed $eth_canspeed -name Audine -shutterinvert $shutterinvert -debug_eth ]
                        }
                     }
                     #--- je cree la liaison utilisee par la camera pour l'acquisition
                     set linkNo [ ::confLink::create $conf(audine,port) "cam$camNo" "acquisition" "" ]
                  }
                  audinet {
                     set camNo [cam::create audinet $conf(audine,port) -ccd $ccd -name Audine \
                        -host $conf(audinet,host) -protocole $conf(audinet,protocole) -udptempo $conf(audinet,udptempo) \
                        -ipsetting $conf(audinet,ipsetting) -macaddress $conf(audinet,mac_address) \
                        -debug_cam $conf(audinet,debug) ]
                     #--- je cree la liaison utilisee par la camera pour l'acquisition
                     set linkNo [ ::confLink::create $conf(audine,port) "cam$camNo" "acquisition" "" ]
                  }
               }
               #--- fin switch conf(audine,port)

               #--- je parametre la camera
               set confCam($camItem,camNo) $camNo
               cam$camNo buf $bufNo
               cam$camNo mirrorh $conf(audine,mirh)
               cam$camNo mirrorv $conf(audine,mirv)

               #--- je cree la thread dediee a la camera
               set confCam($camItem,threadNo) [::confCam::createThread $camNo $bufNo $confCam($camItem,visuNo)]

               #--- je parametre le mode de fonctionnement de l'obturateur
               switch -exact -- $conf(audine,foncobtu) {
                  0 { cam$camNo shutter "opened" }
                  1 { cam$camNo shutter "closed" }
                  2 { cam$camNo shutter "synchro" }
               }

               #--- je parametre le type de l'obturateur
               #--- (sauf pour l'EthernAude qui est commande par l'option -shutterinvert)
               if { [ ::confLink::getLinkNamespace $conf(audine,port) ] != "ethernaude" } {
                  if { $conf(audine,typeobtu) == "$caption(audine,obtu_audine-)" } {
                     cam$camNo shuttertype audine reverse
                  } elseif { $conf(audine,typeobtu) == "$caption(audine,obtu_audine)" } {
                     cam$camNo shuttertype audine
                  } elseif { $conf(audine,typeobtu) == "$caption(audine,obtu_i2c)" } {
                     cam$camNo shuttertype audine
                  } elseif { $conf(audine,typeobtu) == "$caption(audine,obtu_thierry)" } {
                     set confcolor(obtu_pierre) "1"
                     ::Obtu_Pierre::run $camNo
                     cam$camNo shuttertype thierry
                 }
               }

               #--- je parametre le fonctionnement de l'ampli du CCD
               #--- (uniquement pour le port parallele et la QuickAudine)
               if { [ ::confLink::getLinkNamespace $conf(audine,port) ] == "parallelport" } {
                  switch -exact -- $conf(audine,ampli_ccd) {
                     0 { cam$camNo ampli "synchro" }
                     1 { cam$camNo ampli "on" }
                     2 { cam$camNo ampli "off" }
                  }
               } elseif { [ ::confLink::getLinkNamespace $conf(audine,port) ] == "quickaudine" } {
                  switch -exact -- $conf(audine,ampli_ccd) {
                     0 { cam$camNo ampli "synchro" }
                     1 { cam$camNo ampli "on" }
                     2 { cam$camNo ampli "off" }
                  }
               }

               #--- je configure la visu utilisee par la camera
               ::confVisu::visuDynamix $visuNo 32767 -32768

               #--- j'affiche un message d'information
               console::affiche_erreur "$caption(confcam,camera) [ cam$camNo name ] ([ cam$camNo ccd ])\n"
               console::affiche_erreur "$caption(confcam,port_liaison)\
                  ([ ::[ ::confLink::getLinkNamespace $conf(audine,port) ]::getPluginTitle ])\
                  $caption(confcam,2points) $conf(audine,port)\n"
               console::affiche_saut "\n"
            }
         }
         #--- <= fin du switch sur les cameras

         #--- Je mets a jour la liste des "cam$camNo product" des cameras connectees
         #--- En prenant en compte le cas particulier des APN Nikon CoolPix qui n'ont pas de librairie
         if { $confCam($camItem,camName) != "coolpix" && $confCam($camItem,camName) != "" } {
            if { $confCam(A,camNo) == $confCam($camItem,camNo) } {
               set camItem "A"
               set confCam(A,product) [ cam$confCam(A,camNo) product ]
            } elseif { $confCam(B,camNo) == $confCam($camItem,camNo) } {
               set camItem "B"
               set confCam(B,product) [ cam$confCam(B,camNo) product ]
            } elseif { $confCam(C,camNo) == $confCam($camItem,camNo) } {
               set camItem "C"
               set confCam(C,product) [ cam$confCam(C,camNo) product ]
            }
         } elseif { $confCam($camItem,camName) == "coolpix" } {
            if { $confCam(A,camNo) == $confCam($camItem,camNo) } {
               set camItem "A"
               set confCam(A,product) "coolpix"
            } elseif { $confCam(B,camNo) == $confCam($camItem,camNo) } {
               set camItem "B"
               set confCam(B,product) "coolpix"
            } elseif { $confCam(C,camNo) == $confCam($camItem,camNo) } {
               set camItem "C"
               set confCam(C,product) "coolpix"
            }
         }
         set confCam(list_product) [ list $confCam(A,product) $confCam(B,product) $confCam(C,product) ]

         #--- J'associe la camera avec la visu
         ::confVisu::setCamera $confCam($camItem,visuNo) $camItem $confCam($camItem,camNo)

      } errorMessage ]
      #--- <= fin du catch

      #--- Traitement des erreurs detectees par le catch
      if { $catchResult == "1" } {
         ::console::affiche_erreur "$::errorInfo\n\n"
         tk_messageBox -message "$errorMessage. See console" -icon error
         #--- Je desactive le demarrage automatique
         set conf(camera,$camItem,start) "0"
         #--- Je supprime la thread de la camera si elle existe
         if { $confCam($camItem,threadNo)!=0 } {
            #--- Je supprime la thread
            thread::release $confCam($camItem,threadNo)
            set confCam($camItem,threadNo) "0"
         }

         #--- En cas de probleme, camera par defaut
         set confCam($camItem,camName) ""
         set confCam($camItem,camNo)   "0"
         set confCam($camItem,visuNo)  "0"
      }

      if { $confCam($camItem,visuNo) == "1" } {
         #--- Mise a jour de la variable audace pour compatibilite
         set audace(camNo) $confCam($camItem,camNo)
      }

      #--- Creation d'une variable qui se met a jour a la fin de la procedure configureCamera
      #--- Sert au Listener de surveillance de la configuration optique
      set confCam($camItem,super_camNo) $confCam($camItem,camNo)

      #--- Gestion des boutons actifs/inactifs
      ::confCam::confAudine
      ::confCam::confDSLR
      #--- Effacement du message d'alerte s'il existe
      if [ winfo exists $audace(base).connectCamera ] {
         destroy $audace(base).connectCamera
      }

      #--- Desactive le blocage pendant l'acquisition (cli/sti)
      catch {
         cam$confCam($camItem,camNo) interrupt 0
      }

   }

   #
   # confCam::widgetToConf
   # Acquisition de la configuration, c'est a dire isolation des
   # differentes variables dans le tableau conf(...)
   #
   proc widgetToConf { camItem } {
      variable This
      global caption conf confCam

      set camName                       [ $This.usr.onglet raise ]
      set confCam($camItem,camName)     $camName
      set conf(camera,$camItem,camName) $camName

      switch $conf(camera,$camItem,camName) {
         audine {
            #--- Memorise la configuration de Audine dans le tableau conf(audine,...)
            set conf(audine,ampli_ccd)            [ lsearch "$caption(confcam,ampli_synchro) $caption(confcam,ampli_toujours)" "$confCam(audine,ampli_ccd)" ]
            set conf(audine,can)                  $confCam(audine,can)
            set conf(audine,ccd)                  $confCam(audine,ccd)
            set conf(audine,foncobtu)             [ lsearch "$caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) $caption(confcam,obtu_synchro)" "$confCam(audine,foncobtu)" ]
            set conf(audine,mirh)                 $confCam(audine,mirh)
            set conf(audine,mirv)                 $confCam(audine,mirv)
            set conf(audine,port)                 $confCam(audine,port)
            set conf(audine,typeobtu)             $confCam(audine,typeobtu)
         }
         hisis {
            #--- Memorise la configuration des Hi-SIS dans le tableau conf(hisis,...)
            set conf(hisis,delai_a)               $confCam(hisis,delai_a)
            set conf(hisis,delai_b)               $confCam(hisis,delai_b)
            set conf(hisis,delai_c)               $confCam(hisis,delai_c)
            set conf(hisis,foncobtu)              [ lsearch "$caption(confcam,obtu_ouvert) $caption(confcam,obtu_ferme) $caption(confcam,obtu_synchro)" "$confCam(hisis,foncobtu)" ]
            set conf(hisis,mirh)                  $confCam(hisis,mirh)
            set conf(hisis,mirv)                  $confCam(hisis,mirv)
            set conf(hisis,modele)                [ lindex "11 22 23 24 33 36 39 43 44 48" $confCam(hisis,modele) ]
            set conf(hisis,port)                  $confCam(hisis,port)
            set conf(hisis,res)                   $confCam(hisis,res)
         }
         sbig {
            #--- Memorise la configuration de la SBIG dans le tableau conf(sbig,...)
            ::sbig::widgetToConf
         }
         cookbook {
            #--- Memorise la configuration de la CB245 dans le tableau conf(cookbook,...)
            ::cookbook::widgetToConf
         }
         starlight {
            #--- Memorise la configuration des Starlight dans le tableau conf(starlight,...)
            ::starlight::widgetToConf
         }
         kitty {
            #--- Memorise la configuration des Kitty dans le tableau conf(kitty,...)
            ::kitty::widgetToConf
         }
         webcam {
            #--- Memorise la configuration de la WebCam dans le tableau conf(webcam,$camItem,...)
            ::webcam::widgetToConf $camItem
         }
         th7852a {
            #--- Memorise la configuration de la TH7852A dans le tableau conf(th7852a,...)
            ::th7852a::widgetToConf
         }
         scr1300xtc {
            #--- Memorise la configuration de la SCR1300XTC dans le tableau conf(scr1300xtc,...)
            ::scr1300xtc::widgetToConf
         }
         dslr {
            #--- Memorise la configuration de l'APN (DSLR) dans le tableau conf(dslr,...)
            set conf(dslr,longuepose)             $confCam(dslr,longuepose)
            set conf(dslr,longueposeport)         $confCam(dslr,longueposeport)
            set conf(dslr,longueposelinkbit)      $confCam(dslr,longueposelinkbit)
            set conf(dslr,longueposestartvalue)   $confCam(dslr,longueposestartvalue)
            set conf(dslr,longueposestopvalue)    $confCam(dslr,longueposestopvalue)
            set conf(dslr,statut_service)         $confCam(dslr,statut_service)
            set conf(dslr,mirh)                   $confCam(dslr,mirh)
            set conf(dslr,mirv)                   $confCam(dslr,mirv)
         }
         andor {
            #--- Memorise la configuration de la Andor dans le tableau conf(andor,...)
            ::andor::widgetToConf
         }
         fingerlakes {
            #--- Memorise la configuration de la FLI dans le tableau conf(fingerlakes,...)
            ::fingerlakes::widgetToConf
         }
         cemes {
            #--- Memorise la configuration de la Cemes dans le tableau conf(cemes,...)
            ::cemes::widgetToConf
         }
         coolpix {
            #--- Memorise la configuration de la Cemes dans le tableau conf(coolpix,...)
            ::coolpix::widgetToConf
         }
      }
   }

}

#--- Connexion au demarrage de la camera selectionnee par defaut
::confCam::init

