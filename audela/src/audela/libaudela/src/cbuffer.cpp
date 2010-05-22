/* cbuffer.cpp
 *
 * This file is part of the AudeLA project : <http://software.audela.free.fr>
 * Copyright (C) 1998-2004 The AudeLA Core Team
 *
 * Initial author : Denis MARCHAIS <denis.marchais@free.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "sysexp.h"
#include <math.h>
#include <time.h>

#include <float.h>         // pour _isnan()
#if defined(OS_WIN)
   #define isnan _isnan
#endif
#include "libstd.h"       // pour buf_pool
#include "cbuffer.h"
#include "cpixelsgray.h"
#include "cpixelsrgb.h"

#include "cfile.h"          // file io
#include "libdcjpeg.h"      // decode jpeg  pictures
#include "libdcraw.h"         // decode raw  pictures
#include "libtt.h"            // for TFLOAT, LONG_IMG, TT_PTR_...

/**
 *  constructeur par defaut
 *
 */

CBuffer::CBuffer() : CDevice()
{
   SetSavingType(USHORT_IMG);
   keywords = NULL;
   pix = NULL;
   p_ast = NULL;
   compress_type = BUFCOMPRESS_NONE;
   fitsextension = new char[CHAREXTENSION];

   strcpy(fitsextension,".fit");

   pthread_mutexattr_init(&mutexAttr);
   pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&mutex, &mutexAttr);

   /* Cr�e une chaine vide, pour �viter un pb potentiel sur le strlen(temporaryRawFileName) de FreeBuffer() */
   strcpy(temporaryRawFileName, "");

   // On fait de la place avant de recreer le contenu du buffer
   FreeBuffer(DONT_KEEP_KEYWORDS);

   // je cree un tableau de pixels par defaut
   SetPixels(PLANE_GREY,0,0,FORMAT_FLOAT,COMPRESS_NONE, NULL, 0, 0, 0);

   initialMipsLo = 0;
   initialMipsHi = 0;
}


CBuffer::~CBuffer()
{
   pthread_mutexattr_destroy(&mutexAttr);
   pthread_mutex_destroy(&mutex);

   if (fitsextension != NULL) {
      delete[] fitsextension;
   }
   FreeBuffer(FREE_KEYWORDS);
}

CBuffer * CBuffer::Chercher(int bufNo) {
   return (CBuffer *) buf_pool->Chercher(bufNo);
}

void CBuffer::SetSavingType(int st)
{
   switch(st) {
      case BYTE_IMG:
      case SHORT_IMG:
      case USHORT_IMG:
      case LONG_IMG:
      case ULONG_IMG:
      case FLOAT_IMG:
      case DOUBLE_IMG:
         saving_type = st;
         break;
      default:
         saving_type = SHORT_IMG;
   }

}

void CBuffer::SetCompressType(int st)
{
   switch(st) {
      case BUFCOMPRESS_NONE:
      case BUFCOMPRESS_GZIP:
         compress_type = st;
         break;
      default:
         compress_type = BUFCOMPRESS_NONE;
   }
}

void CBuffer::SetExtension(char *ext)
{
   char ext0[CHAREXTENSION];
   if (ext==NULL) {
      strcpy(ext0,"");
   }
   strcpy(fitsextension,"");
   strcpy(ext0,ext);
   if ((strcmp(ext0,"*")==0)||(strcmp(ext0,"")==0)) {
      // cas ou l'on ne veut pas d'extension
      return;
   }
   if (ext0[0]=='?') {
      strcat(fitsextension,".fit");
      return;
   }
   if (ext0[0]!='.') {
      strcat(fitsextension,".");
   }
   strcat(fitsextension,ext0);
}

int CBuffer::GetSavingType()
{
   return saving_type;
}

char *CBuffer::GetExtension()
{
   static char ext[CHAREXTENSION];
   strcpy(ext,fitsextension);
   return ext;
}

int CBuffer::GetCompressType()
{
   return compress_type;
}

/**
 *----------------------------------------------------------------------
 * IsPixelsReady
 *    informe si une image est presente dans le buffer
 *
 * Parameters:
 *    none
 * Results:
 *    returns 1 if pixels are ready, otherwise 0.
 * Side effects:
 *    appelle pix->IsPixelsReady()
 *----------------------------------------------------------------------
 */
int CBuffer::IsPixelsReady(void) {
   return pix->IsPixelsReady();
}


void CBuffer::FreeBuffer(int keep_keywords)
{
   if (keep_keywords == FREE_KEYWORDS) {
      if(keywords)
         delete keywords;
         keywords = NULL;
      if(p_ast) {
         free(p_ast);
         p_ast = NULL;
      }
      return;
   }

   if (keep_keywords == DONT_KEEP_KEYWORDS) {
      if(keywords) {
         delete keywords;
         keywords = NULL;
      }
      // je cree un objet CFitsKeywords vide
      keywords = new CFitsKeywords();
      if(p_ast) {
         free(p_ast);
         p_ast = NULL;
      }
      // je cree un objet mc_ASTROM vide
      p_ast = (mc_ASTROM*)calloc(1,sizeof(mc_ASTROM));
   }

   // j'efface le fichier temporaire de l'image RAW
   if( strlen(temporaryRawFileName) > 0 ) {
      remove(temporaryRawFileName);
      strcpy(temporaryRawFileName, "");
   }

   // je cree un tableau de dimension nulle
   SetPixels(PLANE_GREY,0,0,FORMAT_FLOAT,COMPRESS_NONE,  NULL, 0, 0, 0);
}


void CBuffer::GetDataType(TDataType *dt)
{
   *dt = dt_Float;
}

/**
 *  lecture d'un fichier au format FIT
 *
 */
void CBuffer::LoadFile(char *filename)
{
   CPixels *pixels;
   CFitsKeywords *kwds;
   CFitsKeyword *k;

   try {
      pthread_mutex_lock(&mutex);
      FreeBuffer(DONT_KEEP_KEYWORDS);

      // je charge le fichier
      CFile::loadFile(filename, TFLOAT, &pixels, &kwds);

      // je verifie la presence du mot cle naxis
      k = kwds->FindKeyword("NAXIS");
      if(k == NULL  ) {
         throw CError("LoadFile error : keyword NAXIS not found");
      }

      // je memorise les pixels
      if (this->pix != NULL) {
         delete this->pix;
      }
      this->pix = pixels;

      // je memorise les mots cles
      this->keywords = kwds;

      // je sauvegarde les seuils initiaux et je convertis les seuils en float
      k = this->keywords->FindKeyword("MIPS-HI");
      if(k != NULL  ) {
         initialMipsHi = k->GetFloatValue();
      } else {
         initialMipsHi = 0;
      }

      k = this->keywords->FindKeyword("MIPS-LO");
      if(k) {
         initialMipsLo = k->GetFloatValue();
      } else {
         initialMipsLo = 0;
      }
      pthread_mutex_unlock(&mutex);
   } catch (const CError& e) {
      // je libere le mutex
      pthread_mutex_unlock(&mutex);
      // je transmet l'erreur
      throw e;
   }

}

/**
 *  lecture d'un fichier au format FIT
 *
 */
void CBuffer::LoadFits(char *filename)
{
   int msg;                         // Code erreur de libtt
   int datatype;                    // Type du pointeur de l'image
   int naxis1,naxis2,naxis3;
   int nb_keys;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;
   CFitsKeyword *k;
   TYPE_PIXELS *ppix=NULL;


   try {
      int iaxis3 = 3;

      FreeBuffer(DONT_KEEP_KEYWORDS);

      // Chargement proprement dit de l'image.
      datatype = TFLOAT;

      msg = Libtt_main(TT_PTR_LOADIMA3D,13,filename,&datatype,&iaxis3,&ppix,&naxis1,&naxis2,&naxis3,
         &nb_keys,&keynames,&values,&comments,&units,&datatypes);
      if(msg) throw CErrorLibtt(msg);

      if(naxis3 == 1) {
         SetPixels(PLANE_GREY, naxis1, naxis2, FORMAT_FLOAT, COMPRESS_NONE, ppix, 0, 0, 0);
         // je recupere les mots cles
         keywords->GetFromArray(nb_keys,&keynames,&values,&comments,&units,&datatypes);
      } else if ( naxis3 == 3 ) {
         // s'il y a 3 plans , je charge les deux autres plans
         TYPE_PIXELS * ppixR = NULL;
         TYPE_PIXELS * ppixG = NULL ;
         TYPE_PIXELS * ppixB = ppix ;  // j'ai deja recupere le troisieme plan

         iaxis3 = 1;
         msg = Libtt_main(TT_PTR_LOADIMA3D,13,filename,&datatype,&iaxis3,&ppixR,&naxis1,&naxis2,&naxis3,
            &nb_keys,&keynames,&values,&comments,&units,&datatypes);
         iaxis3 = 2;
         msg = Libtt_main(TT_PTR_LOADIMA3D,13,filename,&datatype,&iaxis3,&ppixG,&naxis1,&naxis2,&naxis3,
            &nb_keys,&keynames,&values,&comments,&units,&datatypes);
         // je cree le tableau de pixels
         pix = new CPixelsRgb(naxis1, naxis2, FORMAT_FLOAT, ppixR, ppixG, ppixB);
         // je recupere les mots cles
         keywords->GetFromArray(nb_keys,&keynames,&values,&comments,&units,&datatypes);
         naxis3 = 3;
         keywords->Add("NAXIS", &naxis3,TINT,"","");
         naxis3 = 3;
         keywords->Add("NAXIS3",&naxis3,TINT,"","");

         // je sauvegarde les seuils initiaux et je convertis les seuils en float
         k = keywords->FindKeyword("MIPS-HI");
         if(k != NULL  ) {
            if( k->GetDatatype() == TINT ) {
               // je convertis le seuil en float
               initialMipsHi = (float) k->GetIntValue();
               keywords->Add("MIPS-HI",(char *) &initialMipsHi,TFLOAT,"Hight cut","ADU");
            } else {
               initialMipsHi = (float) k->GetFloatValue();
            }
         }

         k = keywords->FindKeyword("MIPS-LO");
         if(k) {
            if( k->GetDatatype() == TINT ) {
               // je convertis le seuil en float
               initialMipsLo = k->GetFloatValue();
               keywords->Add("MIPS-LO",&initialMipsLo,TFLOAT,"Low cut","ADU");
            } else {
               initialMipsLo = (float) k->GetFloatValue();
            }
         }
      } else {
         throw CError("LoadFits error: plane number is not 1 or 3.");
      }

      // je verifie la presence du mot cle naxis
      k = keywords->FindKeyword("NAXIS");
      if( k != NULL) {
         throw new CError(ELIBSTD_NO_NAXIS_KWD);
      }

      // On reinitialise les parametres astrometriques
      pthread_mutex_lock(&mutex);
      p_ast->valid = 0;
      saving_type = keywords->FindKeyword("BITPIX")->GetIntValue();
      if(saving_type==16) {
         k = keywords->FindKeyword("BZERO");
         if((k)&&(k->GetIntValue()==32768)) {
            saving_type = USHORT_IMG;
         }
      } else if(saving_type==32) {
         k = keywords->FindKeyword("BZERO");
         if((k)&&(k->GetIntValue()==-(2^31))) {
            saving_type = ULONG_IMG;
         }
      }

      // je sauvegarde les seuils initiaux et je convertis les seuils en float
      k = keywords->FindKeyword("MIPS-HI");
      if(k != NULL  ) {
         if( k->GetDatatype() == TINT ) {
            // je convertis le seuil en float
            initialMipsHi = (float) k->GetIntValue();
            keywords->Add("MIPS-HI",(char *) &initialMipsHi,TFLOAT,"Hight cut","ADU");
         } else {
            initialMipsHi = (float) k->GetFloatValue();
         }
      }

      k = keywords->FindKeyword("MIPS-LO");
      if(k) {
         if( k->GetDatatype() == TINT ) {
            // je convertis le seuil en float
            initialMipsLo = k->GetFloatValue();
            keywords->Add("MIPS-LO",&initialMipsLo,TFLOAT,"Low cut","ADU");
         } else {
            initialMipsLo = (float) k->GetFloatValue();
         }
      }
      pthread_mutex_unlock(&mutex);

   } catch (const CError& e) {
      pthread_mutex_unlock(&mutex);

      // Liberation de la memoire allou?e par libtt
      Libtt_main(TT_PTR_FREEPTR,1,&ppix);
      Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      // je transmet l'erreur
      throw e;
   }

   // Liberation de la memoire allouee par libtt (pas n?cessire de catcher les erreurs)
   msg = Libtt_main(TT_PTR_FREEPTR,1,&ppix);
   msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
}

/**
 *  CBuffer::Create3d
 *  cree un fichier F
 *
 *   @param filename : nom du fichier
 *   @param init     : =1 initialiser le buffer la premiere fois ou bien =0 pour compl?ter
 *   @param nbtot    : nombre d'images
 *   @param index    : index de l'image
 *   @param *naxis10 : valeur de naxis1 de l'image initiale (retourne si init=0, sinon a passer en parametre d'entr?e)
 *   @param *naxis20 : valeur de naxis2 de l'image initiale (retourne si init=0, sinon a passer en parametre d'entr?e)
 *   @param errcode  : code erreur (=0 si pas d'erreur)
 *
 */
void CBuffer::Create3d(char *filename,int init, int nbtot, int index,int *naxis10, int *naxis20, int *errcode)
{
   int msg;                         // Code erreur de libtt
   int datatype;                    // Type du pointeur de l'image
   int naxis1,naxis2,naxis3;
   int nb_keys;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;
   CFitsKeyword *k;
   TYPE_PIXELS *ppix=NULL;
   int iaxis3=0,kx,ky;

   if (init==1) {
      FreeBuffer(DONT_KEEP_KEYWORDS);
   }

   // Chargement proprement dit de l'image.
   *errcode=0;
   datatype = TFLOAT;
   msg = Libtt_main(TT_PTR_LOADIMA3D,13,filename,&datatype,&iaxis3,&ppix,&naxis1,&naxis2,&naxis3,
   	&nb_keys,&keynames,&values,&comments,&units,&datatypes);
   if(msg) throw CErrorLibtt(msg);

   // Recopie de pixels dans le buffer
   try {
      if (init==1) {
         pix = new CPixelsGray(naxis1, naxis2*nbtot, FORMAT_FLOAT, NULL, 0, 0);
         *naxis10=naxis1;
         *naxis20=naxis2;
      }
      if (*naxis10!=naxis1) {
         msg = Libtt_main(TT_PTR_FREEPTR,1,&ppix);
         msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
         *errcode=1;
         return;
      }
      if (*naxis20!=naxis2) {
         msg = Libtt_main(TT_PTR_FREEPTR,1,&ppix);
         msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
         *errcode=2;
         return;
      }
      for (kx=0;kx<naxis1;kx++) {
         for (ky=0;ky<naxis2;ky++) {
            pix->SetPix(PLANE_GREY, ppix[naxis1*ky+kx],kx,ky+index*naxis2);
         }
      }
   } catch (const CError& e) {
      // Liberation de la memoire allouee par libtt
      Libtt_main(TT_PTR_FREEPTR,1,&ppix);
      Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      // je transmet l'erreur
      throw e;
   }
   // On recupere les mots cles
   if (init==1) {
      keywords->GetFromArray(nb_keys,&keynames,&values,&comments,&units,&datatypes);
   }

   // On reinitialise les parametres astrometriques
   if (init==1) {
      p_ast->valid = 0;
      saving_type = keywords->FindKeyword("BITPIX")->GetIntValue();
      if(saving_type==16) {
         k = keywords->FindKeyword("BZERO");
         if((k)&&(k->GetIntValue()==32768)) {
            saving_type = USHORT_IMG;
         }
      } else if(saving_type==32) {
         k = keywords->FindKeyword("BZERO");
         if((k)&&(k->GetIntValue()==-(2^31))) {
            saving_type = ULONG_IMG;
         }
      }
   }
   // Liberation de la memoire allouee par libtt
   msg = Libtt_main(TT_PTR_FREEPTR,1,&ppix);
   msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
}


/**
 *  lecture d'un fichier qui n'est pas au format FIT
 *
 *   @param filename : nom du fichier
 *   @param i
 *
 */
void CBuffer::Load3d(char *filename,int iaxis3)
{
   int msg;                         // Code erreur de libtt
   int datatype;                    // Type du pointeur de l'image
   int naxis1,naxis2,naxis3;
   int nb_keys;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;
   CFitsKeyword *k;
   TYPE_PIXELS *ppix=NULL;

   FreeBuffer(DONT_KEEP_KEYWORDS);

   // Chargement proprement dit de l'image.
   datatype = TFLOAT;
   msg = Libtt_main(TT_PTR_LOADIMA3D,13,filename,&datatype,&iaxis3,&ppix,&naxis1,&naxis2,&naxis3,
   	&nb_keys,&keynames,&values,&comments,&units,&datatypes);
   if(msg) throw CErrorLibtt(msg);

   // Recopie de pixels dans le buffer
   try {
      SetPixels(PLANE_GREY, naxis1, naxis2, FORMAT_FLOAT, COMPRESS_NONE, ppix, 0, 0, 0);
   } catch (const CError& e) {
      // Liberation de la memoire allouee par libtt
      Libtt_main(TT_PTR_FREEPTR,1,&ppix);
      Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      // je transmet l'erreur
      throw e;
   }
   // On recupere les mots cles
   keywords->GetFromArray(nb_keys,&keynames,&values,&comments,&units,&datatypes);

   // On reinitialise les parametres astrometriques
   p_ast->valid = 0;
   saving_type = keywords->FindKeyword("BITPIX")->GetIntValue();
   if(saving_type==16) {
      k = keywords->FindKeyword("BZERO");
      if((k)&&(k->GetIntValue()==32768)) {
         saving_type = USHORT_IMG;
      }
   } else if(saving_type==32) {
      k = keywords->FindKeyword("BZERO");
      if((k)&&(k->GetIntValue()==-(2^31))) {
         saving_type = ULONG_IMG;
      }
   }

   // Liberation de la memoire allouee par libtt
   msg = Libtt_main(TT_PTR_FREEPTR,1,&ppix);
   msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
}

/**
 *----------------------------------------------------------------------
 * SaveFits
 *    sauvegarde le buffer dans fichier au format fits
 *
 * Parameters:
 *    filename : nom du ficher de sortie
 * Results:
 *    none
 * Side effects:
 *    enregistre l'image (niveaux de gris), et les mots cles
 *    avec la fonction SAVEIMA de libtt
 *----------------------------------------------------------------------
 */
void CBuffer::SaveFits(char *filename)
{
   int datatype;                    // Type du pointeur de l'image
   CFitsKeyword *k = NULL;
   float fHi, fLo;
   int iHi, iLo;

   // Je transforme les seuils en entier si saving_type vaut byte, short ou ushort
   // Si les mots-cles n'existent pas, alors on ne fait rien. Merci Jacques !
   if( saving_type == BYTE_IMG || saving_type == SHORT_IMG || saving_type == USHORT_IMG ) {
      k = keywords->FindKeyword("MIPS-HI");
      if (k != NULL) {
         fHi = k->GetFloatValue();
         iHi = (int) fHi;
         keywords->Add("MIPS-HI",(void*)&iHi,TINT,"High cut","ADU");
      }
      k = keywords->FindKeyword("MIPS-LO");
      if (k != NULL) {
         fLo = k->GetFloatValue();
         iLo = (int) fLo;
         keywords->Add("MIPS-LO",(void*)&iLo,TINT,"Low cut","ADU");
      }
   }

   keywords->Add("BITPIX",&this->saving_type,TINT,"","");

   // j'enregistre l'image dans le fichier
   datatype=TFLOAT;
   CFile::saveFits(filename, datatype,this->pix,this->keywords);

   // je restaure les valeur reelles des seuils
   if( saving_type == BYTE_IMG || saving_type == SHORT_IMG || saving_type == USHORT_IMG ) {
      k = keywords->FindKeyword("MIPS-HI");
      if (k!=NULL) {
         keywords->Add("MIPS-HI",(void*)&fHi,TFLOAT,"High cut","ADU");
      }
      k = keywords->FindKeyword("MIPS-LO");
      if (k!=NULL) {
         keywords->Add("MIPS-LO",(void*)&fLo,TFLOAT,"Low cut","ADU");
      }
   }
}


