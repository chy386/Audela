/****************************************
 * ArtemisHSCAPI.h
 *
 * This file is autogenerated.
 *
 ****************************************/

#include <comdef.h>

//////////////////////////////////////////////////////////////////////////
//
// Interface functions for Artemis CCD Camera Library
//


//Error codes

enum ARTEMISERROR
{
	ARTEMIS_OK = 0,
	ARTEMIS_INVALID_PARAMETER,
	ARTEMIS_NOT_CONNECTED,
	ARTEMIS_NOT_IMPLEMENTED,
	ARTEMIS_NO_RESPONSE,
	ARTEMIS_INVALID_FUNCTION,
};

// Colour properties
enum ARTEMISCOLOURTYPE
{
	ARTEMIS_COLOUR_UNKNOWN = 0,
	ARTEMIS_COLOUR_NONE,
	ARTEMIS_COLOUR_RGGB
};

//Other enumeration types
enum ARTEMISPRECHARGEMODE
{
	PRECHARGE_NONE = 0,		// Precharge ignored
	PRECHARGE_ICPS,			// In-camera precharge subtraction
	PRECHARGE_FULL,			// Precharge sent with image data
};

// Camera State
enum ARTEMISCAMERASTATE
{
	CAMERA_ERROR = -1,
	CAMERA_IDLE = 0,
	CAMERA_WAITING,
	CAMERA_EXPOSING,
	CAMERA_READING,
	CAMERA_DOWNLOADING,
	CAMERA_FLUSHING,
};

// Parameters for ArtemisSendMessage
enum ARTEMISSENDMSG
{
	ARTEMIS_LE_LOW				=0,
	ARTEMIS_LE_HIGH				=1,
	ARTEMIS_GUIDE_NORTH			=10,
	ARTEMIS_GUIDE_SOUTH			=11,
	ARTEMIS_GUIDE_EAST			=12,
	ARTEMIS_GUIDE_WEST			=13,
	ARTEMIS_GUIDE_STOP			=14,
};

// Parameters for ArtemisGet/SetProcessing
// These must be powers of 2.
enum ARTEMISPROCESSING
{
	ARTEMIS_PROCESS_LINEARISE	=1,	// compensate for JFET nonlinearity
	ARTEMIS_PROCESS_VBE			=2, // adjust for 'Venetian Blind effect'
};

// Parameters for ArtemisSetUpADC
enum ARTEMISSETUPADC
{
	ARTEMIS_SETUPADC_MODE		=0,
	ARTEMIS_SETUPADC_OFFSETR	=(1<<10),
	ARTEMIS_SETUPADC_OFFSETG	=(2<<10),
	ARTEMIS_SETUPADC_OFFSETB	=(3<<10),
	ARTEMIS_SETUPADC_GAINR		=(4<<10),
	ARTEMIS_SETUPADC_GAING		=(5<<10),
	ARTEMIS_SETUPADC_GAINB		=(6<<10),
};

enum ARTEMISPROPERTIESCCDFLAGS
{
	ARTEMIS_PROPERTIES_CCDFLAGS_INTERLACED =1, // CCD is interlaced type
	ARTEMIS_PROPERTIES_CCDFLAGS_DUMMY=0x7FFFFFFF // force size to 4 bytes
};
enum ARTEMISPROPERTIESCAMERAFLAGS
{
	ARTEMIS_PROPERTIES_CAMERAFLAGS_FIFO =1, // Camera has readout FIFO fitted
	ARTEMIS_PROPERTIES_CAMERAFLAGS_EXT_TRIGGER =2, // Camera has external trigger capabilities
	ARTEMIS_PROPERTIES_CAMERAFLAGS_PREVIEW =4, // Camera can return preview data
	ARTEMIS_PROPERTIES_CAMERAFLAGS_SUBSAMPLE =8, // Camera can return subsampled data
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_SHUTTER =16, // Camera has a mechanical shutter
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_GUIDE_PORT =32, // Camera has a guide port
	ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_GPIO =64, // Camera has GPIO capability
	ARTEMIS_PROPERTIES_CAMERAFLAGS_DUMMY=0x7FFFFFFF // force size to 4 bytes
};

//Structures

// camera/CCD properties
struct ARTEMISPROPERTIES
{
	int Protocol;
	int nPixelsX;
	int nPixelsY;
	float PixelMicronsX;
	float PixelMicronsY;
	int ccdflags;
	int cameraflags;
	char Description[40];
	char Manufacturer[40];
};

typedef void* ArtemisHandle;

#ifdef ARTEMISHSCAPI_CPP

#define artfn /* */
#define NFUNCS 100

