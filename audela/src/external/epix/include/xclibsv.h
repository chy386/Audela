/* 
 * 
 *	xclibsv.h	External	09-Jan-2004 
 * 
 *	Copyright (C)  1999-2001  EPIX, Inc.  All rights reserved. 
 * 
 *	PIXCI(R) Library: API, Services, and Support Structs 
 * 
 */ 
 
 
#if !defined(__EPIX_XCLIBSV_DEFINED) 
#define __EPIX_XCLIBSV_DEFINED 
#include "cext_hps.h"      
 
#ifdef  __cplusplus 
extern "C" { 
#endif 
 
 
/* 
 * Library version ID 
 * 
 * #define XCLIB_IDNVR	   "PIXCI(R) 32 Bit Library V2.10.01 [01.10.04]" 
 */ 
#if defined(PIXCI_CLSER) 
#define XCLIB_IDN	"PIXCI(R) CameraLink API" 
#elif defined(PIXCI_LITE) 
#define XCLIB_IDN	"PIXCI(R) 32 Bit Library Lite" 
#else 
#define XCLIB_IDN	"PIXCI(R) 32 Bit Library" 
#endif 
#define XCLIB_IDV	"2.20.00" 
#define XCLIB_IDV0	2 
#define XCLIB_IDV1	20 
#define XCLIB_IDV2	00 
#define XCLIB_IDR	"[05.04.04]" 
#define XCLIB_IDNVR	XCLIB_IDN " " XCLIB_IDV " " XCLIB_IDR 
 
/* 
 * PIXCI(R) imaging board model codes. 
 */ 
#define PIXCI_SV2	0x0001 
#define PIXCI_SV3	0x0002 
#define PIXCI_DLS	0x0003	/* never used!			    */ 
#define PIXCI_SV4	0x0004 
#define PIXCI_D 	0x0005 
#define PIXCI_D32	0x0006	/* never used! appears as PIXCI_D   */ 
#define PIXCI_A 	0x0007 
#define PIXCI_D24	0x0008	/* never used! appears as PIXCI_D   */ 
#define PIXCI_DVO	0x0009 
#define PIXCI_SV5	0x000A 
#define PIXCI_D2X	0x000B 
#define PIXCI_D2X_DVC	0x000C 
#define PIXCI_CL3SD_NAC 0x000D 
#define PIXCI_CL1	0x000E 
#define PIXCI_D3X	0x000F 
#define PIXCI_CL3SD	0x0010 
#define PIXCI_CL2	0x0011 
#define PIXCI_SI	0x0012 
#define PIXCI_SV6	0x0013 
 
/* 
 * PIXCI(R) imaging board submodel codes. 
 */ 
#define PIXCI_SV5_SV5	0x0000 
#define PIXCI_SV5_SV5A	'A' 
 
 
/* 
 * Static device info extracted from board, or from driver. 
 */ 
struct xcdevinfo { 
    struct  pxddch  ddch; 
 
    pxdevinfo_s s;		/* common info					*/ 
 
    uint16  revlevel;		/* revision level for PIXCI(R) A/D/D24/D32/D2X	*/ 
				/* D3X, CL1, CL3SD, SI				*/ 
    uint16  virq;		/* assigned virq				*/ 
    uint16  pcibus;		/* pci bus #					*/ 
    uint8   pci_config[64];	/* pci configuration space, base ..		*/ 
				/* raw byte array as struct pci_config		*/ 
				/* isn't external .h!                           */ 
    /* 
     * Added. 
     * Software assumes old & new are identical, up to size of old. 
     */ 
    uint16  fpgalevel;		/* for PIXCI(R) CL2				*/ 
    uint16  pcblevel;		/* for PIXCI(R) CL2				*/ 
    uint16  cam0code;		/* for PIXCI(R) CL2				*/ 
    uint16  cam1code;		/* for PIXCI(R) CL2				*/ 
    uint16  rsvd[16]; 
    uint32  rsvd2[16]; 
 
}; 
typedef struct xcdevinfo xcdevinfo_s; 
#define XCMOS_DEVINFO	    (PXMOS_DDCH+PXMOS_DEVINFO+3+64+4+16+16) 
/* 
 * As above, older version phased out mid V2.2. 
 */ 
struct xcdevinfo0 { 
    struct  pxddch  ddch; 
 
    pxdevinfo_s s;		/* common info					*/ 
 
    uint16  revlevel;		/* revision level (PIXCI(R) A/D/D24/D32/D2X only)*/ 
    uint16  virq;		/* assigned virq				*/ 
    uint16  pcibus;		/* pci bus #					*/ 
    uint8   pci_config[64];	/* pci configuration space, base ..		*/ 
				/* raw byte array as struct pci_config		*/ 
				/* isn't external .h!                           */ 
}; 
typedef struct xcdevinfo0 xcdevinfo0_s; 
#define XCMOS_DEVINFO0	    (PXMOS_DDCH+PXMOS_DEVINFO+3+1) 
 
 
 
/* 
 * Driver parms 
 */ 
struct xcdevparms { 
    struct  pxddch  ddch; 
 
    uint32  pollperiod;     /* -PO: poll period for psuedo interrupts, microsec */ 
    uint32  memsize;	    /* -IM: requested total size of image memory	*/ 
    uint32  memblksize;     /* -MB: requested block size of image memory	*/ 
    uint32  memholesize;    /* -MH: requested size of image memory hole 	*/ 
    uint32  memadrs;	    /* -IA: assigned physical image memory address	*/ 
    uint16  vectpassup;     /* -QP: 4GW helper IRQ #, with auto passup		*/ 
    uint16  physunitmap;    /* -DM: which XC's to use, bitmap                   */ 
    uint16  onlymodel;	    /* -MO: ignore all models other than ..		*/ 
    uint8   useirq;	    /* -QU: allow use of interrupts			*/ 
    uint8   shareirq;	    /* -QS: let irq be shared?				*/ 
    uint8   sharemem;	    /* -XM: reserve memory for units opened later	*/ 
    uint8   shareunit;	    /* -XU: allow other clients to share unit(s)	*/ 
    uint8   memmap;	    /* -MU: 0x1: map memory to user space?		*/ 
			    /* -MU: 0x2: map on demand to sys space?		*/ 
}; 
typedef struct xcdevparms	xcdevparms_s; 
#define XCMOS_DEVPARMS		(PXMOS_DDCH+13) 
 
 
/* 
 * Extended video status. 
 */ 
struct xcvidstatus2 { 
    struct  pxddch  ddch; 
 
    pxvidstatus_s   s;			/* common info		      */ 
 
    union { 
	/* 
	 * For PIXCI SV4, SV5 
	 */ 
	struct sv4 { 
	    int 	hlock;		/* 0|1: is hlock reported	    */ 
	    int 	vlock;		/* 0|1: is vlock reported	    */ 
	    int 	lumaoverflow;	/* 0|1: did luma overflow last field	*/ 
	    int 	chromaoverflow; /* 0|1: did chroma overflow last field	*/ 
	} sv4; 
	struct dxx { 
	    int 	vdrive2; 
	    int 	hdrive2; 
	} dxx; 
	int	pad[64]; 
    } u; 
}; 
typedef struct xcvidstatus2 xcvidstatus_s; 
#define XCMOS_VIDSTATUS (PXMOS_DDCH+PXMOS_VIDSTATUS+4) 
 
 
 
 
/* 
 * PIXCI(R) imaging board low level services 
 */ 
typedef struct xcdevservice xcdevservice_s; 
struct xcdevservice { 
    void    _farimap	*stuff;  /* internal stuff */ 
 
    /* 
     * Get and set driver parms, via a structure. 
     * The setParms is for "-configure" use only; parms 
     * can't be modified after boards are open. 
     */ 
    int (_cfunfcc *setDevParms) (xcdevservice_s *me,int unitmap,int rsvd1,xcdevparms_s *parms); 
    int (_cfunfcc *getDevParms) (xcdevservice_s *me,int unitmap,int rsvd1,xcdevparms_s *parms); 
 
    /* 
     * Get board model and other static device info. 
     */ 
    int (_cfunfcc *getDevInfo)	(xcdevservice_s *me,int unitmap,int rsvd1,xcdevinfo_s *info); 
 
    /* 
     * Get extended video status, with board (or 
     * at least the board's family) specific status. 
     */ 
    int (_cfunfcc *getVidStatus)(xcdevservice_s *me,int unitmap,int rsvd1,xcvidstatus_s *status,int mode); 
 
    /* 
     * Low level video control. 
     * These never wait for effect or completion, 
     * and expect familiarity with board timing characteristics, 
     * particularly hardware queuing and video synchronization. 
     * The 'time' option is not current supported. 
     * 
     * The Live and Snap services do not not automatically do 
     * setVideoConfig or setVideoAdjust. 
     * 
     * These can be used with video states that haven't 
     * been 'defined', allowing bypassing the state maintenance logic. 
     */ 
    int (_cfunfcc *setVideoConfig)  (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time); 
    int (_cfunfcc *setVideoAdjust)  (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time); 
    int (_cfunfcc *setSnapBuf)	    (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time,pxbuffer_t buf,int tracker); 
    int (_cfunfcc *setSnapPairBuf)  (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time,pxbuffer_t buf1,int tracker1,pxbuffer_t buf2,int tracker2); 
    int (_cfunfcc *setLiveBuf)	    (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time,pxbuffer_t buf,int tracker); 
    int (_cfunfcc *setLivePairBuf)  (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time,pxbuffer_t buf1,int tracker1,pxbuffer_t buf2,int tracker2,int period); 
    int (_cfunfcc *setLiveSeqBuf)   (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time,pxbuffer_t startbuf,pxbuffer_t endbuf,pxbuffer_t incbuf,pxbuffer_t numbuf,int period,int trackers); 
    int (_cfunfcc *setLiveTrigBuf)  (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time,pxbuffer_t buf,int tracker, pxtrigspec_s *trigspec); 
    int (_cfunfcc *setUnLive)	    (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time); 
    int (_cfunfcc *setAbortLive)    (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time); 
    int (_cfunfcc *getLiveStatus)   (xcdevservice_s *me,int unitmap,int rsvd1,int mode); 
    int (_cfunfcc *setLivePhys)     (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time,pxvidphys_s phys[]); 
 
    /* 
     * Camera control 
     */ 
    int (_cfunfcc *setCameraConfig) (xcdevservice_s *me,int unitmap,int rsvd1,int stateid,pxvidstate_s *statep,pxtimespec_s *time); 
 
    /* 
     * Low level G.P.I.O 
     * The 'time' option is not current supported. 
     */ 
     int (_cfunfcc *getGpout)	   (xcdevservice_s *me,int unitmap,pxtimespec_s *time,int rsvd1,int rsvd2); 
     int (_cfunfcc *setGpout)	   (xcdevservice_s *me,int unitmap,pxtimespec_s *time,int rsvd1,int value); 
     int (_cfunfcc *getGpin)	   (xcdevservice_s *me,int unitmap,pxtimespec_s *time,int rsvd1,int rsvd2); 
     int (_cfunfcc *setGpin)	   (xcdevservice_s *me,int unitmap,pxtimespec_s *time,int rsvd1,int value); 
 
    /* 
     * Internal use only! 
     */ 
     int (_cfunfcc *getDope)	   (xcdevservice_s *me,int unitmap,int rsvd1,void *dopep,int cnt); 
 
    /* 
     * Future expansion 
    int (_cfunfcc *rsvd1)	 (pxdevservice_s *me); 
    int (_cfunfcc *rsvd2)	 (pxdevservice_s *me); 
    int (_cfunfcc *rsvd3)	 (pxdevservice_s *me); 
    int (_cfunfcc *rsvd4)	 (pxdevservice_s *me); 
     */ 
 
}; 
#define XCMOS_DEVSERVICE    (1+2+1+1+12+1+4+1) 
 
/* 
 * Modes for getLiveStatus 
 */ 
#define PXVIST_STATEID	    0x0001 
#define PXVIST_BUFFER	    0x0002 
#define PXVIST_TRACKER	    0x0003 
#define PXVIST_VIDMODE	    0x0004 
#define PXVIST_SYSTICKS0    0x0005 
#define PXVIST_SYSTICKS1    0x0006 
#define PXVIST_VCNT	    0x0007 
#define PXVIST_LIVEMODE     0x0008 
#define PXVIST_BYTES	    0x0009 
#define PXVIST_COUNT	    0x000A	/* internal use only */ 
#define PXVIST_RUNNING	    0x0000 
#define PXVIST_QUEUED	    0x0100 
#define PXVIST_DONE	    0x0010 
#define PXVIST_ORUNITS	    0x1000 
 
 
/* 
 * PIXCI(R) imaging board library level services 
 */ 
typedef struct xclibservice xclibservice_s; 
struct xclibservice { 
    void    _farimap	*stuff;  /* internal stuff */ 
 
    /* 
     * Normalize the video state to be internally consistent, 
     * and consistent with the current board. 
     * Fill the pxvidphys from the remainder of the video state. 
     * Sign the pxvidphys for esoteric applications. 
     * No direct effect on video. 
     */ 
    int (_cfunfcc *fixxStateCopy)  (xclibservice_s *me,int options,int stateid,pxvidstate_s *statep);	  // stateid ignored! 
    int (_cfunfcc *fillStateCopy)  (xclibservice_s *me,int options,int stateid,pxvidstate_s *statep,pxbuffer_t buf);	 // stateid ignored! 
    int (_cfunfcc *signStateCopy)  (xclibservice_s *me,int options,int stateid,pxvidstate_s *statep, int space, int mode); 
 
    /* 
     * Future expansion 
    int (_cfunfcc *rsvd1)	 (pxdevservice_s *me); 
    int (_cfunfcc *rsvd2)	 (pxdevservice_s *me); 
    int (_cfunfcc *rsvd3)	 (pxdevservice_s *me); 
    int (_cfunfcc *rsvd4)	 (pxdevservice_s *me); 
     */ 
}; 
#define XCMOS_LIBSERVICE    (1+3) 
 
 
/* 
 * All services. 
 * Segregating the various services is intended to help, 
 *  (a) Discourage use of low level services, 
 *  (b) Encourage use of board independent services. 
 * Or at least make it clear what types of service are used. 
 * Note that even the so-called "low level" services are far 
 * above a trivial "register poking" API. 
 */ 
typedef struct xclibs xclibs_s; 
struct xclibs { 
 
    struct  pxddch	ddch; 
    void    _farimap	*stuff;     /* internal stuff	*/ 
 
    pxdevservice_s  pxdev;	/* low level, board independent, services   */ 
    xcdevservice_s  xcdev;	/* low level, board dependent, services     */ 
    pxlibservice_s  pxlib;	/* board independent services		    */ 
    xclibservice_s  xclib;	/* board dependent services.		    */ 
    pxauxservice_s  pxaux;	/* auxilliary services			    */ 
}; 
#define XCMOS_LIBS	(PXMOS_DDCH+1+PXMOS_DEVSERVICE+XCMOS_DEVSERVICE+PXMOS_LIBSERVICE+XCMOS_LIBSERVICE+PXMOS_AUXSERVICE) 
 
/* 
 * Export API: 
 * 
 * Open or shared open, filling the struct xclibs* 
 * with service access functions. 
 */ 
#if defined(PIXCI_LITE) 
_cDcl(_dllxxxxx,_cfunfcc,int)	xclib_open (struct xclibs *xclib,char *modes,char *driverparms,char *formatname,char *formatfile); 
_cDcl(_dllxxxxx,_cfunfcc,int)	xclib_close(struct xclibs *xclib); 
#else 
_cDcl(_dllpxlib,_cfunfcc,int)	xclib_open (struct xclibs *xclib,char *modes,char *driverparms,char *formatname,char *formatfile); 
_cDcl(_dllpxlib,_cfunfcc,int)	xclib_close(struct xclibs *xclib); 
#endif 
 
 
/* 
 * Access to identification strings without first xclib_open'ing. 
 * These provide the library's compiled id vis-a-vis 
 * the .h files id - only the info->library is set! 
 */ 
#if defined(PIXCI_LITE) 
_cDcl(_dllxxxxx,_cfunfcc,int) xclib_liblibid(pxdevinfo_s *info);  /* .dll or .obj files */ 
#else 
_cDcl(_dllpxlib,_cfunfcc,int) xclib_liblibid(pxdevinfo_s *info);  /* .dll or .obj files */ 
#endif 
#define xclib_libincid(info) ((info?strncpy((info)->libraryid,XCLIB_IDNVR,sizeof((info)->libraryid)):0),((info)?0:PXERROR)) 
								  /* .h files */ 
/* 
 * Internal use only! 
 */ 
_cDcl(_dllpxlib,_cfunfcc,int) pxlib_pxipl(void*,void*,void*,void*,void*,void*,void _farimap*); 
_cDcl(_dllpxobj _dllpxlib,_cfunfcc,int) pximagesizing250800(struct pximage*,int,int,int,pxcoord_t*,pxcoord_t*);  /* deprecated version */ 
_cDcl(_dllpxobj _dllpxlib,_cfunfcc,int) pximage3sizing250800(struct pximage3*,int,int,int,pxcoord_t*,pxcoord_t*); /* deprecated version */ 
 
 
 
 
/* 
 * Helpful macros for declaring and initializing 
 * a vidstate_s and all component structures. 
 */ 
#define xclib_DeclareVidStateStructs(Name)	    \
	    struct pxvidstate	 Name;		    \
	    struct pxvidformat	 Name_##vidformat;  \
	    struct pxvidres	 Name_##vidres;     \
	    struct pxvidmode	 Name_##vidmode;    \
	    struct pxvidphys	 Name_##vidphys;    \
	    struct pxvidimage	 Name_##vidimage;   \
	    struct pxvidopt	 Name_##vidopt;     \
	    struct pxvidmem	 Name_##vidmem;     \
	    struct pxcamcntl	 Name_##camcntl;    \
	    struct xcsv2format	 Name_##sv2format;  \
	    struct xcsv2mode	 Name_##sv2mode;    \
	    struct xcdxxformat	 Name_##dxxformat; 
 
#define xclib_InitVidStateStructs(Name) 	       \
	    Name.vidformat    = &Name_##vidformat;     \
	    Name.vidres       = &Name_##vidres;        \
	    Name.vidmode      = &Name_##vidmode;       \
	    Name.vidphys      = &Name_##vidphys;       \
	    Name.vidimage     = &Name_##vidimage;      \
	    Name.vidopt       = &Name_##vidopt;        \
	    Name.vidmem       = &Name_##vidmem;        \
	    Name.camcntl      = &Name_##camcntl;       \
	    Name.xc.sv2format = &Name_##sv2format;     \
	    Name.xc.sv2mode   = &Name_##sv2mode;       \
	    Name.xc.dxxformat = &Name_##dxxformat;     \
	    Name	    .ddch.len = sizeof(Name);		   Name.ddch.mos	     = PXMOS_VIDSTATE;	\
	    Name_##vidformat.ddch.len = sizeof(Name_##vidformat);  Name_##vidformat.ddch.mos = PXMOS_VIDFORMAT; \
	    Name_##vidres   .ddch.len = sizeof(Name_##vidres);	   Name_##vidres.ddch.mos    = PXMOS_VIDRES;	\
	    Name_##vidmode  .ddch.len = sizeof(Name_##vidmode);    Name_##vidmode.ddch.mos   = PXMOS_VIDMODE;	\
	    Name_##vidphys  .ddch.len = sizeof(Name_##vidphys);    Name_##vidphys.ddch.mos   = PXMOS_VIDPHYS;	\
	    Name_##vidimage .ddch.len = sizeof(Name_##vidimage);   Name_##vidimage.ddch.mos  = PXMOS_VIDIMAGE;	\
	    Name_##vidopt   .ddch.len = sizeof(Name_##vidopt);	   Name_##vidopt.ddch.mos    = PXMOS_VIDOPT;	\
	    Name_##vidmem   .ddch.len = sizeof(Name_##vidmem);	   Name_##vidmem.ddch.mos    = PXMOS_VIDMEM;	\
	    Name_##camcntl  .ddch.len = sizeof(Name_##camcntl);    Name_##camcntl.ddch.mos   = PXMOS_CAMCNTL;	\
	    Name_##sv2format.ddch.len = sizeof(Name_##sv2format);  Name_##sv2format.ddch.mos = XCMOS_SV2FORMAT; \
	    Name_##dxxformat.ddch.len = sizeof(Name_##dxxformat);  Name_##dxxformat.ddch.mos = XCMOS_DXXFORMAT; \
	    Name_##sv2mode  .ddch.len = sizeof(Name_##sv2mode);    Name_##sv2mode.ddch.mos   = XCMOS_SV2MODE; 
 
 
#ifdef  __cplusplus 
} 
#endif 
 
#include "cext_hpe.h"      
#endif				/* !defined(__EPIX_XCLIBSV_DEFINED) */ 