void CBuffer::Save1d(char *filename,int iaxis2)
{
   int msg;                         // Code erreur de libtt
   int datatype;                    // Type du pointeur de l'image
   int bitpix;
   int naxis1,naxis2,nnaxis1;
   int nb_keys;
   int x1,x2,y1,y2;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;
   TYPE_PIXELS *ppix1=NULL;

   datatype=TFLOAT;
   bitpix = saving_type;

   naxis1  = GetWidth();
   naxis2  = GetHeight();
   if (iaxis2<0) {
      iaxis2=0;
   }
   nnaxis1=naxis1;
   x1 = 0;
   y1 = 0;
   x2 = 0;  //for gcc warning: ?x2? may be used uninitialized in this function
   y2 = 0;
   if (pix->getPixelClass()==CLASS_GRAY) {
      if (naxis1==1) {
         x1=0;
         x2=0;
         y1=0;
         y2=naxis2-2;
         nnaxis1=naxis2;
      } else if (naxis2==1) {
         y1=0;
         y2=0;
         x1=0;
         x2=naxis1-1;
         nnaxis1=naxis1;
      } if  (iaxis2>0 ) {
         if (iaxis2>=naxis2) {
            iaxis2=naxis2-1;
         }
         y1=iaxis2;
         y2=iaxis2;
         x1=0;
         x2=naxis1-1;
         nnaxis1=naxis1;
      }
      ppix1 = (TYPE_PIXELS *) malloc(nnaxis1 * sizeof(float));
      // Yassine
      // pix->GetPixels(x1, y1, x2, y2, FORMAT_FLOAT, PLANE_RGB, (int) ppix1);
      pix->GetPixels(x1, y1, x2, y2, FORMAT_FLOAT, PLANE_RGB, (void*) ppix1);
   } else {
      throw CError("save1d not implemented for RGB image");
   }

   // Collecte de renseignements pour la suite
   nb_keys = keywords->GetKeywordNb();

   // Allocation de l'espace memoire pour les tableaux de mots-cles
   if (nb_keys>0) {
   msg = Libtt_main(TT_PTR_ALLOKEYS,6,&nb_keys,&keynames,&values,&comments,&units,&datatypes);
      if(msg) {
         free(ppix1);
         throw CErrorLibtt(msg);
      }
   }

   // Conversion keywords vers tableaux 'Made in Klotz'
   keywords->SetToArray(&keynames,&values,&comments,&units,&datatypes);

   // Enregistrement proprement dit de l'image.
   msg = Libtt_main(TT_PTR_SAVEIMA1D,11,filename,ppix1,&datatype,&nnaxis1,
      &bitpix,&nb_keys,keynames,values,comments,units,datatypes);
   if(msg) {
      Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      free(ppix1);
      throw CErrorLibtt(msg);
   }

   // Liberation de la memoire allouee par libtt
   if (nb_keys>0) {
      msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      if(msg) {
         free(ppix1);
         throw CErrorLibtt(msg);
      }
   }

   free(ppix1);

}

void CBuffer::Save3d(char *filename,int naxis3,int iaxis3_beg,int iaxis3_end)
{
   int msg;                         // Code erreur de libtt
   int datatype;                    // Type du pointeur de l'image
   int bitpix;
   int naxis1,naxis2,nnaxis2,nnaxis3;
   int nb_keys;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;
   TYPE_PIXELS *ppix=NULL;
   TYPE_PIXELS *ppix1=NULL;
   int dummy;

   datatype=TFLOAT;
   bitpix = saving_type;

   naxis1  = GetWidth();
   naxis2  = GetHeight();
   if (pix->getPixelClass()==CLASS_GRAY) {
      nnaxis2 = naxis2/(iaxis3_end-iaxis3_beg+1);
   } else {
      nnaxis2 = naxis2;
   }

   if (pix->getPixelClass()==CLASS_GRAY) {
      ppix1 = (TYPE_PIXELS *) malloc(naxis1* naxis2 * sizeof(float));
      pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_RGB, (void*) ppix1);
   } else {
      naxis3 = 3;
      ppix1 = (TYPE_PIXELS *) malloc(naxis1* naxis2 * naxis3 * sizeof(float));
      // Yassine
      //pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_R, (int) ppix1);
      //pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_G, (int) ppix1+naxis1*naxis2*sizeof(float));
      //pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_B, (int) ppix1+2*naxis1*naxis2*sizeof(float));
      pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_R, (void*) ppix1);
      pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_G, (void*) (ppix1+naxis1*naxis2));
      pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_B, (void*) (ppix1+2*naxis1*naxis2));
   }

   // Collecte de renseignements pour la suite
   nb_keys = keywords->GetKeywordNb();

   // Allocation de l'espace memoire pour les tableaux de mots-cles
   if (nb_keys>0) {
   msg = Libtt_main(TT_PTR_ALLOKEYS,6,&nb_keys,&keynames,&values,&comments,&units,&datatypes);
      if(msg) {
         free(ppix1);
         throw CErrorLibtt(msg);
      }
   }

   // Conversion keywords vers tableaux 'Made in Klotz'
   keywords->SetToArray(&keynames,&values,&comments,&units,&datatypes);

   // Enregistrement proprement dit de l'image.
   if ((naxis3<=0)||(nnaxis2<1)) {
      msg = Libtt_main(TT_PTR_SAVEIMA,12,filename,ppix1,&datatype,&naxis1,&naxis2,
         &bitpix,&nb_keys,keynames,values,comments,units,datatypes);
   } else {
      ppix=(TYPE_PIXELS*)(ppix1+(iaxis3_beg-1)*naxis1*nnaxis2);
      if (iaxis3_end<iaxis3_beg) { dummy=iaxis3_beg; iaxis3_beg=iaxis3_end; iaxis3_end=iaxis3_beg;}
      if (iaxis3_beg<1) { iaxis3_beg=1; }
      if (iaxis3_end>naxis3) { iaxis3_end=naxis3; }
      nnaxis3=iaxis3_end-iaxis3_beg+1;
      msg = Libtt_main(TT_PTR_SAVEIMA3D,13,filename,ppix,&datatype,&naxis1,&nnaxis2,&nnaxis3,
         &bitpix,&nb_keys,keynames,values,comments,units,datatypes);
   }
   if(msg) {
      Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      free(ppix1);
      throw CErrorLibtt(msg);
   }

   // Liberation de la memoire allouee par libtt
   if (nb_keys>0) {
   msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      if(msg) {
         free(ppix1);
         throw CErrorLibtt(msg);
      }
   }

   free(ppix1);

}

void CBuffer::SaveJpg(char *filename,int quality,int sbsh, double sb,double sh)
{
   int msg;                         // Code erreur de libtt
   int datatype;                    // Type du pointeur de l'image
   int naxis1,naxis2;
   CFitsKeyword *k;
   TYPE_PIXELS *ppix=NULL;

   datatype=TFLOAT;

   naxis1 = GetWidth();
   naxis2 = GetHeight();
   ppix = (TYPE_PIXELS *) malloc(naxis1* naxis2 * sizeof(float));
   pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_GREY, (void*) ppix);

   // Collecte de renseignements pour la suite
   if (sbsh==0) {
      if(keywords) {
         k = keywords->FindKeyword("MIPS-LO");
         if(k) {
            sb = k->GetFloatValue();
         } else {
            sb=0.0;
         }
         k = keywords->FindKeyword("MIPS-HI");
         if(k) {
            sh = k->GetFloatValue();
         } else {
            sh=32767.0;
         }
      }
   }

   // Enregistrement proprement dit de l'image au format Jpeg
   msg = Libtt_main(TT_PTR_SAVEJPG,8,filename,ppix,&datatype,&naxis1,&naxis2,
   		&sb,&sh,&quality);
   free(ppix);
}