class CArtemisHSCAPI
{
public:
	CArtemisHSCAPI() {hArtemisDLL=NULL;}
	~CArtemisHSCAPI() {ArtemisUnLoadDLL();}
	FARPROC pFuncs[NFUNCS];
	HINSTANCE hArtemisDLL;
#else
#define artfn extern
#endif


// interface functions

// Return API version. XYY X=major, YY=minor
artfn  int ArtemisAPIVersion();

// Get USB Identifier of Nth USB device. Return false if no such device.
// pName must be at least 40 chars long.
artfn  bool ArtemisDeviceName(int Device, char *pName);

// Get USB Serial number of Nth USB device. Return false if no such device.
// pName must be at least 40 chars long.
artfn  bool ArtemisDeviceSerial(int Device, char *pName);

// Return true if Nth USB device exists and is a camera.
artfn  bool ArtemisDeviceIsCamera(int Device);

// Return camera type and serial number
// Low byte of flags is camera type, 1=4021, 2=11002, 3=IC24/285, 4=205, 5=QC
// Bits 8-31 of flags are reserved.
artfn  int ArtemisCameraSerial(ArtemisHandle hCam, int* flags, int* serial);

// Disconnect from given device.
// Returns true if disconnected as requested
artfn  bool ArtemisDisconnect(ArtemisHandle hCam);

// Connect to given device. If Device=-1, connect to first available
// Returns handle if connected as requested, else NULL
artfn  ArtemisHandle ArtemisConnect(int Device);

// Disconnect all connected devices
artfn  bool ArtemisDisconnectAll();

// Returns TRUE if currently connected to a device
artfn  bool ArtemisIsConnected(ArtemisHandle hCam);

// Fills in pProp with camera properties
artfn  int ArtemisProperties(ArtemisHandle hCam, struct ARTEMISPROPERTIES *pProp);

// Displays the Artemis setup dialog, if any
artfn  int ArtemisSetupDialog();

// Abort exposure, if one is in progress
artfn  int ArtemisAbortExposure(ArtemisHandle hCam);

// Set the start x,y coords for imaging subframe.
// X,Y in unbinned coordinates
artfn  int ArtemisSubframePos(ArtemisHandle hCam, int x, int y);

// Set the width and height of imaging subframe
// W,H in unbinned coordinates
artfn  int ArtemisSubframeSize(ArtemisHandle hCam, int w, int h);

// set the pos and size of imaging subframe inunbinned coords
artfn  int ArtemisSubframe(ArtemisHandle hCam, int x, int y, int w, int h);

// Get the pos and size of imaging subframe
artfn  int ArtemisGetSubframe(ArtemisHandle hCam, int *x, int *y, int *w, int *h);

// Set the x,y binning factors
artfn  int ArtemisBin(ArtemisHandle hCam, int x, int y);

// Get the x,y binning factors
artfn  int ArtemisGetBin(ArtemisHandle hCam, int *x, int *y);

// Get the maximum x,y binning factors
artfn  int ArtemisGetMaxBin(ArtemisHandle hCam, int *x, int *y);

// Set the Precharge mode
artfn  int ArtemisPrechargeMode(ArtemisHandle hCam, int mode);

// Clear the VRegs
artfn  int ArtemisClearVRegs(ArtemisHandle hCam);

// Set the FIFO usage flag
artfn  int ArtemisFIFO(ArtemisHandle hCam, bool bEnable);

// Start an exposure
artfn  int ArtemisStartExposure(ArtemisHandle hCam, float Seconds);

// Prematurely end an exposure, collecting image data.
artfn  int ArtemisStopExposure(ArtemisHandle hCam);

// Return true if camera can overlap exposure time with image download time
artfn  bool ArtemisCanOverlapExposures(ArtemisHandle hCam);

// Return true if dark mode is set - ie the shutter is kept closed during exposures
artfn  bool ArtemisGetDarkMode(ArtemisHandle hCam);

// Enable/disable dark mode - ie the shutter is to be kept closed during exposures
artfn  int	ArtemisSetDarkMode(ArtemisHandle hCam, bool bEnable);

// Allow/disallow automatic black level adjustment (only applies to quickercams)
artfn  int ArtemisAutoAdjustBlackLevel(ArtemisHandle hCam, bool bEnable);

// Enable/disable termination of guiding before downloading the image
artfn  int ArtemisStopGuidingBeforeDownload(ArtemisHandle hCam, bool bEnable);

// Get an internal DLL value specified by peekCode
artfn  int ArtemisPeek(ArtemisHandle hCam, int peekCode, int* peekValue);

// Set an internal DLL value specified by pokeCode
artfn  int ArtemisPoke(ArtemisHandle hCam, int pokeCode, int pokeValue);

// Get the number of GPIO lines and the value of the input on each line
// (value of input on nth line given by value of nth bit in lineValues)
artfn  int ArtemisGetGpioInformation(ArtemisHandle hCam, int* lineCount, int* lineValues);

// Set the GPIO line directions
// (nth line is set as an input (output) if nth bit of directionMask is 1 (0)
artfn  int	ArtemisSetGpioDirection(ArtemisHandle hCam, int directionMask);

//Set GPIO output line values
// (nth line (if it's an output) is set to high (low) if nth bit of lineValues is 1 (0)
artfn  int ArtemisSetGpioValues(ArtemisHandle hCam, int lineValues);

// Return colour properties
artfn  int ArtemisColourProperties(ArtemisHandle hCam, ARTEMISCOLOURTYPE *colourType, int *normalOffsetX, int *normalOffsetY, int *previewOffsetX, int *previewOffsetY);

// Set duration for overlapped exposures. Call once, not every frame.
artfn  int	ArtemisSetOverlappedExposureTime(ArtemisHandle hCam, float Seconds);

// Request an overlapped exposure to be downloaded when ready
artfn  int	ArtemisStartOverlappedExposure(ArtemisHandle hCam);

// Return true if the previous overlapped exposure had the requested exposure time.
artfn  bool ArtemisOverlappedExposureValid(ArtemisHandle hCam);

// Set a window message to be posted on completion of image download
// hWnd=NULL for no message.
artfn  int ArtemisExposureReadyCallback(ArtemisHandle hCam, HWND hWnd, int msg, int wParam, int lParam);

// Retrieve the downloaded image as a 2D array of type VARIANT
artfn  int ArtemisGetImageArray(ArtemisHandle hCam, VARIANT *pImageArray);

// Retrieve image dimensions and binning factors.
// x,y are actual CCD locations. w,h are pixel dimensions of image
artfn  int ArtemisGetImageData(ArtemisHandle hCam, int *x, int *y, int *w, int *h, int *binx, int *biny);

// Upload a compressed object file to the Artemis PIC
artfn  int ArtemisReflash(ArtemisHandle hCam ,char *objfile);

// Return true if amp switched off during exposures
artfn  bool ArtemisGetAmplifierSwitched(ArtemisHandle hCam);

// Set whether amp is switched off during exposures
artfn  int ArtemisSetAmplifierSwitched(ArtemisHandle hCam, bool bSwitched);

// Return duration of last exposure, in seconds
artfn  float ArtemisLastExposureDuration(ArtemisHandle hCam);

// Return time remaining in current exposure, in seconds
artfn  float ArtemisExposureTimeRemaining(ArtemisHandle hCam);

// Return ptr to static buffer containing time of start of last exposure
artfn  char* ArtemisLastStartTime(ArtemisHandle hCam);

// Return fraction-of-a-second part of time of start of last exposure
// NB timing accuracy only justifies ~0.1s precision but milliseconds returned in case it might be useful
artfn  int ArtemisLastStartTimeMilliseconds(ArtemisHandle hCam);

// Return true if an image is ready to be retrieved
artfn  bool ArtemisImageReady(ArtemisHandle hCam);

// Switch off all guide relays
artfn  int ArtemisStopGuiding(ArtemisHandle hCam);

// Activate a guide relay for a short interval, axis=0,1,2,3 for N,S,E,W
artfn  int ArtemisPulseGuide(ArtemisHandle hCam, int axis, int milli);

// Activate a guide relay, axis=0,1,2,3 for N,S,E,W
artfn  int ArtemisGuide(ArtemisHandle hCam, int axis);

// Set guide port bits (bit 1 = N, bit 2 = S, bit 3 = E, bit 4 = W)
artfn  int ArtemisGuidePort(ArtemisHandle hCam, int nibble);

// Set download thread to high or normal priority
artfn  int ArtemisHighPriority(ArtemisHandle hCam, bool bHigh);

// Retrieve the current camera state
artfn  int ArtemisCameraState(ArtemisHandle hCam);

// Percentage downloaded
artfn  int ArtemisDownloadPercent(ArtemisHandle hCam);

// Return pointer to internal image buffer (actually unsigned shorts)
artfn  void* ArtemisImageBuffer(ArtemisHandle hCam);

// Set the CCD amplifier on or off
artfn  int ArtemisAmplifier(ArtemisHandle hCam, bool bOn);

// Set the webcam Long Exposure control
artfn  int ArtemisWebcamLE(ArtemisHandle hCam, bool bHigh);

// Reset the camera PIC
artfn  int ArtemisReset(ArtemisHandle hCam);

// Get current image processing options
artfn  int ArtemisGetProcessing(ArtemisHandle hCam);

// Set current image processing options
artfn  int ArtemisSetProcessing(ArtemisHandle hCam, int options);

// Set External Trigger mode (if supported by camera). True=wait for trigger.
artfn  int ArtemisTriggeredExposure(ArtemisHandle hCam, bool bAwaitTrigger);

// Set preview mode (if supported by camera). True=preview mode enabled.
artfn  int ArtemisSetPreview(ArtemisHandle hCam, bool bPrev);

// Set subsampling mode (if supported by camera). True=subsampling enabled.
artfn  int ArtemisSetSubSample(ArtemisHandle hCam, bool bSub);

// Send a packet of data to a peripheral, and receive a reply.
// pSendData points to an 8 byte array which is sent to the peripheral.
// pRecvData points to an 8 byte array which is filled with the peripheral's response.
// Returns ARTEMIS_OK if the peripheral responds, in which case pRecvData contains its reply.
// Returns ARTEMIS_NO_RESPONSE if the peripheral doesn't respond.
// If the peripheral does not respond, pRecvData is not guaranteed to be preserved.
// pSendData and pRecvData may be the same but must not be NULL.
artfn  int ArtemisPeripheral(ArtemisHandle hCam, int PeripheralID, unsigned char *pSendData, unsigned char *pRecvData);

// Set ADC parameters.
// param should be one of ARTEMISSETUPADC plus the data value
artfn  int ArtemisSetUpADC(ArtemisHandle hCam, int param);

// Set conversion speed.
artfn  int ArtemisSetConversionSpeed(ArtemisHandle hCam, int speed);

// Set conversion speed.
artfn  int ArtemisSetOversample(ArtemisHandle hCam, int oversample);

artfn  int ArtemisTemperatureSensorInfo(ArtemisHandle hCam, int sensor, int* temperature);

artfn  int ArtemisCoolingInfo(ArtemisHandle hCam, int* flags, int* level, int* minlvl, int* maxlvl, int* setpoint);

artfn  int ArtemisSetCooling(ArtemisHandle hCam, int setpoint);

artfn  int ArtemisCoolerWarmUp(ArtemisHandle hCam);

artfn  int ArtemisReconnectUSB(ArtemisHandle hCam);

/////////////////////////////////////////////////
// Diagnostic Functions

// Ping the camera, return the result. -1 on error.
artfn  int ArtemisDiagnosticPing(ArtemisHandle hCam, int send);

// Ping the FIFO, return the result. -1 on error.
artfn  int ArtemisDiagnosticPingFIFO(ArtemisHandle hCam, int send);

// Set the CCD clocks running (firmware doesn't return!)
artfn  int ArtemisDiagnosticRunCCD(ArtemisHandle hCam);

// Measure the precharge level
artfn  int ArtemisDiagnosticPrecharge(ArtemisHandle hCam);

// Connects to kernel only, safe to use before firmware
// has been uploaded.
// Returns handle if connected as requested
artfn  ArtemisHandle ArtemisDiagnosticConnect(int Device);

// Miscellaneous commands to set voltages etc.
// Not to be called if the CCD is installed!
artfn  int ArtemisDiagnosticCommand(ArtemisHandle hCam, int cmd);

// Return the last FT USB error condition seen
// Calling this function clears the internal error state
artfn  int ArtemisDiagnosticUSBError(ArtemisHandle hCam);

/////////////////////////////////////////////////
// Access LE/Guide ports from 3rd-party software
// msg is the command to execute
// unit is the camera number
// returns:
//  0  OK
//  1  camera busy
//  2  no camera active
//  3  invalid command
artfn  int ArtemisSendMessage(int msg, int unit);

/////////////////////////////////////////////////
// Access peripherals from 3rd-party software
// peripheral is the peripheral's device ID
// send and recv are 8-byte buffers for message-passing
// unit is the camera number
// returns:
//  0  OK
//  1  camera busy
//  2  no camera active
//  3  invalid command
//  4  no response fropm peripheral 
artfn  int ArtemisSendPeripheralMessage(int peripheral, char *send, char *recv, int unit);

/////////////////////////////////////////////////
// Get camera description, for 3rd-party software
// recv is a 40-byte buffer for the data
// info tells which data to return:
//  0  camera description from firmware
//  1  FTDI device name
//  2  FTDI device serial number
// unit is the camera number
// returns:
//  0  OK
//  1  camera busy
//  2  no camera active
artfn  int ArtemisGetDescription(char *recv, int info, int unit);

// Try to load the Artemis DLL.
// Returns true if loaded ok.
artfn bool ArtemisLoadDLL(char *FileName);

// Unload the Artemis DLL.
artfn void ArtemisUnLoadDLL();
#ifdef ARTEMISHSCAPI_CPP
}; // class CArtemisAPI
#endif
#undef artfn

