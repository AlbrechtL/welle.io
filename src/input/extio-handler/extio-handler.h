#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __EXTIO_HANDLER
#define	__EXTIO_HANDLER
#
#include	<QWidget>
#include	<QComboBox>
#include	<QString>
#include	<virtual-input.h>
#include	"ringbuffer.h"

class		QSettings;
class		virtualReader;
//	create type defs for the functions
#ifdef	__MINGW32__
#define	STDCALL	__stdcall
#else
#define	STDCALL 
#endif

typedef	int	(*pfnExtIOCallback) (int cnt, int status, float IQoffs, void *IQdata);
typedef bool (STDCALL *pfnInitHW)(char *, char *, int& ); // 
typedef bool (STDCALL *pfnOpenHW)(void);
typedef void (STDCALL *pfnCloseHW)(void);
typedef int  (STDCALL *pfnStartHW)(long);
typedef void (STDCALL *pfnStopHW)(void);
typedef int  (STDCALL *pfnSetHWLO)(long);
typedef	void (STDCALL *pfnSetCallback)(pfnExtIOCallback funcptr);
typedef int  (STDCALL *pfnGetStatus)(void);
//
typedef long (STDCALL *pfnGetHWLO)(void);
typedef	long (STDCALL *pfnGetHWSR)(void);
typedef void (STDCALL *pfnRawDataReady)(long, void *, void *, int);
typedef void (STDCALL *pfnShowGUI)(void);
typedef void (STDCALL *pfnHideGUI)(void);
typedef long (STDCALL *pfnGetTune)(void);
typedef	uint8_t (STDCALL *pfnGetMode)(void);
typedef void (STDCALL *pfnModeChanged)(char);
typedef void (STDCALL *pfnTuneChanged)(long freq);
typedef void (STDCALL *pfnIfLimitsChanged)(long low, long high);
typedef void (STDCALL *pfnFiltersChanged)(int, int, int, bool);
typedef void (STDCALL *pfnMuteChanged)(bool);
typedef void (STDCALL *pfnGetFilters)(int&, int&, int&);

//	hwtype codes to be set with pfnInitHW
//	Please ask Alberto di Bene (i2phd@weaksignals.com)
//	for the assignment of an index code
//	for cases different from the above.
//	note: "exthwUSBdataNN" don't need to be from USB.
//	The keyword "USB" is just for historical reasons,
//	which may get removed later ..
typedef enum {
    exthwNone       = 0
  , exthwSDR14      = 1
  , exthwSDRX       = 2
  , exthwUSBdata16  = 3 // the hardware does its own digitization and the audio data are returned to Winrad
                        // via the callback device. Data must be in 16-bit  (short) format, little endian.
  , exthwSCdata     = 4 // The audio data are returned via the (S)ound (C)ard managed by Winrad. The external
                        // hardware just controls the LO, and possibly a preselector, under DLL control.
  , exthwUSBdata24  = 5 // the hardware does its own digitization and the audio data are returned to Winrad
                        // via the callback device. Data are in 24-bit  integer format, little endian.
  , exthwUSBdata32  = 6 // the hardware does its own digitization and the audio data are returned to Winrad
                        // via the callback device. Data are in 32-bit  integer format, little endian.
  , exthwUSBfloat32 = 7 // the hardware does its own digitization and the audio data are returned to Winrad
                        // via the callback device. Data are in 32-bit  float format, little endian.
  , exthwHPSDR      = 8 // for HPSDR only!
} extHWtypeT;