void CBuffer::SaveJpg(char *filename, int quality, unsigned char *palette[3], int mirrorx, int mirrory) {
   unsigned char * buf256 = NULL;
   int width, height, planes;
   CFitsKeyword * kwd;
   float cuts[6];

   // je recupere la taille
   width = GetWidth();
   height = GetHeight();
   planes = this->pix->GetPlanes();

   cuts[0] = 32767;
   cuts[2] = 32767;
   cuts[4] = 32767;
   cuts[1] = 0;
   cuts[3] = 0;
   cuts[5] = 0;

   if ( planes == 1 ) {
      // je fabrique des seuils par defaut
      kwd = keywords->FindKeyword((char*)"MIPS-HI");
      if (kwd != NULL ) {
         cuts[0] = kwd->GetFloatValue();
         cuts[2] = kwd->GetFloatValue();
         cuts[4] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-LO");
      if (kwd != NULL ) {
         cuts[1] = kwd->GetFloatValue();
         cuts[3] = kwd->GetFloatValue();
         cuts[5] = kwd->GetFloatValue();
      } 
   } else {
      kwd = keywords->FindKeyword((char*)"MIPS-HI");
      if (kwd != NULL ) {
         cuts[0] = kwd->GetFloatValue();
         cuts[2] = kwd->GetFloatValue();
         cuts[4] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-LO");
      if (kwd != NULL ) {
         cuts[1] = kwd->GetFloatValue();
         cuts[3] = kwd->GetFloatValue();
         cuts[5] = kwd->GetFloatValue();
      } 

      // je fabrique des seuils par defaut
      kwd = keywords->FindKeyword((char*)"MIPS-HIR");
      if (kwd != NULL ) { 
         cuts[0] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-HIG");
      if (kwd != NULL ) { 
         cuts[2] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-HIB");
      if (kwd != NULL ) { 
         cuts[4] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-LOR");
      if (kwd != NULL ) { 
         cuts[1] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-LOG");
      if (kwd != NULL ) { 
         cuts[3] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-LOB");
      if (kwd != NULL ) { 
         cuts[5] = kwd->GetFloatValue();
      } 
   }
   // je cree le buffer pour preparer l'image RGB a 256 niveaux
   buf256 = (unsigned char *) calloc(width*height*3,sizeof(unsigned char));
   if (buf256==NULL) {
      throw CError("saveJpeg : not enouth memory for calloc ");
   }

   // je transforme l'image sur 256 niveaux
   this->pix->GetPixelsRgb(
      0, 0, width-1, height-1,   // size
      mirrorx, mirrory,          // mirror
      cuts,                      // cuts
      palette,                   // palette
      buf256);

   // je cree un seul plan de couleur pour enregistrer une image JPEG N&B
   if ( planes == 1 ) {
      for(int j=0;j<=height-1;j++) {
         for(int i=0;i<=width-1;i++) {
            buf256[j*width+i] = buf256[(j*width+i)*3];
         }
      }
   }

   // j'enregistre l'image dans le fichier
   CFile::saveJpeg(filename, buf256, this->keywords, planes, width, height, quality);


   if ( buf256 != NULL) { free(buf256); }

}

/**
 *  SaveTkImg
 *  Sauvegarde une image BMP, GIF, PNG, TIF
 *    
 *  @param filename   : nom du fichier de l'image
 *  @param palette[3] : palette de couleur ( 3 tableaux de 256 octets)
 *  @param mirrorx    : 0= pas miroir horizontal, 1=miroir horizontal
 *  @param mirrory    : 0= pas miroir vertical , 1=miroir vertical
 *  @return void
 *  @exception  retourne une exception CError en cas d'erreur
 *    
 */
void CBuffer::SaveTkImg(char *filename, unsigned char *palette[3], int mirrorx, int mirrory) {
   unsigned char * buf256;
   int width, height, planes;
   float cuts[6]; 
   CFitsKeyword *kwd;

   // je recupere la taille
   width  = pix->GetWidth();
   height = pix->GetHeight();
   planes = pix->GetPlanes();
   cuts[0] = 255;
   cuts[2] = 255;
   cuts[4] = 255;
   cuts[1] = 0;
   cuts[3] = 0;
   cuts[5] = 0;

   // je fabrique des seuils par defaut
   switch ( this->pix->getPixelClass() ) {
   case CLASS_GRAY :
      kwd = keywords->FindKeyword((char*)"MIPS-HI");
      if (kwd != NULL ) {
         cuts[0] = kwd->GetFloatValue();
         cuts[2] = kwd->GetFloatValue();
         cuts[4] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-LO");
      if (kwd != NULL ) {
         cuts[1] = kwd->GetFloatValue();
         cuts[3] = kwd->GetFloatValue();
         cuts[5] = kwd->GetFloatValue();
      }
      break;
   case CLASS_RGB :
      kwd = keywords->FindKeyword((char*)"MIPS-HIR");
      if (kwd != NULL ) {
         cuts[0] = kwd->GetFloatValue();
      }
      kwd = keywords->FindKeyword((char*)"MIPS-HIG");
      if (kwd != NULL ) {
         cuts[2] = kwd->GetFloatValue();
      }
      kwd = keywords->FindKeyword((char*)"MIPS-HIB");
      if (kwd != NULL ) {
         cuts[4] = kwd->GetFloatValue();
      } 
      kwd = keywords->FindKeyword((char*)"MIPS-LOR");
      if (kwd != NULL ) {
         cuts[1] = kwd->GetFloatValue();
      }
      kwd = keywords->FindKeyword((char*)"MIPS-LOG");
      if (kwd != NULL ) {
         cuts[3] = kwd->GetFloatValue();
      }
      kwd = keywords->FindKeyword((char*)"MIPS-LOB");
      if (kwd != NULL ) {
         cuts[5] = kwd->GetFloatValue();
      }

      break;
   }

   // je cree le buffer pour preparer l'image a 256 niveaux
   buf256 = (unsigned char *) calloc(width*height*4,sizeof(unsigned char));
   if (buf256==NULL) {
      throw CError("saveJpeg : not enouth memory for calloc ");
   }

   // je transforme l'image sur 256 niveaux
   this->pix->GetPixelsVisu(
      0, 0, width-1, height-1,   // size
      mirrorx, mirrory,          // mirror
      cuts,                      // cuts
      palette,                   // palette
      buf256);

   // j'enregistre l'image dans le fichier
   CFile::saveTkimg(filename, buf256, width, height, planes);
   free(buf256);

}



void CBuffer::SaveRawFile(char *filename)
{
   size_t sizeWritten;
   FILE * ifp;
   FILE * ofp;
   char data[65535];
   int dataSize;

   if( strlen(temporaryRawFileName) > 0 ) {
      ifp = fopen (temporaryRawFileName, "rb");
      if ( ifp ) {
         ofp = fopen (filename, "wb");
         if (ofp) {
            while ( (dataSize = fread( data, 1, 65535, ifp)) != 0  ) {
               sizeWritten = fwrite (data, 1, dataSize, ofp);
            }
            fclose(ofp);
         } else {
            throw CError("Can not open file %s",filename);
         }
         fclose(ifp);
      } else {
         throw CError("Can not open file %s",temporaryRawFileName);
      }
   } else {
      CError("no raw file");
   }

}

/**
 *  Cfa2Rgb
 *  convertit une image CFA en image RGB
 */

void CBuffer::Cfa2Rgb(int interpolationMethod)
{
   CPixels *rgbPixels;
   CFitsKeywords *rgbKeywords;

   pthread_mutex_lock(&mutex);

   // je convertis les pixles (une exception est levee en cas d'erreur
   CFile::cfa2Rgb(this->pix , this->keywords, interpolationMethod, &rgbPixels, &rgbKeywords);

   // je supprimer l'image CFA
   delete this->pix;
   delete this->keywords;

   // j'affecte l'image RGB
   this->pix = rgbPixels;
   this->keywords = rgbKeywords;
   pthread_mutex_unlock(&mutex);

}


/*
 * int CBuffer::GetNaxis() --
 *  Renvoie le nombre d'axes de l'image
 */
int CBuffer::GetNaxis()
{
   CFitsKeyword *k;
   int value;
   pthread_mutex_lock(&mutex);
   k = this->keywords->FindKeyword("NAXIS");
   if(k != NULL  ) {
      value= k->GetIntValue();
   } else {
      value= 0;
   }
   pthread_mutex_unlock(&mutex);
   return value;
}

/*
 * int CBuffer::GetW() --
 *  Renvoie la largeur de l'image.
 */
int CBuffer::GetWidth()
{
   int width;
   pthread_mutex_lock(&mutex);
   width = pix->GetWidth();
   pthread_mutex_unlock(&mutex);
   return width;
}

/*
 * int CBuffer::GetH() --
 *  Renvoie la hauteur de l'image.
 */
int CBuffer::GetHeight()
{
   int height;
   pthread_mutex_lock(&mutex);
   height = pix->GetHeight();
   pthread_mutex_unlock(&mutex);
   return height;
}


/*
 * tt_atan2
 *   Calcule atan2 sans 'domain error'
 *   Routine issue de libtt, tt_util1 @ 393
 *
 */
double tt_atan2(double y, double x)
{
   if (y==0) {
      if (x>=0) { return(0.); }
      else { return(2.*acos(0.)); }
   } else if (x==0) {
      if (y>=0) { return(acos(0.)); }
      else { return(-acos(0.)); }
   } else {
      return(atan2(y,x));
   }
}

int tt_util_cd2cdelt_new(double cd11,double cd12,double cd21,double cd22,double *cdelt1,double *cdelt2,double *crota2)
/***************************************************************************/
/* Utilitaire qui tranforme les mots wcs de la matrice cd vers les         */
/* vieux mots cle cdelt1, cdelt2 et crota2.                                */
/***************************************************************************/
{
   double cdp1,cdp2,crotap2;
   double deuxpi,pisur2,sina,cosa,sinb,cosb,cosab,sinab,ab,aa,bb,cosr,sinr;
#define TT_PI (atan(1.)*4.)

   deuxpi=2.*(TT_PI);
   pisur2=(TT_PI)/2.;

   aa=fmod(deuxpi+atan2(cd21,cd11),deuxpi);
   bb=fmod(deuxpi+atan2(-cd12,cd22),deuxpi);

   cosa=cos(aa);
   sina=sin(aa);
   cosb=cos(bb);
   sinb=sin(bb);

   /* a-b */
   cosab=cosa*cosb+sina*sinb;
   sinab=sina*cosb-cosa*sinb;
   ab=fabs(atan2(sinab,cosab));

   /* cas |a-b| proche de PI */
   if (ab>pisur2) {
	   if (cosa>cosb) {
         bb=fmod((TT_PI)+bb,deuxpi);
      } else {
         aa=fmod((TT_PI)+aa,deuxpi);
      }
   }

   /* mean (a+b)/2 */
   ab=bb-aa;
   if (ab>TT_PI) {
   	aa=aa+deuxpi;
   }
   ab=aa-bb;
   if (ab>TT_PI) {
   	bb=bb+deuxpi;
   }
   crotap2=fmod(deuxpi+(aa+bb)/2.,deuxpi);

   cosr=fabs(cos(crotap2));
   sinr=fabs(sin(crotap2));

   /* cdelt */
   if (cosr>sinr) {
	   cdp1=cd11/cos(crotap2);
	   cdp2=cd22/cos(crotap2);
   } else {
   	cdp1= cd21/sin(crotap2);
   	cdp2=-cd12/sin(crotap2);
   }

   *cdelt1=cdp1;
   *cdelt2=cdp2;
   *crota2=crotap2;
   return(0);
#undef TT_PI
}

int tt_util_cd2cdelt_old(double cd11,double cd12,double cd21,double cd22,double *cdelt1,double *cdelt2,double *crota2)
/***************************************************************************/
/* Utilitaire qui tranforme les mots wcs de la matrice cd vers les         */
/* vieux mots cle cdelt1, cdelt2 et crota2.                                */
/***************************************************************************/
{
   double cdp1,cdp2,crotap2;
   double deuxpi,pisur2,sina,cosa,sinb,cosb,cosab,sinab,ab,aa,bb,cosr,sinr;
   double signr,signc,signd;
#define TT_PI (atan(1.)*4.)

   deuxpi=2.*(TT_PI);
   pisur2=(TT_PI)/2.;

   aa=fmod(deuxpi+atan2(cd21,cd11),deuxpi);
   bb=fmod(deuxpi+atan2(-cd12,cd22),deuxpi);

   cosa=cos(aa);
   sina=sin(aa);
   cosb=cos(bb);
   sinb=sin(bb);

   /* a-b */
   cosab=cosa*cosb+sina*sinb;
   sinab=sina*cosb-cosa*sinb;
   ab=fabs(atan2(sinab,cosab));

   /* cas |a-b| proche de PI */
   if (ab>pisur2) {
	   if (cosa>cosb) {
         bb=fmod((TT_PI)+bb,deuxpi);
      } else {
         aa=fmod((TT_PI)+aa,deuxpi);
      }
   }

   /* mean (a+b)/2 */
   ab=bb-aa;
   if (ab>TT_PI) {
   	aa=aa+deuxpi;
   }
   ab=aa-bb;
   if (ab>TT_PI) {
   	bb=bb+deuxpi;
   }
   crotap2=fmod(deuxpi+(aa+bb)/2.,deuxpi);

   cosr=fabs(cos(crotap2));
   sinr=fabs(sin(crotap2));

   /* cdelt */
   if (cosr>sinr) {
	   cdp1=cd11/cos(crotap2);
	   cdp2=cd22/cos(crotap2);
   } else {
   	cdp1=fabs(-cd21/sin(crotap2));
   	cdp2=fabs( cd12/sin(crotap2));
      signr=sinr/fabs(sinr);
      /**/
      signc=cd12/fabs(cd12);
      signd=signc/signr;
      if (signd<0) { cdp1*=-1.; }
      /**/
      signc=cd21/fabs(cd21);
      signd=-signc/signr;
      if (signd<0) { cdp2*=-1.; }
   }

   *cdelt1=cdp1;
   *cdelt2=cdp2;
   *crota2=crotap2;
   return(0);
#undef TT_PI
}


/*
 * int CBuffer::FillAstromParams() --
 *   Complete les parametres astrometriques d'une image en fonction de
 *   ceux deja presents, pour les transformations carto/image.
 *
 */
void CBuffer::FillAstromParams()
{
   #define YES 1
   #define NO 0
   #define PI (atan(1.)*4.)
   CFitsKeyword *k;
   // Debut des declarations MC
   char unit[50];
   int crval1found=NO,crval2found=NO,crota2found=NO;
   int crpix1found=NO,crpix2found=NO;
   int valid_optic=0,valid_crvp=0,valid_cd=0;
   int valid_pv=0,k1,k2;
//   int msg;
   double dvalue;
   int result=0;
   // Fin des declarations MC

   p_ast->crota2=0.;
   p_ast->foclen=0.;
   p_ast->px=0.;
   p_ast->py=0.;
   p_ast->crota2=0.;
   p_ast->cd11=0.;
   p_ast->cd12=0.;
   p_ast->cd21=0.;
   p_ast->cd22=0.;
   p_ast->crpix1=0.;
   p_ast->crpix2=0.;
   p_ast->crval1=0.;
   p_ast->crval2=0.;
   p_ast->cdelta1=0.;
   p_ast->cdelta2=0.;
   p_ast->dec0=-100.;
   p_ast->ra0=-100.;
   for (k1=1;k1<=2;k1++) {
      for (k2=0;k2<=10;k2++) {
         p_ast->pv[k1][k2]=0.;
      }
   }
   p_ast->pv[1][1]=1.;
   p_ast->pv[2][1]=1.;
   p_ast->pv_valid=0;

   /* -- recherche des mots cles d'astrometrie '_optic' dans l'entete FITS --*/
   if((k=keywords->FindKeyword("FOCLEN"))!=NULL) {
      valid_optic++;
      dvalue = k->GetDoubleValue();
      if(dvalue>0) {
         p_ast->foclen = dvalue;
      }
   }
   if((k=keywords->FindKeyword("PIXSIZE1"))!=NULL) {
      valid_optic++;
      dvalue = k->GetDoubleValue();
      strcpy(unit,k->GetUnit());
      if(dvalue>0) {
         p_ast->px = dvalue;
      }
      if(strcmp(unit,"m")==0) {
         p_ast->px *= 1.;
      } else if(strcmp(unit,"mm")==0) {
         p_ast->px *= 1e-3;
      } else {
         p_ast->px *= 1e-6;
      }
   }
   if((k=keywords->FindKeyword("PIXSIZE2"))!=NULL) {
      valid_optic++;
      dvalue = k->GetDoubleValue();
      strcpy(unit,k->GetUnit());
      if(dvalue>0) {
         p_ast->py=dvalue;
      }
      if(strcmp(unit,"m")==0) {
         p_ast->py *= 1.;
      } else if(strcmp(unit,"mm")==0) {
         p_ast->py *= 1e-3;
      } else {
         p_ast->py *= 1e-6;
      }
   }
   if((k=keywords->FindKeyword("RA"))!=NULL) {
      valid_optic++;
      dvalue = k->GetDoubleValue();
      strcpy(unit,k->GetUnit());
      if(dvalue>0) {
         p_ast->ra0 = dvalue;
      }
      if((strcmp(unit,"deg")==0)||(strcmp(unit,"degrees")==0)) {
         p_ast->ra0 *= (PI/180.);
      } else if(strcmp(unit,"h")==0) {
         p_ast->ra0 *= (15.*(PI)/180.);
      } else {
         p_ast->ra0 *= (PI/180.);
      }
      p_ast->crval1 = p_ast->ra0;
      crval1found = YES;
   }
   if((k=keywords->FindKeyword("DEC"))!=NULL) {
      valid_optic++;
      dvalue = k->GetDoubleValue();
      strcpy(unit,k->GetUnit());
      if((dvalue>=-90)&&(dvalue<=90)) {
         p_ast->dec0 = dvalue;
      }
      if((strcmp(unit,"deg")==0)||(strcmp(unit,"degrees")==0)) {
         p_ast->dec0 *= (PI/180.);
      } else {
         p_ast->dec0 *= (PI/180.);
      }
      p_ast->crval2=p_ast->dec0;
      crval2found=YES;
   }
   /* -- recherche des mots cles d'astrometrie '_cd' dans l'entete FITS --*/
   if((k=keywords->FindKeyword("CD1_1"))!=NULL) {
      valid_cd++;
      p_ast->cd11 = k->GetDoubleValue() * (PI) / 180.;
      if((k=keywords->FindKeyword("CD1_2"))!=NULL) {
         valid_cd++;
         p_ast->cd12 = k->GetDoubleValue() * (PI) / 180.;
      }
      if((k=keywords->FindKeyword("CD2_1"))!=NULL) {
         valid_cd++;
         p_ast->cd21 = k->GetDoubleValue() * (PI) / 180.;
      }
      if((k=keywords->FindKeyword("CD2_2"))!=NULL) {
         valid_cd++;
         p_ast->cd22 = k->GetDoubleValue() * (PI) / 180.;
      }
   }
   /* -- recherche des mots cles d'astrometrie '_crvp' dans l'entete FITS --*/
   if((k=keywords->FindKeyword("CDELT1"))!=NULL) {
      valid_crvp++;
      p_ast->cdelta1 = k->GetDoubleValue();
      strcpy(unit,k->GetUnit());
      if(strcmp(unit,"deg/pixel")==0) {
         p_ast->cdelta1 *= (PI/180.);
      } else {
         p_ast->cdelta1 *= (PI/180.);
      }
   }
   if((k=keywords->FindKeyword("CDELT2"))!=NULL) {
      valid_crvp++;
      p_ast->cdelta2 = k->GetDoubleValue();
      strcpy(unit,k->GetUnit());
      if(strcmp(unit,"deg/pixel")==0) {
         p_ast->cdelta2 *= (PI/180.);
      } else {
         p_ast->cdelta2 *= (PI/180.);
      }
   }
   if((k=keywords->FindKeyword("CROTA2"))!=NULL) {
      crota2found=YES;
      valid_crvp++;
      valid_optic++;
      p_ast->crota2 = k->GetDoubleValue();
      strcpy(unit,k->GetUnit());
      if(strcmp(unit,"deg")==0) {
         p_ast->crota2 *= (PI/180.);
      } else if(strcmp(unit,"radian")==0) {
         p_ast->crota2 *= (1.);
      } else {
         p_ast->crota2 *= (PI/180.);
      }
   }
   /* -- recherche des mots cles d'astrometrie '_crvp' '_cd' '_optic' dans l'entete FITS --*/
   if((k=keywords->FindKeyword("CRPIX1"))!=NULL) {
      crpix1found=YES;
      valid_crvp++;
      valid_cd++;
      valid_optic++;
      p_ast->crpix1 = k->GetDoubleValue();
   }
   if((k=keywords->FindKeyword("CRPIX2"))!=NULL) {
      crpix2found=YES;
      valid_crvp++;
      valid_cd++;
      valid_optic++;
      p_ast->crpix2 = k->GetDoubleValue();
   }
   if((k=keywords->FindKeyword("CRVAL1"))!=NULL) {
      valid_crvp++;
      valid_cd++;
      if(crval1found==NO) {
         valid_optic++;
      }
      dvalue = k->GetDoubleValue();
      if(dvalue>0) {
         p_ast->crval1 = dvalue;
      }
      if((strcmp(unit,"deg")==0)||(strcmp(unit,"degrees")==0)) {
         p_ast->crval1 *= (PI/180.);
      } else if(strcmp(unit,"h")==0) {
         p_ast->crval1 *= (15.*(PI)/180.);
      } else {
         p_ast->crval1 *= (PI/180.);
      }
      if(dvalue<=0) {
         p_ast->crval1 = p_ast->ra0;
      }
   } else {
      p_ast->crval1 = p_ast->ra0;
   }
   if((k=keywords->FindKeyword("CRVAL2"))!=NULL) {
      valid_crvp++;
      valid_cd++;
      dvalue = k->GetDoubleValue();
      if(crval2found==NO) {
         valid_optic++;
      }
      if((dvalue>=-90)&&(dvalue<=90)) {
         p_ast->crval2 = dvalue;
      }
      if((strcmp(unit,"deg")==0)||(strcmp(unit,"degrees")==0)) {
         p_ast->crval2 *= (PI/180.);
      } else {
         p_ast->crval2 *= (PI/180.);
      }
   } else {
      p_ast->crval2 = p_ast->dec0;
   }

   /* --- complete les validites --- */
   if(crota2found==NO) {
      p_ast->crota2 = 0.;
      valid_optic++;
      valid_crvp++;
   }
   if((crpix1found==NO)) {
      p_ast->crpix1 = p_ast->naxis1/2.;
      valid_optic++;
   }
   if ((crpix2found==NO)) {
      p_ast->crpix2 = p_ast->naxis2/2.;
      valid_optic++;
   }
   /* -- condition de validite --- */
   if((valid_optic>=8)||(valid_cd>=8)||(valid_crvp>=7)) {
      // v = YES;
      p_ast->valid = 1;
   } else {
      // v = NO;
      p_ast->valid = 0;
   }
   // *valid=v;

   /* --- transcodage --- */
   if ((valid_optic>=8)&&(valid_crvp<7)) {
      if ((p_ast->foclen!=0)&&(p_ast->px!=0)&&(p_ast->py!=0)) {
         p_ast->cdelta1=-2*atan(p_ast->px/2./p_ast->foclen);
         p_ast->cdelta2=2*atan(p_ast->py/2./p_ast->foclen);
      }
      result=1;
   }

   if (valid_cd>=8) {
      /*
      p_ast->cdelta1 = sqrt(p_ast->cd11*p_ast->cd11+p_ast->cd21*p_ast->cd21);
      det = p_ast->cd11*cos(p_ast->crota2);
      if(det<0) {p_ast->cdelta1 *= -1.;}
      p_ast->cdelta2 = sqrt(p_ast->cd12*p_ast->cd12+p_ast->cd22*p_ast->cd22);
      det = p_ast->cd22*cos(p_ast->crota2);
      if(det<0) {p_ast->cdelta2*=-1.;}
      p_ast->crota2 = tt_atan2((p_ast->cd12-p_ast->cd21),(p_ast->cd11+p_ast->cd22));
      */
      tt_util_cd2cdelt_old(p_ast->cd11,p_ast->cd12,p_ast->cd21,p_ast->cd22,&p_ast->cdelta1,&p_ast->cdelta2,&p_ast->crota2);
      result=1;
   } else if ((valid_optic>=8)||(valid_crvp>=7)) {

      p_ast->cd11 = p_ast->cdelta1*cos(p_ast->crota2);
      p_ast->cd12 = fabs(p_ast->cdelta2)*p_ast->cdelta1/fabs(p_ast->cdelta1)*sin(p_ast->crota2);
      p_ast->cd21 = -fabs(p_ast->cdelta1)*p_ast->cdelta2/fabs(p_ast->cdelta2)*sin(p_ast->crota2);
      p_ast->cd22 = p_ast->cdelta2*cos(p_ast->crota2);
      /*
      p_ast->cd11= p_ast->cdelta1*cos(p_ast->crota2);
      p_ast->cd12=-p_ast->cdelta2*sin(p_ast->crota2);
      p_ast->cd21= p_ast->cdelta1*sin(p_ast->crota2);
      p_ast->cd22= p_ast->cdelta2*cos(p_ast->crota2);
      */
      result=1;
   }

   if ((p_ast->foclen==0.)&&(p_ast->px!=0.)&&(p_ast->py!=0.)&&(p_ast->cdelta1!=0.)&&(p_ast->cdelta2!=0.)) {
      p_ast->foclen=fabs(p_ast->px/2./tan(p_ast->cdelta1/2.));
      p_ast->foclen+=fabs(p_ast->py/2./tan(p_ast->cdelta2/2.));
      p_ast->foclen/=2.;
   }

   valid_pv=0;
   for (k1=1;k1<=2;k1++) {
      for (k2=1;k2<=10;k2++) {
         if((k=keywords->FindKeyword("CRVAL2"))!=NULL) {
            valid_pv++;
         }
      }
   }
   if (valid_pv>=20) {
      for (k1=1;k1<=2;k1++) {
         for (k2=0;k2<=10;k2++) {
            sprintf(unit,"PV%d_%d",k1,k2);
            if((k=keywords->FindKeyword(unit))!=NULL) {
               dvalue = k->GetDoubleValue();
               p_ast->pv[k1][k2]=dvalue;
            }
         }
      }
      p_ast->pv_valid=1;
   }

   if (result!=1) {
      throw CError(ELIBSTD_NO_ASTROMPARAMS);
   }
   #undef YES
   #undef NO
   #undef PI
}

void CBuffer::MergePixels(TColorPlane plane, int pixels)
{

   pix->MergePixels(plane, pixels);

}



/*
 * int CBuffer::SetPix(TYPE_PIXELS val, int x, int y) --
 *  Affecte la valeur du pixel aux coordonnees passes en parametres.
 *  Les valeurs des coordonnees vont de (0,0) a (NAXIS1-1,NAXIS2-1).
 *  La valeur de retour est :
 *    - '1'  si pas de pixels (pix=NULL)
 *    - '-1' si pas de mots-cles
 *    - '-2' si pas de mot-cle 'NAXIS1'
 *
 */
void CBuffer::SetPix(TColorPlane plane, TYPE_PIXELS val,int x, int y)
{
    pix->SetPix(plane, val, x, y);
}


/*
 * SetPixels
 *  Affecte le tableau de pixels dans buffer->pix
 *
 *  Si une erreur survient pendant le traitement, buffer->pix conserve l'ancien tableau de pixels
 *
 *  Return : 0
 *
 */
void CBuffer::SetPixels(TColorPlane plane, int width, int height, TPixelFormat pixelFormat, TPixelCompression compression, void * pixels, long pixelSize, int reverseX, int reverseY) {
   CPixels  * pixTemp = NULL ;

   // j'efface le fichier temporaire de l'image precedente
   if( strlen(temporaryRawFileName) > 0 ) {
      remove(temporaryRawFileName);
      strcpy(temporaryRawFileName, "");
   }

   if( plane == PLANE_GREY) {
      // cas d'une image grise
      switch (compression) {
      case COMPRESS_NONE:
         {
            pixTemp = new CPixelsGray(width, height, pixelFormat, pixels, reverseX, reverseY);
            break;
         }
      case COMPRESS_RAW :
         {
            int result = -1;
            unsigned short * decodedData;
            struct libdcraw_DataInfo dataInfo;

            result = libdcraw_bufferRaw2Cfa((unsigned short*) pixels, pixelSize, &dataInfo, &decodedData);
            if (result == 0 )  {
               char  filter[70];
               int width, height;

               width  = dataInfo.width;
               height = dataInfo.height;
               pixTemp = new CPixelsGray( width, height, FORMAT_SHORT, decodedData, reverseX, reverseY);
               libdcraw_freeBuffer(decodedData);
               // j'enregistre l'image brute RAW dans un fichier temporaire
               // au cas ou je voudrais ensuite l'enregistrer a l'etat brut
               {
                  FILE * ofp;
                  size_t sizeWritten;
                  // Yassine
                  // sprintf(temporaryRawFileName,"rawFileBuf%d.dat",(int)this);
                  sprintf(temporaryRawFileName,"rawFileBuf%ld.dat",(long)this);
                  ofp = fopen (temporaryRawFileName, "wb");
                  if (ofp) {
                     sizeWritten = fwrite ( pixels, pixelSize, 1, ofp);
                     fclose(ofp);
                  }
               }
               // j'ajoute les mots cles specifique a une image RAW
               sprintf(filter, "%u",dataInfo.filters);
               keywords->Add("RAWFILTE", &filter,          TSTRING,  "Raw bayer matrix keys", "" );
               keywords->Add("RAWCOLOR", &dataInfo.colors,  TINT,    "Raw color plane number", "" );
               keywords->Add("RAWBLACK", &dataInfo.black,   TINT,    "Raw low cut", "" );
               keywords->Add("RAWMAXI", &dataInfo.maximum, TINT,     "Raw hight cut", "" );

            } else {
               throw CError("libdcraw_decodeBuffer error=%d", result);
            }
            break;
         }
      default:
         {
            throw CError("SetPixels compression=%d not implemented", compression);
            break;
         }
      }
   } else {
      // cas d'un image couleur
      switch (compression) {
      case COMPRESS_NONE:
         {
            pixTemp = new CPixelsRgb(width, height, pixelFormat, pixels, reverseX, reverseY);
            break;
         }
      case COMPRESS_JPEG :
         {
            unsigned char * decodedData;
            long   decodedSize;
            int   width, height;
            int result = -1;


            result  = libdcjpeg_decodeBuffer((unsigned char*) pixels, pixelSize, &decodedData, &decodedSize, &width, &height);
            if (result == 0 )  {
               pixTemp = new CPixelsRgb(width, height, FORMAT_BYTE, decodedData, reverseX, reverseY);
               // l'espace memoire decodedData cree par la librairie libdcjpeg doit etre desalloue par cette meme librairie.
               libdcjpeg_freeBuffer(decodedData);
            } else {
               throw CError("libjpeg_decodeBuffer error=%d", result);
            }
            break;
         }
      default :
         {
            throw CError("SetPixels not implemented for compression=%s", CPixels::getPixelCompressionName(compression));
            break;
         }
      }

   }

   pthread_mutex_lock(&mutex);
   // s'il n'y a pas eu d'exception pendant la creation de pixTemp, je detruis l'ancienne image
	if (this->pix != NULL) {
	   delete this->pix ;
	   this->pix = NULL;
	}

   // j'affecte la nouvelle image
   this->pix = pixTemp;

   // j'ajoute les mots cles NAXIS
   // attention: ne pas utiliser les parametres width et height pass�s en entr�s de la fonctions
   // car leur valeur d�pend de la compression.
   if ( pix->getPixelClass() == CLASS_GRAY ) {
      int naxis = 2; 
      int naxis1 = pix->GetWidth();
      int naxis2 = pix->GetHeight();
      keywords->Add("NAXIS",  &naxis, TINT, "", "");
      keywords->Add("NAXIS1", &naxis1, TINT, "", "");
      keywords->Add("NAXIS2", &naxis2, TINT, "", "");
   } else {
      int naxis = 3; 
      int naxis1 = pix->GetWidth();
      int naxis2 = pix->GetHeight();
      int naxis3 = 3;
      keywords->Add("NAXIS",  &naxis, TINT, "", "");
      keywords->Add("NAXIS1", &naxis1, TINT, "", "");
      keywords->Add("NAXIS2", &naxis2, TINT, "", "");
      keywords->Add("NAXIS3", &naxis3, TINT, "", "");
   }

   pthread_mutex_unlock(&mutex);
}


/*
 * SetPixels
 *  Affecte le tableau de pixels dans buffer->pix
 *
 *  Si une erreur survient pendant le traitement, buffer->pix conserve l'ancien tableau de pixels
 *
 *  Return : 0
 *
 */
void CBuffer::SetPixels(int width, int height, int pixelSize, int offset[4], int pitch, unsigned char * pixels) {
   CPixels  * pixTemp;

   // On fait de la place avant de recreer le contenu du buffer
   //FreeBuffer(keep_keywords);

   pixTemp = new CPixelsRgb(width, height, pixelSize, offset, pitch, pixels);

   pthread_mutex_lock(&mutex);
   // s'il n'y a pas eu d'exception pendant la creation de pixTemp, je detruis l'ancienne image
	if (pix != NULL) {
	   delete pix ;
	   pix = NULL;
	}

   pix = pixTemp;
   pthread_mutex_unlock(&mutex);
}


void CBuffer::SetKeyword(char *nom, char *data, char *datatype, char *comment, char *unit)
{
   int iDatatype;
   int iInt;
   double dDouble;
   float fFloat;
   void *pvData;

   if(keywords==NULL) {
      throw CError(ELIBSTD_NO_KWDS); ;
   } else {
      if(strcmp(datatype,"float")==0) {
         iDatatype = TFLOAT;
         sscanf(data,"%f",&fFloat);
         pvData = (void*)&fFloat;
      } else if(strcmp(datatype,"double")==0) {
         iDatatype = TDOUBLE;
         sscanf(data,"%lf",&dDouble);
         pvData = (void*)&dDouble;
      } else if(strcmp(datatype,"string")==0) {
         iDatatype = TSTRING;
         pvData = (void*)data;
      } else {
         iDatatype = TINT;
         sscanf(data,"%d",&iInt);
         pvData = (void*)&iInt;
      }
      keywords->Add(nom,pvData,iDatatype,comment,unit);
   }
}

void CBuffer::CopyTo(CBuffer*dest)
{
   int naxis1, naxis2, plane;
   TYPE_PIXELS *ppix = NULL;


   if(dest==NULL) throw CError(ELIBSTD_DEST_BUF_NOT_FOUND);
   naxis1 = GetWidth();
   naxis2 = GetHeight();
   plane  = pix->GetPlanes();
   ppix = (TYPE_PIXELS *) malloc(naxis1* naxis2 * plane * sizeof(float));

   if(plane == 1) {
      pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_GREY, (void*) ppix);
      dest->CopyFrom(keywords, PLANE_GREY, ppix);
   }
   else {
      pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_RGB, (void*) ppix);
      dest->CopyFrom(keywords,PLANE_RGB, ppix);
   }
   free(ppix);
}

void CBuffer::CopyFrom(CFitsKeywords*hdr, TColorPlane plane, TYPE_PIXELS*pixels)
{
   int naxis1=1, naxis2=1, naxis=2;
   int nb_keys, msg;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;

   if(hdr==NULL) throw CError(ELIBSTD_NO_KWDS);

   if (hdr->FindKeyword("NAXIS")!=NULL) {
      naxis  = hdr->FindKeyword("NAXIS")->GetIntValue();
   }
   if (hdr->FindKeyword("NAXIS1")!=NULL) {
      naxis1 = hdr->FindKeyword("NAXIS1")->GetIntValue();
   }
   if (hdr->FindKeyword("NAXIS2")!=NULL) {
      naxis2 = hdr->FindKeyword("NAXIS2")->GetIntValue();
   }

   FreeBuffer(DONT_KEEP_KEYWORDS);
   SetPixels(plane, naxis1, naxis2, FORMAT_FLOAT, COMPRESS_NONE, pixels, 0, 0, 0);

   // La recopie des mots cles s'effectue en passant par les tableaux
   // format KLOTZ (j'ai pas envie de me faire chier a la main !
   nb_keys = hdr->GetKeywordNb();

   msg = Libtt_main(TT_PTR_ALLOKEYS,6,&nb_keys,&keynames,&values,&comments,&units,&datatypes);
   if(msg) throw CErrorLibtt(msg);

   hdr->SetToArray(&keynames,&values,&comments,&units,&datatypes);
   keywords->GetFromArray(nb_keys,&keynames,&values,&comments,&units,&datatypes);

   msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
}

void CBuffer::CopyKwdFrom(CBuffer*bufOrg)
{
   int i;
   int nb_keys;
   int msg;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;
   CFitsKeyword *kwd;

   nb_keys = bufOrg->keywords->GetKeywordNb();

   msg = Libtt_main(TT_PTR_ALLOKEYS,6,&nb_keys,&keynames,&values,&comments,&units,&datatypes);
   if(msg) throw CErrorLibtt(msg);

   bufOrg->keywords->SetToArray(&keynames,&values,&comments,&units,&datatypes);

   for(i=0;i<nb_keys;i++) {
      if((strcmp(keynames[i],"SIMPLE")!=0)&&(strcmp(keynames[i],"BITPIX")!=0)&&
         (strcmp(keynames[i],"NAXIS")!=0)&&(strcmp(keynames[i],"NAXIS1")!=0)&&
         (strcmp(keynames[i],"NAXIS2")!=0)) {
         kwd = keywords->AddKeyword(keynames[i]);
         kwd->GetFromArray(i,&keynames,&values,&comments,&units,&datatypes);
      }
   }

   msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
   if(msg) throw CErrorLibtt(msg);
}

/**
* definiton ajoute un constante au fichier
*
*/

void CBuffer::Add(char *filename, float offset)
{
   pix->Add(filename, offset);
}

void CBuffer::AstroBaricenter(int x1, int y1, int x2, int y2, double *xc, double *yc) {
   pix->AstroBaricenter(x1, y1, x2, y2, xc, yc);
}

void CBuffer::AstroCentro(int x1, int y1, int x2, int y2, int xmax, int ymax,
                     TYPE_PIXELS seuil,float* sx, float* sy, float* r){
   pix->AstroCentro(x1, y1, x2, y2, xmax, ymax, seuil, sx, sy, r);
}

void CBuffer::AstroFlux(int x1, int y1, int x2, int y2,
                     TYPE_PIXELS* flux, TYPE_PIXELS* maxi, int *xmax, int* ymax,
                     TYPE_PIXELS *moy, TYPE_PIXELS *seuil, int * nbpix){
   pix->AstroFlux(x1, y1, x2, y2, flux, maxi, xmax, ymax, moy, seuil, nbpix);
}

void CBuffer::AstroPhoto(int x1, int y1, int x2, int y2, int xmax, int ymax,
                    TYPE_PIXELS moy, double *dFlux, int* ntot){
   pix->AstroPhoto(x1, y1, x2, y2, xmax, ymax, moy, dFlux, ntot);
}

void CBuffer::AstroPhotom(int x1, int y1, int x2, int y2, int method, double r1, double r2,double r3,
                      double *flux, double* f23, double* fmoy, double* sigma, int *n1){
   pix->AstroPhotometry(x1, y1, x2, y2, method, r1, r2, r3, flux, f23, fmoy, sigma, n1);
}

/*
 * AstroSlitCentro
 *  calcule le baricentre du signal sur une fente de spectrometre
 *
 *  @param     x1 fenetre de detection de l'etoile (coin bas gauche )
 *  @param     y1 fenetre de detection de l'etoile
 *  @param     x2 fenetre de detection de l'etoile ( coin haud droit )
 *  @param     y2 fenetre de detection de l'etoile
 *  @param     starDetectionMode 1=fit de gaussienne  2=barycentre
 *  @param     pixelMinCount nombre minimal de pixels au dessus du seuil de qualite
 *  @param     slitWidth   largeur de la fente (en pixel)
 *  @param     slitRatio ratio pour convertir le rapport de flux en nombre de pixels
 *  Parameters OUT:
 *  @param     starStatus     resultat de la recherche de l'etoile (DISABLED, DETECTED, NO_SIGNAL)
 *  @param     *xc abcisse du centre de l'etoile
 *  @param     *yc *rdonnee du centre de l'etoile
 *  @param     maxIntensity   intensite max
 *  @param     message        message d'information
 *  Return :
 *    void
 *
 */
void CBuffer::AstroSlitCentro(int x1, int y1, int x2, int y2,
                              int starDetectionMode, int pixelMinCount,
                              int slitWidth, double slitRatio,
                              char *starStatus, double *xc, double *yc,
                              TYPE_PIXELS* maxIntensity, char * message)
{
   int i, j;
   TYPE_PIXELS *imgPix=NULL, *imgOffset,  *imgPtr;
   double *colSum = NULL;
   double s1, s2, v, tableOffset, pixelOffset;
   int width, height, naxis1, naxis2, y0;
   double gauss[4], ecart;
   double qualityMin;

   naxis1 = GetWidth();
   naxis2 = GetHeight();
   if (x1<0) {x1=0;}
   if (x2<0) {x2=0;}
   if (y1<0) {y1=0;}
   if (y2<0) {y2=0;}
   if (x1>naxis1-1) {x1=naxis1-1;}
   if (x2>naxis1-1) {x2=naxis1-1;}
   if (y1>naxis2-1) {y1=naxis2-1;}
   if (y2>naxis2-1) {y2=naxis2-1;}
   width  = x2-x1+1;
   height = y2-y1+1;
   y0     = height/2;

   strcpy(starStatus,"DISABLED");
   strcpy(message,"");

   imgPix = (TYPE_PIXELS *) malloc(width * height * sizeof(float));
   pix->GetPixels(x1, y1, x2, y2, FORMAT_FLOAT, PLANE_GREY, (void*) imgPix);

   // ----------------------------------------------------
   // je calcule la qualite qualityMin ?= dbgmean + 6.0 * dbgsigma
   // ----------------------------------------------------
   int ttResult;
   int datatype = TFLOAT;
   double dlocut, dhicut, dmaxi, dmini, dmean, dsigma, dbgmean, dbgsigma, dcontrast;

   ttResult = Libtt_main(TT_PTR_STATIMA,13,imgPix,&datatype,&width,&height,
      &dlocut,&dhicut,&dmaxi,&dmini,&dmean,&dsigma,&dbgmean,&dbgsigma,&dcontrast);
   if(ttResult) {
      free(imgPix);
      throw CErrorLibtt(ttResult);
   }
   *maxIntensity = (TYPE_PIXELS ) dmaxi;
   //*background   = dbgmean;
   qualityMin = dbgmean + 6.0 * dbgsigma;

   // ----------------------------------------------------
   // j'ajuste une gaussienne pour le calcul de la FWHM,
   // et je compte les pixels qui sont au dessus du seuil de qualite
   // ----------------------------------------------------
   double px[4], py[4];
   double errx, erry;
   TYPE_PIXELS *iX=NULL, *iY=NULL;
   double *dX=NULL, *dY=NULL;

   // je cree les variables de travail pour l'ajustement de la gaussienne
   iX = (TYPE_PIXELS*)calloc(width,sizeof(TYPE_PIXELS));
   dX = (double*)calloc(width,sizeof(double));
   iY = (TYPE_PIXELS*)calloc(height,sizeof(TYPE_PIXELS));
   dY = (double*)calloc(height,sizeof(double));
   int pixelCount = 0;

   for(j=0;j<height;j++) {
      imgOffset  = imgPix  + j * width;
      for(i=0;i<width;i++) {
         imgPtr  = imgOffset+i;
         *(iX+i) += *imgPtr;
         *(iY+j) += *imgPtr;
         // j'incremente le compteur des pixels au dessus du seuil
         if ( (double)*imgPtr > qualityMin ) {
            pixelCount++;
         }
      }
   }

   for(i=0;i<width;i++)  *(dX+i) = (double)*(iX+i);
   for(i=0;i<height;i++) *(dY+i) = (double)*(iY+i);

   pix->fitgauss1d(width,dX,px,&errx);
   pix->fitgauss1d(height,dY,py,&erry);

   //*measuredFwhmX = px[2];
   //*measuredFwhmY = py[2];

   //je libere la memoire
   if (iX!=NULL)  free(iX);
   if (dX!=NULL)  free(dX);
   if (iY!=NULL)  free(iY);
   if (dY!=NULL)  free(dY);

   // ----------------------------------------------------
   // je verifie le seuil de qualite
   // ----------------------------------------------------
   if (  pixelCount < pixelMinCount ) {
      *xc = width/2  +x1;
      *yc = height/2 +y1;
      strcpy(starStatus, "NO_SIGNAL");
      if (imgPix!=NULL)  free(imgPix);
      return;
   }

   strcpy(starStatus, "DETECTED");


   // je calcule la position de l'etoile
   if ( starDetectionMode == 1 ) {
      /////////////////////////////////////////////////////////////////
      // je detecte l'etoile par ajustement de gaussienne sur les 2 axes
      /////////////////////////////////////////////////////////////////
      *xc = px[1] + x1;
      *yc = py[1] + y1;
   } else {
      /////////////////////////////////////////////////////////////////
      //je detecte l'etoile par ajustement de gaussienne sur l'axe X et calcul du centre de gravite sur l'axe Y
      /////////////////////////////////////////////////////////////////
      s1=0;
      s2=0;

      colSum=(double *) calloc(width,sizeof(double));
      for (i=0;i<width;i++) {
         v=0;
         // projection marginale suivant X (somme de la colonne)
         for (j=0;j<height;j++) {
            v+=(double)imgPix[i+j*width];
         }
         colSum[i]=v;
      }

      // offset = moyenne des 3 colonnes de chaque cotes
      tableOffset=(colSum[0]+colSum[1]+colSum[2]+colSum[width-1]+colSum[width-2]+colSum[width-3])/6.0;
      pixelOffset = tableOffset/height;

      // je soustrait l'offset
      for (i=0;i<width;i++) {
         colSum[i] -= tableOffset;
      }

      // j'ajuste une gaussienne l'axe X
      //     gauss[0]=intensite maximum de la gaussienne
      //     gauss[1]=indice du maximum de la gaussienne
      //     gauss[2]=fwhm
      //     gauss[3]=fond
      pix->fitgauss1d(width,colSum,gauss, &ecart);

      //printf("offset=%.2f max=%.0f x=%.2f fwhm=%.2f fond=%.2f ecart=%.2f\n", pixelOffset, gauss[0], gauss[1], gauss[2] , gauss[3], ecart);
      if (gauss[1]>0  && gauss[1] < width ) {
         *xc = gauss[1];
      } else {
         // je prend le centre de l'image (c'est a dire la position precedente)
         // si l'ajustement de la gaussienne donne un resultat hors de l'image
         *xc = width/2 ;
      }

      // je change de repere de coordonnees
      *xc += x1;


      /////////////////////////////////////////////
      //je calcule le centre de gravite sur l'axe Y
      /////////////////////////////////////////////
      s1=0;
      s2=0;
      // je calcule le signal sur la levre haute
      for (i=0;i<width;i++) {
         for (j=y0+slitWidth/2;j<height;j++) {
            v = imgPix[i+j*width] -pixelOffset;
            if ( v <0 ) v =0;
            s1+=(double) v;
         }
      }


      // je calcule le signal sur la levre basse
      for (i=0;i<width;i++) {
         for (j=0;j<y0-slitWidth/2;j++) {
            v = imgPix[i+j*width] -pixelOffset;
            if ( v <0 ) v =0;
            s2+=(double)v;
         }
      }

      *yc = (double) y0;

      if ( s1 > s2 ) {
         v = s1;
      } else {
         v = s2;
      }
      if ( v != 0.0 ) {
         *yc += slitRatio * (s1-s2)/v;
      }

      // ecretage des valeurs
      //   if ( *yc < 0.0 )      *yc = 0.0;
      //   if ( *yc > (double)height ) *yc = (double)height;

      // je change de repere de coordonnees
      *yc += y1;
   }
   // je libere la imgPix
   if (imgPix!=NULL) free(imgPix);
   if (colSum!=NULL) free(colSum);

}


/*
 * AstroFiberCentro
 *  detecte le centre de l'etoile et le centre de l'entree de fibre du spectrometre
 *
 *  Parameters IN:
 *  @param     x1 fenetre de detection de l'etoile (coin bas gauche )
 *  @param     y1 fenetre de detection de l'etoile
 *  @param     x2 fenetre de detection de l'etoile ( coin haud droit )
 *  @param     y2 fenetre de detection de l'etoile
 *  @param     starDetectionMode 1=fit de gaussienne  2=barycentre
 *  @param     fiberDetectionMode 1=Gaussienne 2=barycentre
 *  @param     integratedImage   0=pas d'image integree, 1=image integree centree la fenetre, 2=image integree centree sur la consigne
 *  @param     findFiber      indicateur de detection de fibre (0=pas de detection  1=detection activee)
 *  @param     maskBufNo      numero du buffer du masque
 *  @param     sumBufNo       numero du buffer de l'image integree
 *  @param     fiberBufNo     numero du buffer de l'image resultat
 *  @param     maskRadius     rayon du masque
 *  @param     maskFwhmX      largeur a mi hauteur de la gaussienne du masque
 *  @param     maskPercent    pourcentage du niveau du masque
 *  @param     originSumMinCounter   nombre d'images minimum a integrer pour mettre a jour la consigne
 *  @param     originSumCounter      numero de l'acquisition courante
 *  @param     previousFiberX abcisse du centre de la fibre
 *  @param     previousFiberY ordonnee du centre de la fibre
 *  @param     pixelMinCount  nombre minimal de pixels (facteur de qualite)
 *  @param     biasValue      valeur du bias
 *  Parameters OUT:
 *  @param     starStatus     resultat de la recherche de l'etoile
 *  @param     starX          abcisse du centre de l'etoile
 *  @param     starY          ordonnee du centre de l'etoile
 *  @param     fiberStatus    resultat de la recherche de la fibre
 *  @param     fiberX         abcisse du centre de la fibre
 *  @param     fiberY         ordonnee du centre de la fibre
 *  @param     measuredFwhmX  gaussienne mesuree
 *  @param     measuredFwhmY  gaussienne mesuree
 *  @param     background     fond du ciel
 *  @param     maxIntensity   intensite max
 *  @param     starFlux       flux de l'etoile
 *  @param     message        message d'information
 *
 *  Return :   void . Exception en cas d'erreur imprevue
 *
 */

void CBuffer::AstroFiberCentro(int x1, int y1, int x2, int y2,
                                int starDetectionMode, int fiberDetectionMode,
                                int integratedImage, int findFiber,
                                int maskBufNo, int sumBufNo, int fiberBufNo,
                                int maskRadius, double maskFwhm, double maskPercent,
                                int originSumMinCounter, int originSumCounter,
                                double previousFiberX, double previousFiberY,
                                int pixelMinCount, double biasValue,
                                char *starStatus,  double *starX,  double *starY,
                                char *fiberStatus, double *fiberX, double *fiberY,
                                double *measuredFwhmX, double *measuredFwhmY,
                                double *background, double *maxIntensity, 
                                double *starFlux,char *message)
{
   int i, j;
   int naxis1, naxis2;  // taille de l'image
   int width, height;   // taille de la zone a analyser
   TYPE_PIXELS flux,sx,sy;
   TYPE_PIXELS *imgPix  =NULL, *imgOffset,  *imgPtr;
   TYPE_PIXELS *maskPix =NULL, *maskOffset, *maskPtr;
   TYPE_PIXELS *sumPix  =NULL, *sumOffset,  *sumPtr;
   TYPE_PIXELS *fiberPix=NULL, *fiberOffset,  *fiberPtr;
   TYPE_PIXELS *iX=NULL, *iY=NULL;
   CBuffer * sumBuf  = NULL;

   double *dX=NULL, *dY=NULL;
   char tempMessage[1024];
   double qualityMin;
   CBuffer * biasBuf=NULL;

   try {
      strcpy(message,"");
      strcpy(starStatus,  "DISABLED");

      strcpy(fiberStatus, "DISABLED");
      *fiberX = previousFiberX;
      *fiberY = previousFiberY;

      // debut de protection acces concurrent au buffer
      pthread_mutex_lock(&mutex);
      naxis1 = this->GetWidth();
      naxis2 = this->GetHeight();
      if (x1<0) {x1=0;}
      if (x2<0) {x2=0;}
      if (y1<0) {y1=0;}
      if (y2<0) {y2=0;}
      if (x1>naxis1-1) {x1=naxis1-1;}
      if (x2>naxis1-1) {x2=naxis1-1;}
      if (y1>naxis2-1) {y1=naxis2-1;}
      if (y2>naxis2-1) {y2=naxis2-1;}

      width =  x2-x1+1;
      height = y2-y1+1;

      imgPix = (TYPE_PIXELS *) malloc(width * height * sizeof(TYPE_PIXELS));
      if ( imgPix ==NULL ) {
         throw CError( "CBuffer::AstroFiberCentro not enough memory for imgPix");
      }
      pix->GetPixels(x1, y1, x2, y2, FORMAT_FLOAT, PLANE_GREY, (void*) imgPix);

      // fin de protection acces concurrent au buffer
      pthread_mutex_unlock(&mutex);

      // ----------------------------------------------------
      // je soustrais la valeur arbitraire du bias
      // ----------------------------------------------------
      if ( biasBuf == NULL && biasValue != 0 ) {
         for(j=0;j<height;j++) {
            imgOffset  = imgPix  + j * width;
            for(i=0;i<width;i++) {
               imgPtr  = imgOffset  +i;
               // je soutrais le bias
               *imgPtr -= (float) biasValue;
            }
         }
      }

      // ----------------------------------------------------
      // je calcule la qualite qualityMin ? partir de mean, dsigma
      //  et je recupere dbgmean pour le soustraire de l'image si le bias n'est pas fourni
      // ----------------------------------------------------
      int ttResult;
      int datatype = TFLOAT;
      double dlocut, dhicut, dmaxi, dmini, dmean, dsigma, dbgmean,dbgsigma,dcontrast;

      ttResult = Libtt_main(TT_PTR_STATIMA,13,imgPix,&datatype,&width,&height,
         &dlocut,&dhicut,&dmaxi,&dmini,&dmean,&dsigma,&dbgmean,&dbgsigma,&dcontrast);
      if(ttResult) {
         throw CError("AstroFiberCentro: TT_PTR_STATIMA width=%d height=%d");
      }
      *maxIntensity = dmaxi;
      *background   = dbgmean;

      if ( biasBuf != NULL ) {
         qualityMin = dbgmean + 6.0 * dbgsigma;
      } else {
         qualityMin = 6.0 * dbgsigma;
      }

      // ----------------------------------------------------
      // je soustrais le fond de ciel estim� si le bias n'a pas �t� d�j� soustrait
      // ----------------------------------------------------
      if ( biasBuf == NULL && biasValue == 0 ) {
         for(j=0;j<height;j++) {
            imgOffset  = imgPix  + j * width;
            for(i=0;i<width;i++) {
               imgPtr  = imgOffset  +i;
               // je soutrait le fond de ciel
               *imgPtr -= (float)dbgmean;
            }
         }
      }

      // ----------------------------------------------------
      // j'ajuste une gaussienne pour le calcul de la FWHM,
      // de background et de maxIntensity
      // ----------------------------------------------------
      double px[4], py[4];
      double errx, erry;
      int    pixelCount = 0;
      flux = 0.;
      sx = 0.;
      sy = 0.;


      // je cree les variables de travail pour l'ajustement de la gaussienne
      iX = (TYPE_PIXELS*)calloc(width,sizeof(TYPE_PIXELS));
      dX = (double*)calloc(width,sizeof(double));
      iY = (TYPE_PIXELS*)calloc(height,sizeof(TYPE_PIXELS));
      dY = (double*)calloc(height,sizeof(double));

      for(j=0;j<height;j++) {
         imgOffset  = imgPix  + j * width;
         for(i=0;i<width;i++) {
            imgPtr  = imgOffset+i;
         
            // je supprime les pixels negatifs
            //if ( pixel < 0. ) {
            //  pixel = 0.;
            //}

            // je calcule les histogrammes pour la Fwhm
            *(iX+i) += *imgPtr;
            *(iY+j) += *imgPtr;

            // j'incremente le compteur des pixels au dessus du seuil
            if ( (double)*imgPtr > qualityMin ) {
               pixelCount++;
            }

            // je calcule le flux de l'�toile
            flux += *imgPtr  ;

            // je cumule les flux de l'image courante pour le calcul du barycentre
            if ( starDetectionMode == 2 ) {
               sx += (i+1) * *imgPtr ;
               sy += (j+1) * *imgPtr ;
            }

         }
      }

      for(i=0;i<width;i++)  *(dX+i) = (double)*(iX+i);
      for(i=0;i<height;i++) *(dY+i) = (double)*(iY+i);

      pix->fitgauss1d(width,dX,px,&errx);
      pix->fitgauss1d(height,dY,py,&erry);

      *measuredFwhmX = px[2];
      *measuredFwhmY = py[2];
      *starFlux = flux;

      // ----------------------------------------------------
      // j'enregistre l'image courante pour debogage
      // ----------------------------------------------------
      /*
      if ( findFiber == 0 ) {
         sumBuf = (CBuffer *)buf_pool->Chercher(sumBufNo);
         int naxis = 2;
         sumBuf->initialMipsLo = (float)dlocut;
         sumBuf->initialMipsHi = (float)dhicut;
         sumBuf->GetKeywords()->Add("NAXIS",  &naxis, TINT, "","");
         sumBuf->GetKeywords()->Add("NAXIS1", &width, TINT, "","");
         sumBuf->GetKeywords()->Add("NAXIS2", &height,TINT, "","");
         // je copie la nouvelle image integree et les mots cles dans le buffer sumBuf
         sumBuf->SetPixels(PLANE_GREY, width, height, FORMAT_FLOAT, COMPRESS_NONE, imgPix, 0,0,0) ;
         sumBuf->GetKeywords()->Add("MIPS-LO", &sumBuf->initialMipsLo, TFLOAT, "","");
         sumBuf->GetKeywords()->Add("MIPS-HI", &sumBuf->initialMipsHi, TFLOAT, "","");
      }
      */


      // ----------------------------------------------------
      // je compte les pixels  au dessus du seuil de qualite (pixelCount)
      // je calcule le barycentre (flux, sx, sy)
      // je calcule l'image integree
      // ----------------------------------------------------

      *starX=0.;
      *starY=0.;

      // je calcule la position de l'etoile
      if ( starDetectionMode == 1 ) {
         // centre de la gaussienne
         *starX = px[1];
         *starY = py[1];
      } else {
         // barycentre
         if ( flux != 0 ) {
            *starX = (sx / flux) -1 ;
            *starY = (sy / flux) -1;
            if ( *starX <0 || *starX > width ) {
               pixelCount = 0;
               sprintf(message, "starX= %f", *starX);
            }
            if ( *starY <0 || *starY > height ) {
               pixelCount = 0;
               sprintf(message, "starY= %f", *starY);
            }
         } else {
            *starX = px[1];
            *starY = py[1];
            // je force le nombre de pixel a zero pour retouner une erreur "NO SIGNAL"
            pixelCount = 0;
         }
      }

      // je change de repere de coordonnees
      *starX += x1;
      *starY += y1;



      // ----------------------------------------------------
      // je verifie le seuil de qualite
      // ----------------------------------------------------
      if (  pixelCount < pixelMinCount ) {
         strcpy(starStatus, "NO SIGNAL");
         if (imgPix!=NULL)    free(imgPix);
         if (maskPix!=NULL)   free(maskPix);
         if (sumPix!=NULL)    free(sumPix);
         if (fiberPix!=NULL)  free(fiberPix);

         if (iX!=NULL)  free(iX);
         if (dX!=NULL)  free(dX);
         if (iY!=NULL)  free(iY);
         if (dY!=NULL)  free(dY);
         return;
      }

      strcpy(starStatus, "DETECTED");

      // ----------------------------------------------------
      // calcul de  l'image integree
      // ----------------------------------------------------
      if ( integratedImage != 0) {
         TYPE_PIXELS sumMaxIntensity = 0;
         TYPE_PIXELS sumMinIntensity = 0;
         float floatCounter = (float) originSumCounter - 1 ;

         // je reserve l'espace memoire de l'image integree
         sumPix = (TYPE_PIXELS *) malloc(width * height * sizeof(TYPE_PIXELS));
         if ( sumPix ==NULL ) {
            throw CError( "CBuffer::AstroFiberCentro not enough memory for sumPix");
         }

         sumBuf = (CBuffer *)buf_pool->Chercher(sumBufNo);

         if ( originSumCounter == 1 ) {
            // j'initialise l'image integree
            int naxis = 2;
            sumBuf->GetKeywords()->Add("NAXIS",  &naxis, TINT, "","");
            sumBuf->GetKeywords()->Add("NAXIS1", &width, TINT, "","");
            sumBuf->GetKeywords()->Add("NAXIS2", &height,TINT, "","");
         } else {
            // je recupere l'image integree
            sumBuf->GetPixels(0, 0, width -1, height -1, FORMAT_FLOAT, PLANE_GREY, (long) sumPix);
         }

         if ( integratedImage == 2 ) {
            //// je recupere l'image courante centree sur la fibre
            //int xmin = (int) (previousFiberX - (width / 2));
            //int xmax = xmin + width - 1;
            //int ymin = (int) (previousFiberY - (height / 2));
            //int ymax = ymin + height - 1;

            //// je verifie qu'on ne sort pas de l'image courante
            //if (xmin<0) {xmin=0;}
            //if (xmin>naxis1-1) {xmin=naxis1-1;}
            //if (xmax>naxis1-1) {xmax=naxis1-1;}
            //if (ymin<0) {ymin=0;}
            //if (ymin>naxis2-1) {ymin=naxis2-1;}
            //if (ymax>naxis2-1) {ymax=naxis2-1;}

            //// je recupere les pixels de la fenetre autour de la fibre
            //pix->GetPixels(xmin, ymin, xmax, ymax, FORMAT_FLOAT, PLANE_GREY, (void*) imgPix);

            //--- j'ajoute l'image courante dans l'image integree
            for(j=0;j<height;j++) {
               imgOffset  = imgPix  + j * width;
               sumOffset  = sumPix +  j * width;
               for(i=0;i<width;i++) {
                  imgPtr  = imgOffset+i;
                  sumPtr  = sumOffset+i;

                  // je soustrais le bias et le fond du ciel
                  //TYPE_PIXELS pixelValue = *imgPtr - (TYPE_PIXELS) biasValue - (TYPE_PIXELS) dbgmean;
                  // TYPE_PIXELS pixelValue = *imgPtr - (TYPE_PIXELS) biasValue;
                  TYPE_PIXELS pixelValue = *imgPtr;

                  if ( originSumCounter == 1 ) {
                     // je copie la premiere image dans l'image integree
                     *sumPtr = pixelValue;
                  } else {
                     // j'ajoute une image dans l'image integree
                     *sumPtr = (*sumPtr + pixelValue / floatCounter ) / ( (TYPE_PIXELS) 1.0 + (TYPE_PIXELS) 1.0 / floatCounter );
                  }
                  // je calcule l'intensite max de l'image integree
                  if ( *sumPtr > sumMaxIntensity ) {
                     sumMaxIntensity = *sumPtr;
                  }
                  // je calcule l'intensite min de l'image integree
                  if ( *sumPtr < sumMinIntensity ) {
                     sumMinIntensity = *sumPtr;
                  }
               }
            }

         } else {
            // je copie l'image centree
            for(j=0;j<height;j++) {
               imgOffset  = imgPix  + j * width;
               sumOffset  = sumPix +  j * width;
               for(i=0;i<width;i++) {
                  imgPtr  = imgOffset+i;
                  sumPtr  = sumOffset+i;

                  if ( originSumCounter == 1 ) {
                     // je copie la premiere image dans l'image integree
                     *sumPtr = *imgPtr;
                  } else {
                     // j'ajoute une image dans l'image integree
                     *sumPtr = (*sumPtr + *imgPtr / floatCounter ) / ( (TYPE_PIXELS) 1.0 + (TYPE_PIXELS) 1.0 / floatCounter );
                  }
                  // je calcule l'intensite max de l'image integree
                  if ( *sumPtr > sumMaxIntensity ) {
                     sumMaxIntensity = *sumPtr;
                  }
                  // je calcule l'intensite min de l'image integree
                  if ( *sumPtr < sumMinIntensity ) {
                     sumMinIntensity = *sumPtr;
                  }
               }
            }
         }

         // ----------------------------------------------------
         // je calcule le fond du ciel  sur l'image integree
         // ----------------------------------------------------
         int ttResult;
         int datatype = TFLOAT;
         double dlocut, dhicut, dmaxi, dmini, dmean, dsigma, dbgmean,dbgsigma,dcontrast;

         ttResult = Libtt_main(TT_PTR_STATIMA,13,sumPix,&datatype,&width,&height,
            &dlocut,&dhicut,&dmaxi,&dmini,&dmean,&dsigma,&dbgmean,&dbgsigma,&dcontrast);
         if(ttResult) {
            throw CError("AstroFiberCentro skylevel: TT_PTR_STATIMA width=%d height=%d");
         }
         *background   = dbgmean;

         // je copie la nouvelle image integree et les mots cles dans le buffer sumBuf
         sumBuf->SetPixels(PLANE_GREY, width, height, FORMAT_FLOAT, COMPRESS_NONE, sumPix, 0,0,0) ;
         sumBuf->GetKeywords()->Add("MIPS-LO", &sumMinIntensity, TFLOAT, "","");
         sumBuf->GetKeywords()->Add("MIPS-HI", &sumMaxIntensity, TFLOAT, "","");
         sumBuf->GetKeywords()->Add("SUM_COUNT", &originSumCounter, TINT, "integrated image counter","");
         sumBuf->initialMipsLo = sumMinIntensity;
         sumBuf->initialMipsHi = sumMaxIntensity;


         // ----------------------------------------------------
         // je detecte l'entree de la fibre
         // ----------------------------------------------------
         if ( findFiber == 1 )  {
            double previousFiberCgX , previousFiberCgY;
            int fiberInside = 1;

            // je verifie si la fibre est dans l'image
            if (previousFiberX <= x1) {fiberInside = 0;}
            if (previousFiberX >= x2) {fiberInside = 0; }
            if (previousFiberY <= y1) {fiberInside = 0; }
            if (previousFiberY >= y2) {fiberInside = 0; }

            if (fiberInside == 0 ) {
               *fiberX = previousFiberX;
               *fiberY = previousFiberY;
            }

            previousFiberCgX = previousFiberX - x1;
            previousFiberCgY = previousFiberY - y1;

            // je verifie que la consigne est dans l'image
            if ( fiberInside==1 ) {
               CBuffer * maskBuf = NULL;
               maskBuf = (CBuffer *)buf_pool->Chercher(maskBufNo);
               // si c'est la premiere image integree, je prepare le masque
               maskPix = (TYPE_PIXELS *) malloc(width * height * sizeof(TYPE_PIXELS));
               if ( maskPix ==NULL ) {
                  throw CError( "CBuffer::AstroFiberCentro not enough memory for maskPix");
               }
               if ( originSumCounter == 1 ) {

                  // je cree le masque
                  for(j=0;j<height;j++) {
                     maskOffset = maskPix + j * width;
                     for(i=0;i<width;i++) {
                        maskPtr = maskOffset+i;
                        // je calcule la distance entre le point la position de la fibre 
                        double dist = sqrt( ((double)i-previousFiberCgX)*((double)i-previousFiberCgX) + ((double)j-previousFiberCgY)*((double)j-previousFiberCgY));
                        if ( dist > maskRadius ) {
                           *maskPtr = 0;
                           // j'applique le masque sur l'image courante
                           // *imgPtr  = 0;
                        } else {
                           *maskPtr = 1;
                        }
                     }
                  }

                  // j'enregistre le masque dans le buffer
                  int naxis = 2;
                  TYPE_PIXELS minMaskIntensity = 0;
                  TYPE_PIXELS maxMaskIntensity = 1;
                  maskBuf->SetPixels(PLANE_GREY, width, height, FORMAT_FLOAT, COMPRESS_NONE, maskPix, 0,0,0) ;
                  maskBuf->GetKeywords()->Add("NAXIS",  &naxis, TINT, "","");
                  maskBuf->GetKeywords()->Add("NAXIS1", &width, TINT, "","");
                  maskBuf->GetKeywords()->Add("NAXIS2", &height,TINT, "","");
                  maskBuf->GetKeywords()->Add("MIPS-LO", &minMaskIntensity, TFLOAT, "","");
                  maskBuf->GetKeywords()->Add("MIPS-HI", &maxMaskIntensity, TFLOAT, "","");
                  maskBuf->GetKeywords()->Add("SUM_COUNT", &originSumCounter, TINT, "integrated image counter","");
                  maskBuf->initialMipsLo = minMaskIntensity;
                  maskBuf->initialMipsHi = maxMaskIntensity;

               } else {
                  // je recupere le masque
                  maskBuf->GetPixels(0, 0, width -1, height -1, FORMAT_FLOAT, PLANE_GREY, (long) maskPix);
               }

               // je calcule la position de la fibre si l'image integree a integree suffisamment d'images
               if ( originSumCounter >= originSumMinCounter ) {

                  // ----------------------------------------------------
                  // 1) je synthetise la gaussienne
                  //  IMA2[i,j]=(1.-offset)*exp(-((i-cgx)**2 / (0.36*fwhmx**2))-((j-cgy)**2 /(0.36*fwhmy**2)))+offset
                  //  avec fwhmx = fwhmy = le seeing moyen en pixel,
                  //  et cgx, cgy le centre de gravite de l'etoile detectee,
                  //  et offset = 1. - 1./max_ima
                  // ----------------------------------------------------

                  TYPE_PIXELS offset;
                  TYPE_PIXELS gaussIntensity ;
                  TYPE_PIXELS fiberMaxIntensity = 0;          // intensite du plateau

                  if ( sumMaxIntensity > 0 ) {
                     offset = (TYPE_PIXELS) (1. - 1. / sumMaxIntensity);
                  } else {
                     offset = 0;
                  }

                  //fiberPix = (TYPE_PIXELS*) malloc(width * height * sizeof(TYPE_PIXELS));
                  fiberPix = (TYPE_PIXELS*) calloc(width * height, sizeof(TYPE_PIXELS));
                  if ( fiberPix ==NULL ) {
                     throw CError( "CBuffer::AstroFiberCentro not enough memory for fiberPix");
                  }

                  double cgx =    *starX - x1;
                  double cgy =    *starY - y1;

                  /*
                  int imin = int(previousFiberCgX) - maskRadius; 
                  int imax = int(previousFiberCgX) + maskRadius; 
                  int jmin = int(previousFiberCgY) - maskRadius; 
                  int jmax = int(previousFiberCgY) + maskRadius;
                  if ( imin <0 )       { imin = 0;}
                  if ( imax > width )  { imax = width;}
                  if ( jmin <0 )       { jmin = 0;}
                  if ( jmax > height ) { jmax = height;}
                  */

                  for(j=0;j<height;j++) {
                  //for(j=jmin;j<jmax;j++) {
                     sumOffset  = sumPix  + j * width;
                     maskOffset = maskPix + j * width;
                     fiberOffset = fiberPix + j * width;
                     for(i=0;i<width;i++) {
                     //for(i=imin;i<imax;i++) {
                        sumPtr   = sumOffset+i;
                        maskPtr  = maskOffset+i;
                        fiberPtr = fiberOffset+i;
                        
                        // je calcule le valeur theorique de la guaussienne pour ce point 
                        gaussIntensity = (TYPE_PIXELS) ( (1. - offset) * exp( - ((double)i-cgx)*((double)i-cgx)/(0.36*maskFwhm*maskFwhm)  - ((double)j-cgy)*((double)j-cgy) / (0.36*maskFwhm*maskFwhm) ) + offset);
                        if ( gaussIntensity != 0 ) {
                           *fiberPtr = *sumPtr * *maskPtr / gaussIntensity;
                        } else {
                           *fiberPtr = *sumPtr * *maskPtr;
                        }

                        if ( isnan(*fiberPtr) == 1 ) {
                           *fiberPtr = 0;
                        }

                        if ( *fiberPtr > fiberMaxIntensity ) {
                           // je mets � jour la nouvelle valeur max
                           fiberMaxIntensity = *fiberPtr;
                        }
                     }
                  }

                  // ----------------------------------------------------
                  // 2) je cree le plateau
                  //  IMA3=IMA x MASKT / IMA2
                  //  PLATEAU = MAX(IMA3)
                  //  j'impose cette valeur a tous les pixels de valeurs superieurs a maskPercent=15% de cette valeur
                  //  IMA4 = WHERE ( IMA3 > 0.15 x PLATEAU, PLATEAU, IMA3 )
                  //
                  // 3) j'inverse les flux
                  //  IMA5 = (PLATEAU - IMA4 ) x MASKT
                  //  j'impose cette valeur a tous les pixels de valeurs superieurs a 15% de cette valeur
                  //  j'inverse les flux
                  //  je cumule les flux pour le calcul du centre de gravite
                  // ----------------------------------------------------
                  flux = 0.;
                  sx = 0.;
                  sy = 0.;

                  for(j=0;j<height;j++) {
                  //for(j=jmin;j< jmax ;j++) {
                     fiberOffset = fiberPix + j * width;
                     maskOffset =  maskPix  + j * width;
                     for(i=0;i<width;i++) {
                     //for(i=imin;i< imax ;i++) {
                        fiberPtr = fiberOffset+i;
                        maskPtr = maskOffset+i;

                        // j'ecrete a 15% du plateau
                        if ( *fiberPtr > fiberMaxIntensity * maskPercent ) {
                           *fiberPtr = fiberMaxIntensity;
                        }
                        // j'inverse le flux
                        *fiberPtr = (fiberMaxIntensity - *fiberPtr) * *maskPtr ;

                        if ( isnan(*fiberPtr) == 1 ) {
                           *fiberPtr = 0;
                        }

                        // je cumule les flux
                        flux += *fiberPtr;
                        sx += *fiberPtr * i;
                        sy += *fiberPtr * j ;
                     }
                  }

                  if ( fiberDetectionMode == 1 ) {
                     // je prom�ne une gaussienne 
                     TYPE_PIXELS fwhm = (TYPE_PIXELS) maskRadius;
                     int radius =  8; 
                     TYPE_PIXELS threshin = 10;
                     TYPE_PIXELS threshold = 40; 
                     int border = 10; 
                    
                     int nbPoint = A_filtrGauss2 ( fwhm,  radius,  threshin, threshold, 
						      fiberPix, width, height,  border, fiberX, fiberY); 
                     if ( nbPoint == 0 ) {
                        // si pas d'ajustement de gaussienne, je retourne la position precedente de la fibre
                        *fiberX = previousFiberX;
                        *fiberY = previousFiberY;
                     } else {
                        *fiberX += x1 ;
                        *fiberY += y1 ;
                     }
                  } else {
                     // je calcule le barycentre
                     if ( flux == 0 ) {
                        // si le flux est null, je retourne la position precedente de la fibre
                        *fiberX = previousFiberX;
                        *fiberY = previousFiberY;
                     } else {
                        *fiberX = sx / flux  ;
                        *fiberY = sy / flux  ;
                        // je change de repere de coordonnees fenetre => coordonnes image
                        *fiberX += x1;
                        *fiberY += y1;
                     }
                  }


                  // je copie l'image inversee de la fibre dans le buffer fiberBuf
                  if ( fiberBufNo !=0 ) {
                     CBuffer * fiberBuf = (CBuffer *)buf_pool->Chercher(fiberBufNo);
                     fiberBuf->SetPixels(PLANE_GREY, width, height, FORMAT_FLOAT, COMPRESS_NONE, fiberPix, 0,0,0) ;
                     int naxis = 2;
                     TYPE_PIXELS fiberMinIntensity = 1.0;
                     fiberBuf->GetKeywords()->Add("NAXIS",  &naxis, TINT, "","");
                     fiberBuf->GetKeywords()->Add("NAXIS1", &width, TINT, "","");
                     fiberBuf->GetKeywords()->Add("NAXIS2", &height,TINT, "","");
                     fiberBuf->GetKeywords()->Add("MIPS-LO", &fiberMinIntensity, TFLOAT, "","");
                     fiberBuf->GetKeywords()->Add("MIPS-HI", &fiberMaxIntensity, TFLOAT, "","");
                     fiberBuf->GetKeywords()->Add("SUM_COUNT", &originSumCounter, TINT, "integrated image counter","");
                     fiberBuf->initialMipsLo = fiberMinIntensity;
                     fiberBuf->initialMipsHi = fiberMaxIntensity;
                  }

                  // ----------------------------------------------------
                  // je verifie la qualite de l'image inversee � partir de mean, dsigma
                  // je calcule dsigma, dmaxi, dmini de la fenetre (x1,y1,x2,y2)
                  //   si dsigma < 10 , je retourne l'erreur LOW_SIGNAL
                  //   si (dmaxi-dmini) < 3*sqrt(dsigma) je retoune l'erreur NO_SIGNAL
                  //   sinon je retourne DETECTED
                  // ----------------------------------------------------
                  int ttResult;
                  int datatype = TFLOAT;
                  double dlocut, dhicut, dmaxi, dmini, dmean, dsigma, dbgmean,dbgsigma,dcontrast;

                  ttResult = Libtt_main(TT_PTR_STATIMA,13,fiberPix,&datatype,&width,&height,
                     &dlocut,&dhicut,&dmaxi,&dmini,&dmean,&dsigma,&dbgmean,&dbgsigma,&dcontrast);
                  if(ttResult) {
                     throw CErrorLibtt(ttResult);
                  }

                  if ( dmaxi < 10 ) {
                     // je retourne "LOW_SIGNAL" si le niveau maxi de l'image est < 10
                     strcpy(fiberStatus, "LOW_SIGNAL");
                     *fiberX=previousFiberX;
                     *fiberY=previousFiberY;
                     sprintf(tempMessage, "Low fiber signal: %6.2f < 10", dmaxi);
                     strcat(message,tempMessage);
                  } else if ( (dmaxi-dmini) < 3*sqrt(dsigma) ) {
                     // je retourne "NO_SIGNAL" si le niveau moyen de l'image est trop proche du foncud ciel
                     strcpy(fiberStatus, "NO_SIGNAL");
                     sprintf(tempMessage, "No fiber signal:(%6.2f -%6.2f) < 3*sqrt(%6.2f) ", dmaxi, dmini,dsigma);
                     strcat(message,tempMessage);
                     *fiberX=previousFiberX;
                     *fiberY=previousFiberY;
                  } else {
                     if ( ! isnan(*fiberX) && ! isnan(*fiberY) ) {
                        // je calcule la distance entre la nouvelle position et la precedente position de la fibre
                        double fiberDist = sqrt((*fiberX-previousFiberX)*(*fiberX-previousFiberX)+(*fiberY-previousFiberY)*(*fiberY-previousFiberY));
                        if ( fiberDist > 20 ) {
                           // je retourne "TOO_FAR" si la nouvelle position est a plus de 20 pixels de la position precendente
                           strcpy(fiberStatus, "TOO_FAR");
                           sprintf(tempMessage, "Fiber too far: %6.2f > 5", fiberDist);
                           strcat(message,tempMessage);
                        } else {
                           // je retourne "DETECTED" si tout les controles sont OK
                           strcpy(fiberStatus, "DETECTED");
                        }
                     } else {
                        // je retourne "INTEGRATING" si une deivision par zero est apparue dans le cacule de la position de le fibre
                        strcpy(fiberStatus, "IS NAN");
                     }
                  }
               } else {
                  // je retourne "INTEGRATING" si l'image integree n'a pas integre un nombre suffisant d'images
                  strcpy(fiberStatus, "INTEGRATING");
               }
            } else {
               // je retourne "OUTSIDE" si la fibre n'est pas dans la fenetre
               strcpy(fiberStatus, "OUTSIDE");
            }
         } else {
            // je retourne "DISABLED" si la detection de la position de la fibre est desactivee
            strcpy(fiberStatus, "DISABLED");
         }
      }
      sprintf(message, "%5.2f %5.2f", *fiberX, *fiberY);

      if (imgPix!=NULL)    free(imgPix);
      if (maskPix!=NULL)   free(maskPix);
      if (sumPix!=NULL)    free(sumPix);
      if (fiberPix!=NULL)  free(fiberPix);

      if (iX!=NULL)  free(iX);
      if (dX!=NULL)  free(dX);
      if (iY!=NULL)  free(iY);
      if (dY!=NULL)  free(dY);

   } catch(const CError& e) {
      if (imgPix!=NULL)    free(imgPix);
      if (maskPix!=NULL)   free(maskPix);
      if (sumPix!=NULL)    free(sumPix);
      if (fiberPix!=NULL)  free(fiberPix);

      if (iX!=NULL)  free(iX);
      if (dX!=NULL)  free(dX);
      if (iY!=NULL)  free(iY);
      if (dY!=NULL)  free(dY);

      throw e;
   }


}



void CBuffer::Autocut(double *phicut,double *plocut,double *pmode)
{
   pix->Autocut( phicut, plocut, pmode);
}

void CBuffer::BinX(int x1, int x2, int width)
{
   pix->BinX(x1, x2, width);
}

void CBuffer::BinY(int y1, int y2, int height)
{
   pix->BinY(y1,y2, height);
}

void CBuffer::Clipmin(double value)
{
   pix->Clipmin(value);
}

void CBuffer::Clipmax(double value)
{
   pix->Clipmax(value);
}

void CBuffer::Div(char *filename, float constante)
{
   pix->Div(filename, constante);
}



/*
 * int CBuffer::GetPix(TYPE_PIXELS *val, int x, int y) --
 *  Renvoie la valeur du pixel aux coordonnees passes en parametres,
 *  dans le pointeur val. Les valeurs des coordonnees vont de (0,0) a
 * (NAXIS1-1,NAXIS2-1).
 *  La valeur de retour est :
 *    - '1'  si pas de pixels (pix=NULL)
 *    - '-1' si pas de mots-cles
 *    - '-2' si pas de mot-cle 'NAXIS1'
 *
 */
void CBuffer::GetPix(int *plane, TYPE_PIXELS *val1,TYPE_PIXELS *val2,TYPE_PIXELS *val3,int x, int y)
{
   pthread_mutex_lock(&mutex);
   try {
      pix->GetPix(plane , val1, val2, val3, x, y);
   } catch(const CError& e) {
      pthread_mutex_unlock(&mutex);
      throw e;
   }
   pthread_mutex_unlock(&mutex);
}


/**
 *----------------------------------------------------------------------
 * GetPixels(TYPE_PIXELS *pixels)
 *    retourne les pixels au format float, non compresse
 *    (retourne des niveaux de gris si l'image est en couleur)
 *
 * Parameters:
 *    pixels :  pointeur de l'espace memoire prealablement alloue par l'appelant
 * Results:
 *    exception CError
 * Side effects:
 *    appelle pix->GetPixels(0, 0, width -1, height -1, FORMAT_FLOAT, PLANE_GREY, TColorPlane plane, int pixels)
 *----------------------------------------------------------------------
 */
void CBuffer::GetPixels(TYPE_PIXELS *pixels)
{
   try {
      pthread_mutex_lock(&mutex);
      int width = GetWidth();
      int height = GetHeight();
      pix->GetPixels(0, 0, width -1, height -1, FORMAT_FLOAT, PLANE_GREY, (void*) pixels);
      pthread_mutex_unlock(&mutex);
   } catch(const CError& e) {
      pthread_mutex_unlock(&mutex);
      throw e;
   }
}

void CBuffer::GetPixels(TYPE_PIXELS *pixels, TColorPlane colorPlane)
{
   try {
      pthread_mutex_lock(&mutex);
      int width = GetWidth();
      int height = GetHeight();
      pix->GetPixels(0, 0, width -1, height -1, FORMAT_FLOAT, colorPlane, (void*) pixels);
      pthread_mutex_unlock(&mutex);
   } catch(const CError& e) {
      pthread_mutex_unlock(&mutex);
      throw e;
   }

}

void CBuffer::GetPixels(int x1, int y1, int x2, int y2, TPixelFormat pixelFormat, TColorPlane colorPlane, long pixelsPtr)
{
   try {
      pthread_mutex_lock(&mutex);
      pix->GetPixels(x1, y1, x2, y2, pixelFormat, colorPlane, (void*)pixelsPtr);
      pthread_mutex_unlock(&mutex);
   } catch(const CError& e) {
      pthread_mutex_unlock(&mutex);
      throw e;
   }
}


void CBuffer::GetPixelsPointer(TYPE_PIXELS **ppixels)
{
   pix->GetPixelsPointer(ppixels);
}


void CBuffer::GetPixelsRgb( int x1,int y1,int x2, int y2,
            int mirrorX, int mirrorY, float *cuts,
            unsigned char *palette[3], unsigned char *ptr)
{
   try {
      pthread_mutex_lock(&mutex);
      pix->GetPixelsRgb(x1,y1,x2, y2, mirrorX, mirrorY,
         cuts,
         palette, ptr);
      pthread_mutex_unlock(&mutex);
   } catch(const CError& e) {
      pthread_mutex_unlock(&mutex);
      throw e;
   }
}


void CBuffer::GetPixelsVisu( int x1,int y1,int x2, int y2,
            int mirrorX, int mirrorY, float *cuts,
            unsigned char *palette[3], unsigned char *ptr)
{
   try {
      pthread_mutex_lock(&mutex);
      pix->GetPixelsVisu(x1,y1,x2, y2, mirrorX, mirrorY,
         cuts,
         palette, ptr);
      pthread_mutex_unlock(&mutex);
   } catch(const CError& e) {
      pthread_mutex_unlock(&mutex);
      throw e;
   }
}

void CBuffer::Log(float coef, float offset)
{
   pix->Log(coef, offset);
}

/*
void CBuffer::MirX()
{
   pix->MirX();
}

void CBuffer::MirY()
{
   pix->MirY();
}
*/

void CBuffer::NGain(float gain)
{
   pix->NGain(gain);
}

void CBuffer::NOffset(float offset)
{
   pix->NOffset(offset);
}

void CBuffer::Offset(float offset)
{
   pix->Offset(offset);
}

void CBuffer::Opt(char *dark, char *offset)
{
   pix->Opt(dark, offset);
}

/**
 *----------------------------------------------------------------------
 * RestoreInitialCut
 *    restaure les seuils initiaux
 *        - soit les seuils initiaux recupere
 *
 * Parameters:
 *    none
 * Results:
 *    none
 * Side effects:
 *    si les seuils initiaux n'existaien pas,
 *    ils sont calcules avec la methode Stat()
 *----------------------------------------------------------------------
 */
void CBuffer::RestoreInitialCut()
{
   float hicut;
   float locut;
   float mini, maxi, mean, sigma, bgmean, bgsigma, contrast;

   if (initialMipsLo==0 && initialMipsHi==0 ) {
      // je calcule les seuils initiaux
      Stat( -1, -1, -1, -1,
           &locut,  &hicut,   &maxi,
           &mini, &mean, &sigma,
           &bgmean, &bgsigma, &contrast);
      initialMipsHi = hicut;
      initialMipsLo = locut;
   }
   keywords->Add("MIPS-HI",&initialMipsHi,TFLOAT,"Hight cut","ADU");
   keywords->Add("MIPS-LO",&initialMipsLo,TFLOAT,"Low cut","ADU");
}

void CBuffer::Rot(float x0, float y0, float angle)
{
   pix->Rot(x0, y0, angle);
}

void CBuffer::Sub(char *filename, float offset)
{
   pix->Sub(filename, offset);
}

void CBuffer::Unsmear(float coef)
{
   pix->Unsmear(coef);
}


void CBuffer::TtImaSeries(char *s)
{

   CFitsKeyword *keyword;
   int msg;
   int nb_keys;
   char **keynames=NULL;
   char **values=NULL;
   char **comments=NULL;
   char **units=NULL;
   int *datatypes=NULL;
   int datatypeIn = 0;
   int datatypeOut = 0;
//   char valchar[80];
   TYPE_PIXELS *pixOut=NULL;
   TYPE_PIXELS *pixIn=NULL;
   TYPE_PIXELS *pixOutR = NULL;
   TYPE_PIXELS *pixOutG = NULL;
   TYPE_PIXELS *pixOutB = NULL;
   int naxis, naxis1, naxis2, naxis3;
   CPixels * newPixels = NULL;
   CFitsKeywords *newKeywords = new CFitsKeywords();

   pthread_mutex_lock(&mutex);
   try {
      naxis1 = GetWidth();
      naxis2 = GetHeight();
      nb_keys = keywords->GetKeywordNb();

      // Allocation de l'espace memoire pour les tableaux de mots-cles
      msg = Libtt_main(TT_PTR_ALLOKEYS,6,&nb_keys,&keynames,&values,&comments,&units,&datatypes);
      if(msg) throw CErrorLibtt(msg);

      // Conversion keywords vers tableaux 'Made in Klotz'
      keywords->SetToArray(&keynames,&values,&comments,&units,&datatypes);

      switch ( this->pix->getPixelClass() ) {
      case CLASS_GRAY :
         // je recupere les pixels GREY
         naxis1 = GetWidth();
         naxis2 = GetHeight();
         naxis3 = 0;
         pixIn = (TYPE_PIXELS *)malloc(naxis1*naxis2 * sizeof(TYPE_PIXELS));
         this->pix->GetPixels(0, 0, naxis1 -1, naxis2 -1, FORMAT_FLOAT, PLANE_GREY, (void*) pixIn);
         // j'applique le traitement
         datatypeIn = TFLOAT;
         datatypeOut = TFLOAT;
         msg = Libtt_main(TT_PTR_IMASERIES,13,&pixIn,&datatypeIn,&naxis1,&naxis2,&pixOut,&datatypeOut,
                           s,&nb_keys,&keynames,&values,&comments,&units,&datatypes);
         if(msg) throw CErrorLibtt(msg);

         // Je recupere les mots cles
         newKeywords->GetFromArray(nb_keys,&keynames,&values,&comments,&units,&datatypes);

         // je recherche le valeurs de NAXIS et NAXIS3
         // pour savoir si TT_PTR_IMASERIES a cree une image RGB ou GREY
         keyword = newKeywords->FindKeyword("NAXIS");
         if (keyword != NULL ) {
            naxis = keyword->GetIntValue();
         } else {
            throw CError( "CBuffer::TtImaSeries NAXIS keyword not found");
         }
         if ( naxis >= 1 ) {
            keyword = newKeywords->FindKeyword("NAXIS1");
            if (keyword != NULL ) {
               naxis1 = keyword->GetIntValue();
            } else {
               throw CError( "CBuffer::TtImaSeries NAXIS1 keyword not found");
            }
         }
         if ( naxis >= 2 ) {
            keyword = newKeywords->FindKeyword("NAXIS2");
            if (keyword != NULL ) {
               naxis2 = keyword->GetIntValue();
            } else {
               throw CError( "CBuffer::TtImaSeries NAXIS2 keyword not found");
            }
         }
         if ( naxis >= 3 ) {
            keyword = newKeywords->FindKeyword("NAXIS3");
            if (keyword != NULL ) {
               naxis3 = keyword->GetIntValue();
            } else {
               throw CError( "CBuffer::TtImaSeries NAXIS3 keyword not found");
            }
         }

         // je cree le nouvel objet contenant les pixels
         if ( naxis==3 && naxis3==3) {
            // c'est une image couleur RGB
            newPixels = new CPixelsRgb( naxis1, naxis2, FORMAT_FLOAT, pixOut, 0 , 0);
         } else {
            // c'est une image en niceau de gris
            newPixels = new CPixelsGray( naxis1, naxis2, FORMAT_FLOAT, pixOut, 0 , 0);
         }

         free(pixIn);
         Libtt_main(TT_PTR_FREEPTR,1,&pixOut);
         break;

       case CLASS_RGB :
         // je repete le traitement pour chacun des 3 plans
         naxis1 = GetWidth();
         naxis2 = GetHeight();
         pixIn = (TYPE_PIXELS *)malloc(naxis1*naxis2 * sizeof(TYPE_PIXELS));
         datatypeIn = TFLOAT;
         datatypeOut = TFLOAT;
         this->pix->GetPixels(0, 0, naxis1 -1, naxis2 -1, FORMAT_FLOAT, PLANE_R, (void*) pixIn);
         msg = Libtt_main(TT_PTR_IMASERIES,13,&pixIn,&datatypeIn,&naxis1,&naxis2,&pixOutR,&datatypeOut,s,
                  &nb_keys,&keynames,&values,&comments,&units,&datatypes);
         if(msg) throw CErrorLibtt(msg);

         naxis1 = GetWidth();  // je reinitialise naxis1, naxis2 au cas ou le traitement les aurait modifies
         naxis2 = GetHeight();
         this->pix->GetPixels(0, 0, naxis1 -1, naxis2 -1, FORMAT_FLOAT, PLANE_G, (void*) pixIn);
         msg = Libtt_main(TT_PTR_IMASERIES,7,&pixIn,&datatypeIn,&naxis1,&naxis2,&pixOutG,&datatypeOut,s);
         if(msg) throw CErrorLibtt(msg);

         naxis1 = GetWidth(); // je reinitialise naxis1, naxis2 au cas ou le traitement les aurait modifies
         naxis2 = GetHeight();
         this->pix->GetPixels(0, 0, naxis1 -1, naxis2 -1, FORMAT_FLOAT, PLANE_B, (void*) pixIn);
         msg = Libtt_main(TT_PTR_IMASERIES,7,&pixIn,&datatypeIn,&naxis1,&naxis2,&pixOutB,&datatypeOut,s);
         if(msg) throw CErrorLibtt(msg);

         // je cree le nouvel objet contenant les pixels
         // en supposant le resultat est une image RGB (a modifier si ce n'est pas le cas)
         newPixels = new CPixelsRgb( naxis1, naxis2, FORMAT_FLOAT, pixOutR, pixOutG, pixOutB);

         // Je recupere les mots cles
         newKeywords->GetFromArray(nb_keys,&keynames,&values,&comments,&units,&datatypes);

         free(pixIn);
         Libtt_main(TT_PTR_FREEPTR,1,&pixOutR);
         Libtt_main(TT_PTR_FREEPTR,1,&pixOutG);
         Libtt_main(TT_PTR_FREEPTR,1,&pixOutB);
         break;

       default :
         throw CError( "CBuffer::TtImaSeries is not implemented for class of pixels %s",
            CPixels::getPixelClassName(this->pix->getPixelClass()));
      }

      // je supprime les anciens pixels du buffer
      delete this->pix;
      // j'affecte les nouveaus pixels au buffer
      this->pix = newPixels;

      delete this->keywords;
      this->keywords = newKeywords;


      // On reinitialise les parametres astrometriques
      p_ast->valid = 0;

      // On reinitialise les parametres astrometriques
      p_ast->valid = 0;

      keyword = keywords->FindKeyword("BITPIX");
      if(keyword==0) {
         saving_type = USHORT_IMG;
      } else {
         saving_type = keywords->FindKeyword("BITPIX")->GetIntValue();
      }
      if(saving_type==16) {
         keyword = keywords->FindKeyword("BZERO");
         if((keyword)&&(keyword->GetIntValue()==32768)) {
            saving_type = USHORT_IMG;
         }
      } else if(saving_type==32) {
         keyword = keywords->FindKeyword("BZERO");
         if((keyword)&&(keyword->GetIntValue()==-(2^31))) {
            saving_type = ULONG_IMG;
         }
      }
      // Liberation de la memoire allouee par libtt
      msg = Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      pthread_mutex_unlock(&mutex);

   } catch (const CError& e) {
      // je libere la memoire
      if(pixIn) free(pixIn);
      if(newPixels) delete newPixels;
      if(newKeywords) delete newKeywords;
      Libtt_main(TT_PTR_FREEPTR,1,&pixOut);
      Libtt_main(TT_PTR_FREEPTR,1,&pixOutR);
      Libtt_main(TT_PTR_FREEPTR,1,&pixOutG);
      Libtt_main(TT_PTR_FREEPTR,1,&pixOutB);
      Libtt_main(TT_PTR_FREEKEYS,5,&keynames,&values,&comments,&units,&datatypes);
      pthread_mutex_unlock(&mutex);
    // je transmets l'exception
      throw e;
   }

}

void CBuffer::Stat( int x1,int y1,int x2,int y2,
                   float *locut,  float *hicut,   float *maxi,
                   float *mini,   float *mean,    float *sigma,
                   float *bgmean, float *bgsigma, float *contrast)
{
   int msg, naxis1, naxis2;
   int datatype, iMin, iMax;
   float fHi, fLo;
   double dlocut, dhicut, dmaxi, dmini, dmean, dsigma, dbgmean,dbgsigma,dcontrast;
   int i,naxis11,naxis22;
   TYPE_PIXELS *ppix= NULL;

   try {
      pthread_mutex_lock(&mutex);

      if ( pix->IsPixelsReady() == 0 ) {
         // je retourne une erreur si le buffer est vide
         throw CError("buffer is empty");
      }
      naxis1=GetWidth();
      naxis2=GetHeight();

      datatype = TFLOAT;
      if ((x1==-1)&&(y1==-1)&&(x2==-1)&&(y2==-1)) {
         // x1=y1=x2=y2=-1 si l'on souhaite traiter toute l'image
         ppix = (TYPE_PIXELS*)malloc(naxis1*naxis2 * sizeof(TYPE_PIXELS));
         if (ppix==NULL) throw CError(ELIBSTD_NO_MEMORY_FOR_PIXELS);
         pix->GetPixels(0, 0, naxis1-1, naxis2-1 , FORMAT_FLOAT, PLANE_GREY, (void*) ppix);
         msg = Libtt_main(TT_PTR_STATIMA,13,ppix,&datatype,&naxis1,&naxis2,
            &dlocut,&dhicut,&dmaxi,&dmini,&dmean,&dsigma,&dbgmean,&dbgsigma,&dcontrast);
         free(ppix);
         if(msg) throw CErrorLibtt(msg);
      } else {
         // traite une fenetre dans l'image
         if((x1<0)||(x2<0)||(x1>naxis1-1)||(x2>naxis1-1)) {throw CError(ELIBSTD_X1X2_NOT_IN_1NAXIS1);}
         if((y1<0)||(y2<0)||(y1>naxis2-1)||(y2>naxis2-1)) {throw CError(ELIBSTD_Y1Y2_NOT_IN_1NAXIS2);}
         if(x1>x2) {i = x2; x2 = x1; x1 = i;}
         if(y1>y2) {i = y2; y2 = y1; y1 = i;}
         naxis11 = x2-x1+1;
         naxis22 = y2-y1+1;
         ppix = (TYPE_PIXELS*)malloc(naxis11*naxis22 * sizeof(TYPE_PIXELS));
         if (ppix==NULL) throw CError(ELIBSTD_NO_MEMORY_FOR_PIXELS);
         pix->GetPixels(x1, y1, x2, y2 , FORMAT_FLOAT, PLANE_GREY, (void*) ppix);
         msg = Libtt_main(TT_PTR_STATIMA,13,ppix,&datatype,&naxis11,&naxis22,
            &dlocut,&dhicut,&dmaxi,&dmini,&dmean,&dsigma,&dbgmean,&dbgsigma,&dcontrast);
         free(ppix);
         if(msg) throw CErrorLibtt(msg);
      }

      *locut = (float)dlocut; 
      *hicut = (float)dhicut;
      *maxi = (float)dmaxi; 
      *mini = (float)dmini;
      *mean = (float)dmean; 
      *sigma = (float)dsigma;
      *bgmean = (float)dbgmean; 
      *bgsigma = (float)dbgsigma;
      *contrast = (float)dcontrast;

      // si la statistique porte sur l'image enti�re, je mets a jour les mots cls de l'image
      if ((x1==-1)&&(y1==-1)&&(x2==-1)&&(y2==-1)) {
         fLo = (float) dlocut;
         fHi = (float) dhicut;
         iMin = (int)dmini;
         iMax = (int)dmaxi;
         keywords->Add("MIPS-HI",(void*)&fHi,TFLOAT,"High cut","ADU");
         keywords->Add("MIPS-LO",(void*)&fLo,TFLOAT,"Low cut","ADU");
         keywords->Add("CONTRAST",(void*)contrast,TFLOAT,"Pixel contrast","ADU");
         keywords->Add("MEAN",(void*)mean,TFLOAT,"Mean value","ADU");
         keywords->Add("SIGMA",(void*)sigma,TFLOAT,"Standard deviation","ADU");
         keywords->Add("DATAMAX",(void*)&iMax,TINT,"Maximum value","ADU");
         keywords->Add("DATAMIN",(void*)&iMin,TINT,"Minimum value","ADU");
         keywords->Add("BGMEAN",(void*)bgmean,TFLOAT,"Background mean value","ADU");
         keywords->Add("BGSIGMA",(void*)bgsigma,TFLOAT,"Background standard deviation","ADU");
      }
      pthread_mutex_unlock(&mutex);

   } catch (const CError& e) {
      pthread_mutex_unlock(&mutex);
      throw e;
   }


}

void CBuffer::Scar( int x1,int y1,int x2,int y2)
{
   int naxis1, naxis2;
   int i,j;
   double dltadu,dltpix,deb,fin;
   TYPE_PIXELS *ppix= NULL;
   int xx1,xx2,yy1,yy2;

   // seulement pour les images non couleur
   if( pix->getPixelClass() != CLASS_GRAY) {
      throw CError(ELIBSTD_NOT_IMPLEMENTED);
   }
   naxis1 = GetWidth();
   naxis2 = GetHeight();
   ppix = (TYPE_PIXELS *) malloc(naxis1* naxis2 * sizeof(float));
   pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_GREY, (void*) ppix);


   // traite une fenetre dans l'image
   if((x1<0)||(x2<0)||(x1>naxis1-1)||(x2>naxis1-1)) {throw CError(ELIBSTD_X1X2_NOT_IN_1NAXIS1);}
   if((y1<0)||(y2<0)||(y1>naxis2-1)||(y2>naxis2-1)) {throw CError(ELIBSTD_Y1Y2_NOT_IN_1NAXIS2);}
   if(x1>x2) {i = x2; x2 = x1; x1 = i;}
   if(y1>y2) {i = y2; y2 = y1; y1 = i;}
   yy1=y1;
   yy2=y2;
   if (y1==y2) {
      yy1--;
      yy2++;
   }
   if (yy2>yy1+1) {
         // couture horizontale
         dltpix=(double)(x2-x1);
      for(j=yy1+1;j<yy2;j++) {
            deb=(double)*(ppix+naxis1*j+x1);
            fin=(double)*(ppix+naxis1*j+x2);
            dltadu=(double)(fin-deb);
            for(i=x1+1;i<x2;i++) {
               *(ppix+naxis1*j+i)=(TYPE_PIXELS)(deb+dltadu*(i-x1)/dltpix);
            }
         }
   }
   xx1=x1;
   xx2=x2;
   if (x1==x2) {
      xx1--;
      xx2++;
   }
   if (xx2>xx1+1) {
         // couture verticale
         dltpix=(double)(y2-y1);
      for(i=xx1+1;i<xx2;i++) {
            deb=(double)*(ppix+naxis1*y1+i);
            fin=(double)*(ppix+naxis1*y2+i);
            dltadu=(double)(fin-deb);
            for(j=y1+1;j<y2;j++) {
              *(ppix+naxis1*j+i)=(TYPE_PIXELS) (( *(ppix+naxis1*j+i) + (deb+dltadu*(j-y1)/dltpix) )/2.);
            }
         }
      }

   //  memorisation des pixels
   //FreeBuffer(DONT_KEEP_KEYWORDS);
   SetPixels(PLANE_GREY, naxis1, naxis2, FORMAT_FLOAT, COMPRESS_NONE, ppix, 0, 0, 0);

   free(ppix);
}

void CBuffer::SyntheGauss(double xc, double yc, double imax, double jmax, double fwhmx, double fwhmy, double limitadu)
{
   int naxis1, naxis2;
   int i,j,x1,x2,y1,y2;
   double sigx,sigy,rx,ry,r,intmax;
   TYPE_PIXELS *ppix= NULL;

   // seulement pour les images non couleur
   if( pix->getPixelClass() != CLASS_GRAY) {
      throw CError(ELIBSTD_NOT_IMPLEMENTED);
   }
   naxis1 = GetWidth();
   naxis2 = GetHeight();
   ppix = (TYPE_PIXELS *) malloc(naxis1* naxis2 * sizeof(float));
   pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_GREY, (void*) ppix);

   // sig2 = 0.601*0.601*fwhmx,fwhmy
   // r2 = (x-xc)*(x-xc) + (y-yc)*(y-yc)
   // i(x,y) = i(x,y) + ( imax * exp ( -r2/sig2 ) )
   // imax peut etre negatif pour soustraire la gaussienne
   if ((fwhmx<=0.)||(fwhmy<=0.)||((imax==0.)&&(jmax==0.))) {
      return ;
   }
   sigx=fwhmx*0.601;
   sigy=fwhmy*0.601;
   if (limitadu==0.) {
      limitadu=1.;
   }
   intmax=(imax+jmax)/2.;
   if ((naxis1==1)&&(naxis2>1)) {
      intmax=jmax;
   } else if ((naxis1>1)&&(naxis2==1)) {
      intmax=imax;
   }
   r=-log(fabs(limitadu/intmax));
   if (r<=0.) {return ;}
   rx=sigx*sqrt(r);
   ry=sigy*sqrt(r);
   sigx*=sigx;
   sigy*=sigy;
   x1=(int)floor(xc-rx);
   x2=(int)ceil(xc+rx);
   y1=(int)floor(yc-ry);
   y2=(int)ceil(yc+ry);
   if (x1<0) {x1=0;}
   if (x1>naxis1-1) {x1=naxis1-1;}
   if (y1<0) {y1=0;}
   if (y1>naxis2-1) {y1=naxis2-1;}
   if (x2<0) {x2=0;}
   if (x2>naxis1-1) {x2=naxis1-1;}
   if (y2<0) {y2=0;}
   if (y2>naxis2-1) {y2=naxis2-1;}
   for(j=y1;j<=y2;j++) {
      ry=(double)j-yc;
      ry*=ry;
      for(i=x1;i<=x2;i++) {
         rx=(double)i-xc;
         rx*=rx;
         *(ppix+naxis1*j+i)=(TYPE_PIXELS)( *(ppix+naxis1*j+i) + intmax*exp(-rx/sigx-ry/sigy) );
      }
   }

   //  memorisation des pixels
   SetPixels(PLANE_GREY, naxis1, naxis2, FORMAT_FLOAT, COMPRESS_NONE, ppix, 0, 0, 0);

   free(ppix);

}

void CBuffer::Histogram(int n, float *adus, float *meanadus, long *histo,
                       int ismini,float mini,int ismaxi,float maxi)
{

    pix->Histogram(n, adus, meanadus, histo, ismini, mini, ismaxi, maxi);
}

void CBuffer::radec2xy(double ra, double dec, double *x, double *y,int order)
{
   double sindec,cosdec,sindec0,cosdec0,cosrara0,sinrara0;
   double h,det;
   double dra,ddec;
   double PI = (atan(1.)*4.);
   double x0,y0,xp,yp;

   if (p_ast==NULL) {
      throw CError(ELIBSTD_NO_ASTROMPARAMS) ;
   }

   if(p_ast->valid==0) {
       FillAstromParams();
   }


   /* --- Conversion deg -> rad --- */
   ra *= (double)(PI/180.);
   dec *= (double)(PI/180.);

   /* --- passage ra,dec -> x,y ---*/
   sindec=sin(dec);
   cosdec=cos(dec);
   sindec0 = sin(p_ast->crval2);
   cosdec0 = cos(p_ast->crval2);
   cosrara0 = cos(ra-p_ast->crval1);
   sinrara0 = sin(ra-p_ast->crval1);
   h = sindec*sindec0+cosdec*cosdec0*cosrara0;
   dra = cosdec*sinrara0/h;
   ddec = (sindec*cosdec0-cosdec*sindec0*cosrara0)/h;
   det = p_ast->cd22 * p_ast->cd11 - p_ast->cd12 * p_ast->cd21;
   if(det==0) {
      *x = *y = (double)0.;
   } else {
      *x = (double)(p_ast->crpix1 - (p_ast->cd12*ddec-p_ast->cd22*dra) / det -0.5);
      *y = (double)(p_ast->crpix2 + (p_ast->cd11*ddec-p_ast->cd21*dra) / det -0.5);
   }
   /* --- cas de distorsion ---*/
   if ((order==2)&&(p_ast->pv_valid==1)) {
      x0=*x;
      y0=*y;
      xp = p_ast->pv[1][1]*x0 + p_ast->pv[1][2]*y0 + p_ast->pv[1][0] + p_ast->pv[1][5]*x0*y0 + p_ast->pv[1][4]*x0*x0 + p_ast->pv[1][6]*y0*y0 + p_ast->pv[1][7]*x0*x0*x0 + p_ast->pv[1][8]*x0*x0*y0 + p_ast->pv[1][9]*x0*y0*y0 + p_ast->pv[1][10]*y0*y0*y0;
      yp = p_ast->pv[2][2]*x0 + p_ast->pv[2][1]*y0 + p_ast->pv[2][0] + p_ast->pv[2][5]*x0*y0 + p_ast->pv[2][6]*x0*x0 + p_ast->pv[2][4]*y0*y0 + p_ast->pv[2][10]*x0*x0*x0 + p_ast->pv[2][9]*x0*x0*y0 + p_ast->pv[2][8]*x0*y0*y0 + p_ast->pv[2][7]*y0*y0*y0;
      x0-=(xp-x0);
      y0-=(yp-y0);
      *x=x0;
      *y=y0;
   }
}


void CBuffer::xy2radec(double x, double y, double *ra, double *dec,int order)
{
   double delta,gamma;
   double dra,ddec;
   double PI = (atan(1.)*4.);
   double xp,yp;

   if (p_ast==NULL) {
      throw CError(ELIBSTD_NO_ASTROMPARAMS);
   }
   if(p_ast->valid==0) {
      FillAstromParams();
   }

   /* --- cas de distorsion ---*/
   if ((order==2)&&(p_ast->pv_valid==1)) {
      xp = p_ast->pv[1][1]*x + p_ast->pv[1][2]*y + p_ast->pv[1][0] + p_ast->pv[1][5]*x*y + p_ast->pv[1][4]*x*x + p_ast->pv[1][6]*y*y + p_ast->pv[1][7]*x*x*x + p_ast->pv[1][8]*x*x*y + p_ast->pv[1][9]*x*y*y + p_ast->pv[1][10]*y*y*y;
      yp = p_ast->pv[2][2]*x + p_ast->pv[2][1]*y + p_ast->pv[2][0] + p_ast->pv[2][5]*x*y + p_ast->pv[2][6]*x*x + p_ast->pv[2][4]*y*y + p_ast->pv[2][10]*x*x*x + p_ast->pv[2][9]*x*x*y + p_ast->pv[2][8]*x*y*y + p_ast->pv[2][7]*y*y*y;
      if ((y>100)&&(y<200)) {
         x=1.*x;
      }
      x=xp;
      y=yp;
   }
   /* --- passage x,y -> ra,dec ---*/
   x=x+.5;
   y=y+.5;
   dra = p_ast->cd11*(x-p_ast->crpix1) + p_ast->cd12*(y-p_ast->crpix2);
   ddec = p_ast->cd21*(x-p_ast->crpix1) + p_ast->cd22*(y-p_ast->crpix2);
   delta = cos(p_ast->crval2) - ddec*sin(p_ast->crval2);
   gamma = sqrt(dra*dra+delta*delta);
   *ra = (double)(p_ast->crval1 + atan(dra/delta));
   if(*ra<0) {
      *ra += (double)(2*PI);
   }
   if(*ra>(double)(2*PI)) {
      *ra -= (double)(2*PI);
   }
   *dec = (double)(atan((sin(p_ast->crval2)+ddec*cos(p_ast->crval2))/gamma));

   /* --- Conversion rad -> deg --- */
   *ra *= (double)(180./PI);
   *dec *= (double)(180./PI);

}



void CBuffer::Fwhm(int x1, int y1, int x2, int y2,
                  double *maxx, double *posx, double *fwhmx, double *fondx, double *errx,
                  double *maxy, double *posy, double *fwhmy, double *fondy, double *erry,
				  double fwhmx0, double fwhmy0)
{
   pthread_mutex_lock(&mutex);
   pix->Fwhm(x1, y1, x2, y2,
                  maxx, posx, fwhmx, fondx, errx,
                  maxy, posy, fwhmy, fondy, erry,
				  fwhmx0, fwhmy0);
   pthread_mutex_unlock(&mutex);
}



void CBuffer::MedX(int , int , int )
{
#if 0
   int temp;
   int w, h, x, y;
   TYPE_PIXELS value;
   TYPE_PIXELS *out;

   w=GetWidth();
   h=GetHeight();

   if (width < 0)
   {
      throw CError(ELIBSTD_WIDTH_POSITIVE);
   }

   if ((x1 < 0) || (x2 < 0) || (x1 > w-1) || (x2 > w-1))
   {
      throw CError( ELIBSTD_X1X2_NOT_IN_1NAXIS1);
   }

   if(x1 > x2)
   {
       temp = x2;
       x2 = x1;
       x1 = temp;
   }

//   out = (TYPE_PIXELS*)calloc(width*h,sizeof(TYPE_PIXELS));

   // Mettre ici le traitement.

//   memcpy(pix,out,width*h*sizeof(TYPE_PIXELS));
//   free(out);
#endif
}


void CBuffer::MedY(int , int , int )
{
#if 0
   int temp;
   int w, h, x, y;
   TYPE_PIXELS value;
   TYPE_PIXELS *out;

   if(pix==NULL) {return ELIBSTD_BUF_EMPTY;}
   w=GetWidth();
   h=GetHeight();


   if (height < 0)
   {
         throw CError(  ELIBSTD_HEIGHT_POSITIVE);

   }
   if ((y1 < 0) || (y2 < 0) || (y1 > h-1) || (y2 > h-1))
   {
         throw CError(  ELIBSTD_Y1Y2_NOT_IN_1NAXIS2);
   }

   if(y1 > y2)
   {
       temp = y2;
       y2 = y1;
       y1 = temp;
   }

#endif
}


void CBuffer::Window(int x1, int y1, int x2, int y2)
{
   pix->Window(x1, y1, x2, y2);
}
int CBuffer::A_StarList(int x1, int y1, int x2, int y2, double threshin,char *filename, int fileFormat, double fwhm,int radius,
						int border,double threshold,int after_gauss)
{
   int i,retour;
   int gmsize = 2*radius + 1;
   TYPE_PIXELS *temp_pic,*gauss_matrix;
   TYPE_PIXELS *ppix= NULL;
   int naxis1,naxis2,width,height;

   naxis1 = GetWidth();
   naxis2 = GetHeight();
   if ((x1==-1)&&(y1==-1)&&(x2==-1)&&(y2==-1)) {
      x1 = 0;
      y1 = 0;
      x2 = naxis1-1;
      y2 = naxis2-1;
   } else {
      if((x1<0)||(x2<0)||(x1>naxis1-1)||(x2>naxis1-1)) {throw CError(ELIBSTD_X1X2_NOT_IN_1NAXIS1);}
      if((y1<0)||(y2<0)||(y1>naxis2-1)||(y2>naxis2-1)) {throw CError(ELIBSTD_Y1Y2_NOT_IN_1NAXIS2);}
      if(x1>x2) {i = x2; x2 = x1; x1 = i;}
      if(y1>y2) {i = y2; y2 = y1; y1 = i;}
   }
   width  = x2-x1+1;
   height = y2-y1+1;

   ppix = (TYPE_PIXELS *) malloc(width * height * sizeof(float));
   // Yassine
   //pix->GetPixels(x1, y1, x2, y2, FORMAT_FLOAT, PLANE_GREY, (int) ppix);
   pix->GetPixels(x1, y1, x2, y2, FORMAT_FLOAT, PLANE_GREY, (void*) ppix);

   if((temp_pic = (TYPE_PIXELS *)calloc(width*height,sizeof(TYPE_PIXELS)))==NULL) {
      free(ppix);
	   throw CError(ELIBSTD_NO_MEMORY_FOR_KWDS);
   }

   if((gauss_matrix = (TYPE_PIXELS *)malloc(sizeof(TYPE_PIXELS)*gmsize*gmsize))==NULL) {
      free(ppix);
      free(temp_pic);
      throw CError(ELIBSTD_NO_MEMORY_FOR_KWDS);
   }

   retour = A_filtrGauss ((TYPE_PIXELS)fwhm, radius, (TYPE_PIXELS)threshin,
      (TYPE_PIXELS)threshold, filename, fileFormat,
      ppix,temp_pic,gauss_matrix,
      width,height,gmsize,border);

   if(after_gauss!=0 && retour>=0) {
      CPixelsGray * newpix;
      newpix = new CPixelsGray(naxis1, naxis2, FORMAT_SHORT, temp_pic, 0, 0);

      delete pix;
      pix = newpix;

   }

	free(temp_pic);
   free(gauss_matrix);
   free(ppix);

   return retour;

}


int CBuffer::A_filtrGauss (TYPE_PIXELS fwhm, int radius, TYPE_PIXELS threshin,
						   TYPE_PIXELS threshold, char *filename, int fileFormat,
						   TYPE_PIXELS *picture,TYPE_PIXELS *temp_pic,TYPE_PIXELS *gauss_matrix,
						   int size_x,int size_y,int gmsize,int border)
{

/*
  Authors: Bogumil and Dorota
*/

   int x, y, i;
   int center = radius;
   double sig = 0.4246*fwhm;
   double dsig2 = 2*sig*sig;
   double dpisig2 = 3.14159265358979*dsig2; //=2*pi*sig^2
   double summ = 0.0;
   int nbStar = 0; 

   /* Creation of matrix with Gauss/normal distribution values */
   for (y=0; y<gmsize; y++)
   {
	   for (x=0; x<gmsize; x++)
	   {
		   gauss_matrix[x+y*gmsize] = (TYPE_PIXELS)(
			   exp( -((x-center)*(x-center) + (y-center)*(y-center)) / dsig2)
			   /dpisig2);
		   summ += gauss_matrix[x+y*gmsize];
	   }
   }

   double summa = 0.0;

   /* Dodatkowa normalizacja wynikajaca z nie nieskonczonej macierzy
   i sprowadzenie do postaci, w ktorej suma elem. rowna jest zero */

   /* Additional normalization - sum of elements is zero */
   for (y=0; y<gmsize; y++)
   {
	   for (x=0; x<gmsize; x++)
	   {
		   gauss_matrix[x+y*gmsize] = (TYPE_PIXELS)(gauss_matrix[x+y*gmsize]/summ
			   - (1.0 / (gmsize*gmsize)));
		   summa += gauss_matrix[x+y*gmsize];
	   }
   }

   long pixcount=0; //stores 'how many pixel were calculated' information

   /* For each pixel that is above 'threshin' multiply surrounding
   pixels by corresponding 'gauss_matrix' values and store
   the summation in 'temp_pic' table (float) */

   for (y=radius; y < (size_y-radius) ; y++)
		for (x=radius; x < (size_x-radius) ; x++)
		{
			if ( picture[x+y*size_x] > threshin)
			{
				for (int j=0; j<gmsize; j++)
					for (int i=0; i<gmsize; i++)
					{
						temp_pic[x+y*size_x] +=
							picture[x-center+i+(y-center+j)*size_x]*gauss_matrix[i+j*gmsize];
					}
					pixcount++;
			}
			else temp_pic[x+y*size_x] = (float)0.0;
		}

   i = 1;
   int border1 = border + 1;    // +1 because when searching for maksimum,
   // we use surrounding pixels

   if ( fileFormat == 1 ) {
      FILE *fout;
      fout=fopen(filename,"wt");
      if (fout == NULL) {
         return ELIBSTD_CANT_OPEN_FILE;
      }

      //looking for stars (max. values), now is not very precise
      for (y=border1; y < size_y-border1; y++) {
         for (x=border1; x < size_x-border1; x++)
         {
            TYPE_PIXELS temp_p=temp_pic[x+y*size_x];
            if(
               (threshold < temp_p)
               && (temp_pic[x-1+y*size_x] < temp_p)
               && (temp_pic[x+1+y*size_x] < temp_p)
               && (temp_pic[x+(y-1)*size_x] < temp_p)
               && (temp_pic[x+(y+1)*size_x] < temp_p)
               && (temp_pic[x+1+(y+1)*size_x] < temp_p)
               && (temp_pic[x-1+(y+1)*size_x] < temp_p)
               && (temp_pic[x-1+(y-1)*size_x] < temp_p)
               && (temp_pic[x+1+(y-1)*size_x] < temp_p)
               )
            {
               if(fout != NULL) {
                  fprintf(fout, "%d     %d     %d     %f     %f\n",i,x, y,temp_p , (22-2.5*log10(temp_p)) );
               }
               i++;
            }
         }
      }

      if(fout != NULL) {
         fclose(fout);
      }
      nbStar = i;
   }

   if ( fileFormat == 2 ) {
      int nstar =pixcount;
      char *columnData[13];
      char value[100];
      double xmax, ymax;
      int nbRow2, nbCol, col;
      char columnTypes[100];
      char **columnUnits = NULL;
      char **columnTitle = NULL;
      int msg;

      nbCol =13;

      strcpy(columnTypes,"DOUBLE DOUBLE SHORT DOUBLE DOUBLE DOUBLE DOUBLE DOUBLE DOUBLE DOUBLE DOUBLE DOUBLE DOUBLE");

      if (nbCol>0) {
         msg = Libtt_main(TT_PTR_ALLOTBL,4,columnTypes,&nbCol,&columnUnits,&columnTitle);
         if(msg) {
            throw CErrorLibtt(msg);
         }
      }

      strcpy(columnUnits[0],"pixel");
      strcpy(columnUnits[1],"pixel");
      strcpy(columnUnits[2],"identification symbol");
      strcpy(columnUnits[3],"adu");
      strcpy(columnUnits[4],"deg");
      strcpy(columnUnits[5],"deg");
      strcpy(columnUnits[6],"mag");
      strcpy(columnUnits[7],"adu");
      strcpy(columnUnits[8],"pixel");
      strcpy(columnUnits[9],"pixel");
      strcpy(columnUnits[10],"adu");
      strcpy(columnUnits[11],"none");
      strcpy(columnUnits[12],"deg");

      strcpy(columnTitle[0],"x coordinate");
      strcpy(columnTitle[1],"y coordinate");
      strcpy(columnTitle[2],"pixel identification");
      strcpy(columnTitle[3],"flux");
      strcpy(columnTitle[4],"right ascension");
      strcpy(columnTitle[5],"declination");
      strcpy(columnTitle[6],"magnitude");
      strcpy(columnTitle[7],"background");
      strcpy(columnTitle[8],"fwhmx");
      strcpy(columnTitle[9],"fwhmy");
      strcpy(columnTitle[10],"intensity");
      strcpy(columnTitle[11],"ab ratio");
      strcpy(columnTitle[12],"position angle");

      for (col=0 ; col < nbCol; col++ ) {
         columnData[col] = (char*) calloc(nstar, 13);
         if (columnData[col] == NULL ){ throw CError("Could not calloc %d bytes for columnData", nstar*12); }
      }


      xmax = GetWidth();
      ymax = GetHeight();
      nbRow2 = 0;

      //looking for stars (max. values), now is not very precise
      for (y=border1; y < size_y-border1; y++) {
         for (x=border1; x < size_x-border1; x++)
         {
            TYPE_PIXELS temp_p=temp_pic[x+y*size_x];
            if(
               (threshold < temp_p)
               && (temp_pic[x-1+y*size_x] < temp_p)
               && (temp_pic[x+1+y*size_x] < temp_p)
               && (temp_pic[x+(y-1)*size_x] < temp_p)
               && (temp_pic[x+(y+1)*size_x] < temp_p)
               && (temp_pic[x+1+(y+1)*size_x] < temp_p)
               && (temp_pic[x-1+(y+1)*size_x] < temp_p)
               && (temp_pic[x-1+(y-1)*size_x] < temp_p)
               && (temp_pic[x+1+(y-1)*size_x] < temp_p)
               )
            {
               //printf(fout, "%d     %d     %d     %d     %f\n",i,x, y,temp_p , (22-2.5*log10(temp_p)) );
               nbRow2++;
               sprintf(value,"%11.4e ", ((double)x));
               strcat(columnData[0],value);
               sprintf(value,"%11.4e ", ((double)y));
               strcat(columnData[1],value);
               strcat(columnData[2],"1 ");
               sprintf(value,"%11.4e ", temp_p);
               strcat(columnData[3],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[4],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[5],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[6],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[7],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[8],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[9],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[10],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[11],value);
               sprintf(value,"%11.4e ", 1.0);
               strcat(columnData[12],value);
            }
         }
      }

      try {
         if ( nbRow2 > 0 ) {
            CFile::saveFitsTable(filename, this->keywords, nbRow2, nbCol, columnTypes, columnUnits,  columnTitle, columnData);
         }
      } catch (const CError& e) {
         for (col=0 ; col < nbCol; col++ ) {
            free(columnData[col]);
         }
         msg = Libtt_main(TT_PTR_FREETBL,2,&columnUnits,&columnTitle);
         throw e;
      }

      for (col=0 ; col < nbCol; col++ ) {
         free(columnData[col]);
      }

      Libtt_main(TT_PTR_FREETBL,2,&columnUnits,&columnTitle);
      nbStar = nbRow2;
   }


   return nbStar;  //number of stars
}

int CBuffer::A_filtrGauss2 (TYPE_PIXELS fwhm, int radius, TYPE_PIXELS threshin,
						   TYPE_PIXELS threshold, 
						   TYPE_PIXELS *picture,
						   int size_x,int size_y,int border, double *xCenter, double *yCenter)
{

/*
  Authors: Bogumil and Dorota
*/

   int x, y, i;
   int center = radius;
   double sig = 0.4246*fwhm;
   double dsig2 = 2*sig*sig;
   double dpisig2 = 3.14159265358979*dsig2; //=2*pi*sig^2
   double summ = 0.0;
   int nbStar = 0; 
   TYPE_PIXELS *temp_pic;
   TYPE_PIXELS *gauss_matrix;
   int gmsize = 2*radius + 1;

   if((temp_pic = (TYPE_PIXELS *)calloc(size_x*size_y,sizeof(TYPE_PIXELS)))==NULL) {
	   throw CError(ELIBSTD_NO_MEMORY_FOR_KWDS);
   }

   if((gauss_matrix = (TYPE_PIXELS *)malloc(sizeof(TYPE_PIXELS)*gmsize*gmsize))==NULL) {
      free(temp_pic);
      throw CError(ELIBSTD_NO_MEMORY_FOR_KWDS);
   }

   /* Creation of matrix with Gauss/normal distribution values */
   for (y=0; y<gmsize; y++)
   {
	   for (x=0; x<gmsize; x++)
	   {
		   gauss_matrix[x+y*gmsize] = (TYPE_PIXELS)(
			   exp( -((x-center)*(x-center) + (y-center)*(y-center)) / dsig2)
			   /dpisig2);
		   summ += gauss_matrix[x+y*gmsize];
	   }
   }

   double summa = 0.0;

   /* Dodatkowa normalizacja wynikajaca z nie nieskonczonej macierzy
   i sprowadzenie do postaci, w ktorej suma elem. rowna jest zero */

   /* Additional normalization - sum of elements is zero */
   for (y=0; y<gmsize; y++)
   {
	   for (x=0; x<gmsize; x++)
	   {
		   gauss_matrix[x+y*gmsize] = (TYPE_PIXELS)(gauss_matrix[x+y*gmsize]/summ
			   - (1.0 / (gmsize*gmsize)));
		   summa += gauss_matrix[x+y*gmsize];
	   }
   }

   long pixcount=0; //stores 'how many pixel were calculated' information

   /* For each pixel that is above 'threshin' multiply surrounding
   pixels by corresponding 'gauss_matrix' values and store
   the summation in 'temp_pic' table (float) */

   for (y=radius; y < (size_y-radius) ; y++) {
		for (x=radius; x < (size_x-radius) ; x++)
		{
			if ( picture[x+y*size_x] > threshin)
			{
				for (int j=0; j<gmsize; j++)
					for (int i=0; i<gmsize; i++)
					{
						temp_pic[x+y*size_x] +=
							picture[x-center+i+(y-center+j)*size_x]*gauss_matrix[i+j*gmsize];
					}
					pixcount++;
			}
			else temp_pic[x+y*size_x] = (float)0.0;
		}
   }

   i = 0;
   int border1 = border + 1;    // +1 because when searching for maksimum,
   // we use surrounding pixels
   TYPE_PIXELS intensity = 0;
   
      //looking for stars (max. values), now is not very precise
      for (y=border1; y < size_y-border1; y++) {
         for (x=border1; x < size_x-border1; x++)
         {
            TYPE_PIXELS temp_p=temp_pic[x+y*size_x];
            if(
               (threshold < temp_p)
               && (temp_pic[x-1+y*size_x] < temp_p)
               && (temp_pic[x+1+y*size_x] < temp_p)
               && (temp_pic[x+(y-1)*size_x] < temp_p)
               && (temp_pic[x+(y+1)*size_x] < temp_p)
               && (temp_pic[x+1+(y+1)*size_x] < temp_p)
               && (temp_pic[x-1+(y+1)*size_x] < temp_p)
               && (temp_pic[x-1+(y-1)*size_x] < temp_p)
               && (temp_pic[x+1+(y-1)*size_x] < temp_p)
               )
            {
               if(intensity < temp_p) {
                  intensity = temp_p;
                  *xCenter = x;
                  *yCenter = y;
               }
               i++;
            }
         }
      }

    
      nbStar = i;
 
   free(gauss_matrix);
   free(temp_pic);

   return nbStar;
}

void CBuffer::Fwhm2d(int x1, int y1, int x2, int y2,
                  double *maxx, double *posx, double *fwhmx, double *fondx, double *errx,
                  double *maxy, double *posy, double *fwhmy, double *fondy, double *erry,
				  double fwhmx0, double fwhmy0)
{
   pix->Fwhm2d(x1, y1, x2, y2,
                  maxx, posx, fwhmx, fondx, errx,
                  maxy, posy, fwhmy, fondy, erry,
				      fwhmx0, fwhmy0);
}


//***************************************************
// unifybg
//
// - decoupe l'image en fenetres
// - calcule le fond de ciel pour chaque fenetre
// - construit une matrice du fond du ciel pour chaque pixel
//   par interpolation entre les centres des rectangles
//
//***************************************************

void CBuffer::UnifyBg()
{
   pix->UnifyBg();
}

//***************************************************
// SubStars
//
// - elimine les etoiles a partir d'un fichier ASCII en x,y
// - n'elimine pas les etoiles a l'interieur d'un rayon centre xc,yc
//
//***************************************************
void CBuffer::SubStars(FILE *fascii, int indexcol_x, int indexcol_y, int indexcol_bg, double radius, double xc_exclu, double yc_exclu, double radius_exclu,int *n)
{
   int naxis1, naxis2;
   int i,j,x1,x2,y1,y2,col,nn,np;
   double r2,dx,dy;
   TYPE_PIXELS *ppix= NULL;
   char ligne[300],*lig,sep[10];
   double x,y,bg,bg1,bg2,bgg[4];
   double xc1,xc2,yc1,yc2,a,b,c,aa,bb,sdelta,vx,vy,d,percent;
   int nb;

   // seulement pour les images non couleur
   if( pix->getPixelClass() != CLASS_GRAY) {
      throw CError(ELIBSTD_NOT_IMPLEMENTED);
   }
   naxis1 = GetWidth();
   naxis2 = GetHeight();
   ppix = (TYPE_PIXELS *) malloc(naxis1* naxis2 * sizeof(float));
   pix->GetPixels(0, 0, naxis1-1, naxis2-1, FORMAT_FLOAT, PLANE_GREY, (void*) ppix);

   radius_exclu=radius_exclu*radius_exclu;
   strcpy(sep," ");
   nn=0;
   while (!feof(fascii)) {
      if (fgets(ligne,300,fascii)==NULL) {
         continue;
      }
      /* --- recherche les coordonnees x,y ---*/
      x=-1;
      y=-1;
      bg=-1e9;
      lig=strtok(ligne,sep);
      col=1;
      if (col==indexcol_x) { x=atof(lig)-1; }
      if (col==indexcol_y) { y=atof(lig)-1; }
      if (col==indexcol_bg) { bg=atof(lig); }
      while (lig!=NULL) {
         lig=strtok(NULL,sep);
         if (lig!=NULL) {
            col++;
         } else {
            break;
         }
         if (col==indexcol_x) { x=atof(lig)-1; }
         if (col==indexcol_y) { y=atof(lig)-1; }
         if (col==indexcol_bg) { bg=atof(lig); }
      }
      if ((x==-1)||(y==-1)) {
         continue;
      }
      /* --- test si l'etoile est dans la zone d'exclusion ---*/
      if (radius_exclu>0) {
         dx=x-xc_exclu;
         dy=y-yc_exclu;
         r2=dx*dx+dy*dy;
         if (r2<radius_exclu) {
            continue;
         }
      }
      /* --- test si le fond est connu ---*/
      if (bg==-1e9) {
         d=3*radius;
         np=0;
         percent=0.5;
         if (radius_exclu>0) {
            /* --- mesure le fond sur les cotes perpendiculaire a l'axe ---*/
            vx=x-xc_exclu;
            vy=y-yc_exclu;
            if (fabs(vy)>1e-5) {
               bb=(vx*x+vy*y)/vy;
               aa=-vx/vy;
               a=1+aa*aa;
               b=2*aa*bb-2*x-2*aa*y;
               c=x*x+(y-bb)*(y-bb)-d*d;
               sdelta=sqrt(b*b-4*a*c);
               xc1=(-b+sdelta)/2/a;
               yc1=bb+aa*xc1;
               xc2=(-b-sdelta)/2/a;
               yc2=bb+aa*xc2;
            } else {
               bb=(vx*x+vy*y)/vx;
               aa=-vy/vx;
               a=1+aa*aa;
               b=2*aa*bb-2*y-2*aa*x;
               c=y*y+(x-bb)*(x-bb)-d*d;
               sdelta=sqrt(b*b-4*a*c);
               yc1=(-b+sdelta)/2/a;
               xc1=bb+aa*yc1;
               yc2=(-b-sdelta)/2/a;
               xc2=bb+aa*yc2;
            }
            np=0;
            bg=0.;
            BoxBackground(ppix,xc1,yc1,radius,percent,&nb,&bg1); if (nb>2) { bgg[np]=bg1; np++; }
            BoxBackground(ppix,xc2,yc2,radius,percent,&nb,&bg2); if (nb>2) { bgg[np]=bg2; np++; }
            if (np>0) {
               util_qsort_double(bgg,0,np,NULL);
               bg=bgg[0];
            }
         }
         if (np==0) {
            bg=0.;
            xc1=x-d; yc1=y;   BoxBackground(ppix,xc1,yc1,radius,percent,&nb,&bg1); if (nb>2) { bgg[np]=bg1; np++; }
            xc1=x  ; yc1=y-d; BoxBackground(ppix,xc1,yc1,radius,percent,&nb,&bg1); if (nb>2) { bgg[np]=bg1; np++; }
            xc1=x+d; yc1=y;   BoxBackground(ppix,xc1,yc1,radius,percent,&nb,&bg1); if (nb>2) { bgg[np]=bg1; np++; }
            xc1=x  ; yc1=y+d; BoxBackground(ppix,xc1,yc1,radius,percent,&nb,&bg1); if (nb>2) { bgg[np]=bg1; np++; }
            if (np>0) {
               util_qsort_double(bgg,0,np,NULL);
               bg=bgg[0];
            } else {
               bg=0.;
            }
         }
      }
      /* --- remplace par la valeur du fond ---*/
      x1=(int)floor(x-radius);
      x2=(int)ceil(x+radius);
      y1=(int)floor(y-radius);
      y2=(int)ceil(y+radius);
      if (x1<0) { x1=0; }
      if (x2>naxis1-1) { x2=naxis1-1; }
      if (y1<0) { y1=0; }
      if (y2>naxis2-1) { y2=naxis2-1; }
      for(i=x1;i<x2;i++) {
         dx=i-x;
         for(j=y1;j<y2;j++) {
            dy=j-y;
            r2=sqrt(dx*dx+dy*dy);
            if (r2<radius) {
               *(ppix+naxis1*j+i)=(TYPE_PIXELS)(bg);
            }
         }
      }
      nn++;
   }
   *n=nn;

   //  memorisation des pixels
   SetPixels(PLANE_GREY, naxis1, naxis2, FORMAT_FLOAT, COMPRESS_NONE, ppix, 0, 0, 0);

   free(ppix);

}

void CBuffer::BoxBackground(TYPE_PIXELS *ppix,double xc,double yc,double radius,double percent,int *nb,double *bg)
{
   int naxis1,naxis2;
   int i,j,x1,x2,y1,y2,n,nn;
   double r2,dx,dy,*vec=NULL;

   *bg=0.;
   if (percent<0) { percent=0.; }
   if (percent>1) { percent=1.; }
   vec=(double*)calloc((int)((2*ceil(radius)+1)*(2*ceil(radius)+1)),sizeof(double));
   naxis1 = GetWidth();
   naxis2 = GetHeight();
   x1=(int)floor(xc-radius);
   x2=(int)ceil(xc+radius);
   y1=(int)floor(yc-radius);
   y2=(int)ceil(yc+radius);
   if (x1<0) { x1=0; }
   if (x2>naxis1-1) { x2=naxis1-1; }
   if (y1<0) { y1=0; }
   if (y2>naxis2-1) { y2=naxis2-1; }
   n=0;
   for(i=x1;i<x2;i++) {
      dx=i-xc;
      for(j=y1;j<y2;j++) {
         dy=j-yc;
         r2=sqrt(dx*dx+dy*dy);
         if (r2<radius) {
            vec[n]=(double)*(ppix+naxis1*j+i);
            n++;
         }
      }
   }
   if (n>0) {
      util_qsort_double(vec,0,n,NULL);
      nn=(int)(floor((n-1)*percent));
      *bg=vec[nn];
   }
   *nb=n;
   free(vec);
}

int CBuffer::util_qsort_double(double *x,int kdeb,int n,int *index)
/***************************************************************************/
/* Quick sort pour un tableau de double                                    */
/***************************************************************************/
/* x est le tableau qui commence a l'indice 1                              */
/* kdeb la valeur de l'indice a partir duquel il faut trier                */
/* n est le nombre d'elements                                              */
/* index est le tableau des indices une fois le tri effectue (=NULL si on  */
/*  ne veut pas l'utiliser).                                               */
/***************************************************************************/
{
   double qsort_r[50],qsort_l[50];
   int s,l,r,i,j,kfin;
   double v,w;
   int wi;
   int kt1,kt2,kp;
   double m,a;
   int mi,ai;
   kfin=n+kdeb-1;
   /* --- retour immediat si n==1 ---*/
   if (n==1) { return(0); }
   if (index!=NULL) {
      /*--- effectue un tri simple si n<=15 ---*/
      if (n<=15) {
         for (kt1=kdeb;kt1<kfin;kt1++) {
            m=x[kt1];
            mi=index[kt1];
            kp=kt1;
            for (kt2=kt1+1;kt2<=kfin;kt2++) {
               if (x[kt2]<m) {
                  m=x[kt2];
                  mi=index[kt2];
                  kp=kt2;
               }
            }
            a=x[kt1];x[kt1]=m;x[kp]=a;
            ai=index[kt1];index[kt1]=mi;index[kp]=ai;
         }
         return(0);
      }
      /*--- trie le tableau dans l'ordre croissant avec quick sort ---*/
      s=kdeb; qsort_l[kdeb]=kdeb; qsort_r[kdeb]=kfin;
      do {
         l=(int)(qsort_l[s]); r=(int)(qsort_r[s]);
         s=s-1;
         do {
            i=l; j=r;
            v=x[(int) (floor((double)(l+r)/(double)2))];
            do {
               while (x[i]<v) {i++;}
               while (v<x[j]) {j--;}
               if (i<=j) {
                  w=x[i];x[i]=x[j];x[j]=w;
                  wi=index[i];index[i]=index[j];index[j]=wi;
                  i++; j--;
               }
            } while (i<=j);
            if ((j-l)>=(r-i)) {if (i<r) {s++ ; qsort_l[s]=i ; qsort_r[s]=r;} r=j; } else { if (l<j) {s++ ; qsort_l[s]=l ; qsort_r[s]=j;} l=i; }
         } while (l<r);
      } while (s!=(kdeb-1)) ;
      return(0);
   } else {
      /*--- effectue un tri simple si n<=15 ---*/
      if (n<=15) {
         for (kt1=kdeb;kt1<kfin;kt1++) {
            m=x[kt1];
            kp=kt1;
            for (kt2=kt1+1;kt2<=kfin;kt2++) {
               if (x[kt2]<m) {
                  m=x[kt2];
                  kp=kt2;
               }
            }
            a=x[kt1];x[kt1]=m;x[kp]=a;
         }
         return(0);
      }
      /*--- trie le tableau dans l'ordre croissant avec quick sort ---*/
      s=kdeb; qsort_l[kdeb]=kdeb; qsort_r[kdeb]=kfin;
      do {
         l=(int)(qsort_l[s]); r=(int)(qsort_r[s]);
         s=s-1;
         do {
            i=l; j=r;
            v=x[(int) (floor((double)(l+r)/(double)2))];
            do {
               while (x[i]<v) {i++;}
               while (v<x[j]) {j--;}
               if (i<=j) {
                  w=x[i];x[i]=x[j];x[j]=w;
                  i++; j--;
               }
            } while (i<=j);
            if ((j-l)>=(r-i)) {if (i<r) {s++ ; qsort_l[s]=i ; qsort_r[s]=r;} r=j; } else { if (l<j) {s++ ; qsort_l[s]=l ; qsort_r[s]=j;} l=i; }
         } while (l<r);
      } while (s!=(kdeb-1)) ;
      return(0);
   }
}