// status codes for pfnExtIOCallback; used when cnt < 0
typedef enum {
//	only processed/understood for SDR14
    extHw_Disconnected        = 0     // SDR-14/IQ not connected or powered off
  , extHw_READY               = 1     // IDLE / Ready
  , extHw_RUNNING             = 2     // RUNNING  => not disconnected
  , extHw_ERROR               = 3     // ??
  , extHw_OVERLOAD            = 4     // OVERLOAD => not disconnected

  // for all extIO's
  , extHw_Changed_SampleRate  = 100   // sampling speed has changed in the external HW
  , extHw_Changed_LO          = 101   // LO frequency has changed in the external HW
  , extHw_Lock_LO             = 102
  , extHw_Unlock_LO           = 103
  , extHw_Changed_LO_Not_TUNE = 104   // CURRENTLY NOT YET IMPLEMENTED
                                      // LO freq. has changed, Winrad must keep the Tune freq. unchanged
                                      // (must immediately call GetHWLO() )
  , extHw_Changed_TUNE        = 105   // a change of the Tune freq. is being requested.
                                      // Winrad must call GetTune() to know which value is wanted
  , extHw_Changed_MODE        = 106   // a change of demod. mode is being requested.
                                      // Winrad must call GetMode() to know the new mode
  , extHw_Start               = 107   // The DLL wants Winrad to Start
  , extHw_Stop                = 108   // The DLL wants Winrad to Stop
  , extHw_Changed_FILTER      = 109   // a change in the band limits is being requested
                                      // Winrad must call GetFilters()

//	Above status codes are processed with Winrad 1.32.
//	All Winrad derivation like WRplus, WinradF, WinradHD and
//	HDSDR should understand them,
//	but these do not provide version info with
//	VersionInfo(progname, ver_major, ver_minor).

  , extHw_Mercury_DAC_ON      = 110   // enable audio output on the Mercury DAC when using the HPSDR
  , extHw_Mercury_DAC_OFF     = 111   // disable audio output on the Mercury DAC when using the HPSDR
  , extHw_PC_Audio_ON         = 112   // enable audio output on the PC sound card when using the HPSDR
  , extHw_PC_Audio_OFF        = 113   // disable audio output on the PC sound card when using the HPSDR

  , extHw_Audio_MUTE_ON       = 114   // the DLL is asking Winrad to mute the audio output
  , extHw_Audio_MUTE_OFF      = 115   // the DLL is asking Winrad to unmute the audio output
//	Above status codes are processed with Winrad 1.33 and HDSDR
//	Winrad 1.33 and HDSDR still do not provide their
//	version with VersionInfo()
//	Following status codes are processed when VersionInfo delivers
//	0 == strcmp(progname, "HDSDR") &&
//		( ver_major > 2 || ( ver_major == 2 && ver_minor >= 13 ) )
//
//	all extHw_XX_SwapIQ_YYY callbacks shall be
//	reported after each OpenHW() call
  , extHw_RX_SwapIQ_ON        = 116   // additionaly swap IQ - this does not modify the menu point / user selection
  , extHw_RX_SwapIQ_OFF       = 117   //   the user selected swapIQ is additionally applied
  , extHw_TX_SwapIQ_ON        = 118   // additionaly swap IQ - this does not modify the menu point / user selection
  , extHw_TX_SwapIQ_OFF       = 119   //   the user selected swapIQ is additionally applied

//	Following status codes (for I/Q transceivers)
//	are processed when VersionInfo delivers
//	0 == strcmp(progname, "HDSDR") &&
//	          ( ver_major > 2 || ( ver_major == 2 && ver_minor >= 13 ) )

  , extHw_TX_Request          = 120   // DLL requests TX mode / User pressed PTT
                                      //  exciter/transmitter must wait until SetModeRxTx() is called!
  , extHw_RX_Request          = 121   // DLL wants to leave TX mode / User released PTT
                                      //   exciter/transmitter must wait until SetModeRxTx() is called!
  , extHw_CW_Pressed          = 122   // User pressed  CW key
  , extHw_CW_Released         = 123   // User released CW key
  , extHw_PTT_as_CWkey        = 124   // handle extHw_TX_Request as extHw_CW_Pressed in CW mode
                                      //  and   extHw_RX_Request as extHw_CW_Released
  , extHw_Changed_ATT         = 125   // Attenuator changed => call GetActualAttIdx()

//	Following status codes are processed when VersionInfo delivers
//	0 == strcmp(progname, "HDSDR") &&
//	             ( ver_major > 2 || ( ver_major == 2 && ver_minor >= 14 ) )

  , extHw_Changed_ATTENS      = 136   // refresh selectable attenuators => starts calling GetAttenuators()
} extHWstatusT;

// codes for pfnSetModeRxTx:
typedef enum {
    extHw_modeRX  = 0
  , extHw_modeTX  = 1
} extHw_ModeRxTxT;

class	extioHandler:public virtualInput {
Q_OBJECT
public:
			extioHandler		(QSettings *, bool *);
			~extioHandler		(void);
	int32_t		getRate			(void);
	void		setVFOFrequency		(int32_t);
	int32_t		getVFOFrequency		(void);
	bool		legalFrequency		(int32_t);
	int32_t		defaultFrequency	(void);

	bool		restartReader		(void);
	void		stopReader		(void);
	int32_t		Samples			(void);
	int32_t		getSamples		(DSPCOMPLEX *, int32_t);
	int16_t		bitDepth		(void);
	long		GetHWLO		(void);	// should be available
	long		GetHWSR		(void); // may be a noop

//
//	The call back need access to
	virtualReader	*theReader;
	bool		isStarted;
private:
	int32_t		base_16;
	int32_t		base_24;
	int32_t		base_32;
	int32_t		inputRate;
	bool		loadFunctions	(void);
	RingBuffer<DSPCOMPLEX>	*theBuffer;
//	functions to be extracted from the dll
	pfnInitHW	InitHW;		// should be available
	pfnOpenHW	OpenHW;		// should be available
	pfnSetHWLO	SetHWLO;	// should be available
	pfnStartHW	StartHW;	// should be available
	pfnStopHW	StopHW;		// should be available
	pfnCloseHW	CloseHW;	// should be available
	pfnGetStatus	GetStatus;	// should be available
	pfnSetCallback	SetCallback;	// should be available
//
//	optionals
	pfnShowGUI	L_ShowGUI;
	pfnHideGUI	L_HideGUI;
	pfnGetHWSR	L_GetHWSR;	
	pfnGetFilters	L_GetFilters;
	pfnGetTune	L_GetTune;	
	pfnGetMode	L_GetMode;
	pfnGetHWLO	L_GetHWLO;	// should be available
	void		ShowGUI		(void);
	void		HideGUI		(void);

//	filename of dll
	QString		dll_filename;
	HINSTANCE	Handle;
	bool		dll_open;
	char		*rigName;
	char		*rigModel;
	int		hardwareType;
signals:
//	The following signals originate from the Winrad Extio interface
};
#endif

