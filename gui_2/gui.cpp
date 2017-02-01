#
/*
 *    Copyright (C) 2013, 2014, 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  SDR-J (JSDR).
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are acknowledged.
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
#include	<QSettings>
#include	"dab-constants.h"
#include	"gui.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#ifdef  TCP_STREAMER
#include        "tcp-streamer.h"
#endif
#ifdef	HAVE_DABSTICK
#include	"dabstick.h"
#endif
#ifdef	HAVE_SDRPLAY
#include	"sdrplay.h"
#endif
#ifdef	HAVE_RTL_TCP
#include	"rtl_tcp_client.h"
#endif
#ifdef	HAVE_AIRSPY
#include	"airspy-handler.h"
#endif

/**
  *	We use the creation function merely to set up the
  *	user interface and make the connections between the
  *	gui elements and the handling agents. All real action
  *	is embedded in actions, initiated by gui buttons
  */
#define	BASE_PORT	20020
	RadioInterface::RadioInterface (QSettings	*Si,
	                                QString		device,
	                                uint8_t		dabMode,
	                                QString		dabBand,
                                        QObject		*parent):
	                                     QObject (parent) {
int16_t	latency;

	dabSettings		= Si;
//
//	Before printing anything, we set
	setlocale (LC_ALL, "");
//	
//	the name of the device is passed on from the main program
	if (!setDevice (device)) {
	   fprintf (stderr, "NO VALID DEVICE, GIVING UP\n");
	   exit (1);
	}
	running			= false;
	
/**	threshold is used in the phaseReference class 
  *	as threshold for checking the validity of the correlation result
  */
	threshold	=
               dabSettings -> value ("threshold", 3). toInt ();

	isSynced		= UNSYNCED;
//
//	latency is used to allow different settings for different
//	situations wrt the output buffering
	latency			=
	           dabSettings -> value ("latency", 1). toInt ();
/**
  *	With this GUI there is no choice for the output channel,
  *	It is the tcp streamer, since we work on a different machine
  */
	audioBuffer		= new RingBuffer<int16_t>(2 * 32768);
	soundOut		= new tcpStreamer (audioBuffer, 20040);
//
	this -> dabBand		= dabBand == "BAND III" ? BAND_III : L_BAND;
	setModeParameters (dabMode);
/**
  *	The actual work is done elsewhere: in ofdmProcessor
  *	and ofdmDecoder for the ofdm related part, ficHandler
  *	for the FIC's and mscHandler for the MSC.
  *	The ficHandler shares information with the mscHandler
  *	but the handlers do not change each others modes.
  */
	my_mscHandler	= new mscHandler (this,
                                          &dabModeParameters,
                                          audioBuffer,
                                          false);
	my_ficHandler	= new ficHandler (this);
/**
  *	The default for the ofdmProcessor depends on
  *	the input device, note that in this setup the
  *	device is selected on start up and cannot be changed.
  */
	my_ofdmProcessor = new ofdmProcessor   (inputDevice,
	                                        &dabModeParameters,
	                                        this,
	                                        my_mscHandler,
	                                        my_ficHandler,
	                                        threshold,
	                                        3);
//	display the version
//	QString v = "sdr-j DAB-rpi(+)  " ;
//	v. append (CURRENT_VERSION);
	notConnected	= true;
	connect (&server, SIGNAL (newConnection (void)),
	         this, SLOT (acceptConnection (void)));
	server. listen (QHostAddress::Any, 20020);
	ficBlocks	= 0;
	ficSuccess	= 0;
	haveStereo	= false;
}

	RadioInterface::~RadioInterface () {
}
//
/**
  *	\brief At the end, we might save some GUI values
  *	The QSettings could have been the class variable as well
  *	as the parameter
  */
void	RadioInterface::dumpControlState (QSettings *s) {
	if (s == NULL)	// cannot happen
	   return;
}
//
///	the values for the different Modes:
void	RadioInterface::setModeParameters (uint8_t Mode) {

	if (Mode == 2) {
	   dabModeParameters. dabMode	= 2;
	   dabModeParameters. L		= 76;		// blocks per frame
	   dabModeParameters. K		= 384;		// carriers
	   dabModeParameters. T_null	= 664;		// null length
	   dabModeParameters. T_F	= 49152;	// samples per frame
	   dabModeParameters. T_s	= 638;		// block length
	   dabModeParameters. T_u	= 512;		// useful part
	   dabModeParameters. guardLength	= 126;
	   dabModeParameters. carrierDiff	= 4000;
	} else
	if (Mode == 4) {
	   dabModeParameters. dabMode		= 4;
	   dabModeParameters. L			= 76;
	   dabModeParameters. K			= 768;
	   dabModeParameters. T_F		= 98304;
	   dabModeParameters. T_null		= 1328;
	   dabModeParameters. T_s		= 1276;
	   dabModeParameters. T_u		= 1024;
	   dabModeParameters. guardLength	= 252;
	   dabModeParameters. carrierDiff	= 2000;
	} else 
	if (Mode == 3) {
	   dabModeParameters. dabMode		= 3;
	   dabModeParameters. L			= 153;
	   dabModeParameters. K			= 192;
	   dabModeParameters. T_F		= 49152;
	   dabModeParameters. T_null		= 345;
	   dabModeParameters. T_s		= 319;
	   dabModeParameters. T_u		= 256;
	   dabModeParameters. guardLength	= 63;
	   dabModeParameters. carrierDiff	= 2000;
	} else {	// default = Mode I
	   dabModeParameters. dabMode		= 1;
	   dabModeParameters. L			= 76;
	   dabModeParameters. K			= 1536;
	   dabModeParameters. T_F		= 196608;
	   dabModeParameters. T_null		= 2656;
	   dabModeParameters. T_s		= 2552;
	   dabModeParameters. T_u		= 2048;
	   dabModeParameters. guardLength	= 504;
	   dabModeParameters. carrierDiff	= 1000;
	}

	spectrumBuffer = new DSPCOMPLEX [dabModeParameters. T_u];
        memset (spectrumBuffer, 0,
	                  dabModeParameters.T_u * sizeof (DSPCOMPLEX));
}

struct dabFrequencies {
	const char	*key;
	int	fKHz;
};

struct dabFrequencies bandIII_frequencies [] = {
{"5A",	174928},
{"5B",	176640},
{"5C",	178352},
{"5D",	180064},
{"6A",	181936},
{"6B",	183648},
{"6C",	185360},
{"6D",	187072},
{"7A",	188928},
{"7B",	190640},
{"7C",	192352},
{"7D",	194064},
{"8A",	195936},
{"8B",	197648},
{"8C",	199360},
{"8D",	201072},
{"9A",	202928},
{"9B",	204640},
{"9C",	206352},
{"9D",	208064},
{"10A",	209936},
{"10B", 211648},
{"10C", 213360},
{"10D", 215072},
{"11A", 216928},
{"11B",	218640},
{"11C",	220352},
{"11D",	222064},
{"12A",	223936},
{"12B",	225648},
{"12C",	227360},
{"12D",	229072},
{"13A",	230748},
{"13B",	232496},
{"13C",	234208},
{"13D",	235776},
{"13E",	237488},
{"13F",	239200},
{NULL, 0}
};

struct dabFrequencies Lband_frequencies [] = {
{"LA", 1452960},
{"LB", 1454672},
{"LC", 1456384},
{"LD", 1458096},
{"LE", 1459808},
{"LF", 1461520},
{"LG", 1463232},
{"LH", 1464944},
{"LI", 1466656},
{"LJ", 1468368},
{"LK", 1470080},
{"LL", 1471792},
{"LM", 1473504},
{"LN", 1475216},
{"LO", 1476928},
{"LP", 1478640},
{NULL, 0}
};

static 
const char *table12 [] = {
"none",
"news",
"current affairs",
"information",
"sport",
"education",
"drama",
"arts",
"science",
"talk",
"pop music",
"rock music",
"easy listening",
"light classical",
"classical music",
"other music",
"wheather",
"finance",
"children\'s",
"factual",
"religion",
"phone in",
"travel",
"leisure",
"jazz and blues",
"country music",
"national music",
"oldies music",
"folk music",
"entry 29 not used",
"entry 30 not used",
"entry 31 not used"
};

const char *RadioInterface::get_programm_type_string (uint8_t type) {
	if (type > 0x40) {
	   fprintf (stderr, "GUI: programmtype wrong (%d)\n", type);
	   return (table12 [0]);
	}

	return table12 [type];
}

static
const char *table9 [] = {
"unknown",
"Albanian",
"Breton",
"Catalan",
"Croatian",
"Welsh",
"Czech",
"Danish",
"German",
"English",
"Spanish",
"Esperanto",
"Estonian",
"Basque",
"Faroese",
"French",
"Frisian",
"Irish",
"Gaelic",
"Galician",
"Icelandic",
"Italian",
"Lappish",
"Latin",
"Latvian",
"Luxembourgian",
"Lithuanian",
"Hungarian",
"Maltese",
"Dutch",
"Norwegian",
"Occitan",
"Polish",
"Postuguese",
"Romanian",
"Romansh",
"Serbian",
"Slovak",
"Slovene",
"Finnish",
"Swedish",
"Tuskish",
"Flemish",
"Walloon"
};

const char *RadioInterface::get_programm_language_string (uint8_t language) {
	if (language > 43) {
	   fprintf (stderr, "GUI: wrong language (%d)\n", language);
	   return table9 [0];
	}
	return table9 [language];
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//	
//	The public slots are called from other places within the dab software
//	so please provide some implementation, perhaps an empty one
//
//	a slot called by the ofdmprocessor
void	RadioInterface::set_fineCorrectorDisplay (int v) {
	fineCorrector = v;
	showMessage (COARSE_CORRECTOR, coarseCorrector + v);
}

//	a slot called by the ofdmprocessor
void	RadioInterface::set_coarseCorrectorDisplay (int v) {
	coarseCorrector = v * kHz (1);
	showMessage (COARSE_CORRECTOR, coarseCorrector + fineCorrector);
}
/**
  *	clearEnsemble
  *	on changing settings, we clear all things in the gui
  *	related to the ensemble.
  *	The function is called from "deep" within the handling code
  *	Potentially a dangerous approach, since the fic handler
  *	might run in a separate thread and generate data to be displayed
  */
void	RadioInterface::clearEnsemble	(void) {
//
//	it obviously means: stop processing
	my_mscHandler		-> stopProcessing ();
	my_ficHandler		-> clearEnsemble ();
	my_ofdmProcessor	-> coarseCorrectorOn ();
	my_ofdmProcessor	-> reset ();
	showMessage (CLEAR_ENSEMBLE);
}

//
//	a slot, called by the fic/fib handlers
void	RadioInterface::addtoEnsemble (const QString &s) {
//	Add new station into list
	if (!s.contains ("data") && !stationList.contains (s)) {
	   stationList.append (s);
	   showMessage (PROGRAM_NAME, s);
	}
}

//
///	a slot, called by the fib processor
void	RadioInterface::nameofEnsemble (int id, const QString &s) {
	if (ensemble != s) {
	   showMessage (ENSEMBLE_NAME, s);
	   ensemble = s;
	}
	my_ofdmProcessor	-> coarseCorrectorOff ();
}

///	just switch a color, obviously GUI dependent, but called
//	from the ofdmprocessor
void	RadioInterface::setSynced	(char b) {
	if (isSynced == b)
	   return;

	isSynced = b;
	switch (isSynced) {
           case SYNCED:
	      showMessage (SYNC_FLAG, 1);
	      break;

	   default:
	      showMessage (SYNC_FLAG, 0);
              break;
	}
}

//	showLabel is triggered by the message handler
//	the GUI may decide to ignore this
void	RadioInterface::showLabel	(QString s) {
	showMessage (STATION_TEXT, "");
	showMessage (STATION_TEXT, s);
}
//
//	showMOT is triggered by the MOT handler,
void	RadioInterface::showMOT  (QByteArray data, int subtype, QString s) {
	(void)data; (void)subtype; (void)s;
}
//
//	sendDatagram is triggered by the ip handler, just ignore
void	RadioInterface::sendDatagram	(char *data, int length) {
	(void)data;
	(void)length;
}

/**
  *	\brief changeinConfiguration
  *	No idea yet what to do, so just give up
  *	with what we were doing. The user will -eventually -
  *	see the new configuration from which he can select
  */
void	RadioInterface::changeinConfiguration	(void) {
	if (running) {
	   soundOut		-> stop ();
	   inputDevice		-> stopReader ();
	   inputDevice		-> resetBuffer ();
	   running		= false;
	}
}
//
//	The audio is sent back from the audio decoder to the GUI
//	The gui will sent it to the appropriate soundhandler,
//	which for this GUI is the soundcard
//	Note the - when shutting down - some signals might
//	still wait for handling
void	RadioInterface::newAudio	(int rate) {
	if (running)
	   soundOut	-> audioOut (rate);
}

/**
  *	\brief show_successRate
  *	a slot, called by the MSC handler to show the
  *	percentage of frames that could be handled
  */
void	RadioInterface::show_frameErrors (int s) {
	showMessage (SUCCESS_RATE, s);
}

void	RadioInterface::show_rsErrors (int s) {
	(void)s;
}

void	RadioInterface::show_aacErrors (int s) {
	(void)s;
}

///	called from the ofdmDecoder, which computes this for each frame
void	RadioInterface::show_snr (int s) {
	showMessage (SIGNAL_POWER, s);
}

//	if so configured, the function might be triggered
//	from the message decoding software. The GUI
//	might decide to ignore the data sent
void	RadioInterface::show_mscErrors          (int err) {
	(void)err;
}

//	a slot, called by the iphandler
void	RadioInterface::show_ipErrors	(int er) {
	(void)er;
}
//
void    RadioInterface::setStereo (bool isStereo) {
	if (haveStereo == isStereo)
	   return;
	haveStereo	= isStereo;
	showMessage (STEREO_FLAG, isStereo);
}
//
//
void	RadioInterface::show_ficSuccess (bool b) {
        if (b)
           ficSuccess ++;
        if (++ficBlocks >= 100) {
	   showMessage (FIC_FLAG, ficSuccess);
           ficSuccess   = 0;
           ficBlocks    = 0;
        }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//	
//	Private slots relate to the modeling of the GUI
//
/**
  */
void	RadioInterface::setStart	(void) {
bool	r = 0;
	if (running)		// only listen when not running yet
	   return;
//
	r = inputDevice		-> restartReader ();
	qDebug ("Starting %d\n", r);
	if (!r) {
	   qDebug ("Opening  input stream failed\n");
	   return;
	}
//	Of course, starting the machine will generate a new instance
//	of the ensemble, so the listing - if any - should be cleared
	clearEnsemble ();		// the display
//
///	this does not hurt
	soundOut	-> restart ();
	running = true;
}

/**
  *	\brief TerminateProcess
  *	Pretty critical, since there are many threads involved
  *	A clean termination is what is needed, regardless of the GUI
  */
void	RadioInterface::TerminateProcess (void) {
	running			= false;
	inputDevice		-> stopReader ();	// might be concurrent
	my_mscHandler		-> stopHandler ();	// might be concurrent
	my_ofdmProcessor	-> stop ();	// definitely concurrent
	soundOut		-> stop ();
//
//	everything should be halted by now
	dumpControlState (dabSettings);
	delete		my_ofdmProcessor;
	delete		my_ficHandler;
	delete		my_mscHandler;
	delete		soundOut;
	soundOut	= NULL;		// signals may be pending, so careful
	delete		inputDevice;
	QApplication::quit();

}

/**
  */
void	RadioInterface::setChannel (QString s) {
int16_t	i;
struct dabFrequencies *finger;
bool	localRunning	= running;
int32_t	tunedFrequency;

	clearEnsemble ();
	if (localRunning) {
	   soundOut	-> stop ();
	   inputDevice		-> stopReader ();
	   inputDevice		-> resetBuffer ();
	}

	tunedFrequency		= 0;
	if (dabBand == BAND_III)
	   finger = bandIII_frequencies;
	else
	   finger = Lband_frequencies;

	for (i = 0; finger [i]. key != NULL; i ++) {
	   if (finger [i]. key == s) {
	      tunedFrequency	= KHz (finger [i]. fKHz);
	      break;
	   }
	}

	if (tunedFrequency == 0)
	   return;

	inputDevice		-> setVFOFrequency (tunedFrequency);

	if (localRunning) {
	   soundOut -> restart ();
	   my_ofdmProcessor	-> reset ();
	   inputDevice	 -> restartReader ();
	   running	 = true;
	}
}

//	Note that the audiodata or the packetdata contains quite some
//	info on the service (i.e. rate, address, etc)
//	Here we only support audio services.
void	RadioInterface::setService (QString a) {

	switch (my_ficHandler -> kindofService (a)) {
	   case AUDIO_SERVICE:
	      { audiodata d;
	        my_ficHandler	-> dataforAudioService (a, &d);
	        my_mscHandler	-> set_audioChannel (&d);
	        break;
	      }
//
//	For the remorely controlled device, we do not have a data service
	   default:
	      return;
	}
}

void	RadioInterface::autoCorrector_on (void) {
//	first the real stuff
	my_ficHandler		-> clearEnsemble ();
	my_ofdmProcessor	-> coarseCorrectorOn ();
	my_ofdmProcessor	-> reset ();
}

/**
  *	\brief setDevice
  *	In this version, a device is specified in the command line
  *	or a default is taken. I.e., no dynamic switching of devices
  */
//
bool	RadioInterface::setDevice (QString s) {
bool	success;
#ifdef HAVE_AIRSPY
	if (s == "airspy") {
	   inputDevice	= new airspyHandler (dabSettings, &success, false);
	   if (!success) {
	      delete inputDevice;
	      inputDevice = new virtualInput ();
	      return false;
	   }
	   else 
	      return true;
	}
	else
#endif
#ifdef HAVE_RTL_TCP
//	RTL_TCP might be working. 
	if (s == "rtl_tcp") {
	   inputDevice = new rtl_tcp_client (dabSettings, &success, true);
	   if (!success) {
	      delete inputDevice;
	      inputDevice = new virtualInput();
	      return false;
	   }
	   else
	      return true;
	}
	else
#endif
#ifdef	HAVE_SDRPLAY
	if (s == "sdrplay") {
	   inputDevice	= new sdrplay (dabSettings, &success, false);
	   if (!success) {
	      delete inputDevice;
	      inputDevice = new virtualInput ();
	      return false;
	   }
	   else 
	      return true;
	}
	else
#endif
#ifdef	HAVE_DABSTICK
	if (s == "dabstick") {
	   inputDevice	= new dabStick (dabSettings, &success, false);
	   if (!success) {
	      delete inputDevice;
	      inputDevice = new virtualInput ();
	      return false;
	   }
	   else
	      return true;
	}
	else
#endif
    {	// s == "no device"
//	and as default option, we have a "no device"
	   inputDevice	= new virtualInput ();
	}
	return false;
}

void RadioInterface::showCorrectedErrors (int Errors) {
	(void)Errors;
}

////////////////////////////////////////////////////////////////////////////
//
//	Server stuff
//

void	RadioInterface::acceptConnection (void) {
	if (!notConnected)
	   return;

	client = server. nextPendingConnection ();
	QHostAddress s = client -> peerAddress ();
	fprintf (stderr, "Accepted a client %s\n",
	                     s.toString (). toLatin1 (). data ());
	QByteArray a;
	a. resize (client -> bytesAvailable ());
	if (a. size () > 0) {
	   client -> read (a. data (), a. size ());
	}

	connect (client, SIGNAL (readyRead (void)),
	         this, SLOT (processCommand (void)));
}

//	We actually only have a handful of commands.
void	RadioInterface::processCommand (void) {
QByteArray a;
char	command;
int16_t	length;
char	x;

	while (client -> bytesAvailable () >= 2) {
	   client -> read (&command, 1);
	   client -> read (&x,  1);
	   length	=  (uint8_t)(x);
	   a. resize (length - 2);
	   client -> read (a. data (), length - 2);
	   switch (command) {
	      case 0170:		// start
	         setStart ();
	         break;
	      case 0171:		// terminate;
	         notConnected	 = true;
	         TerminateProcess	();
	         break;
	      case 0172:		// set Channel
	         setChannel (stringFrom (a));
	         break;
	      case 0173:		// set program
	         setService (stringFrom (a));
	         break;
	      case 0174:		// set attenuation
	         inputDevice	-> setGain (a [0]);
	         break;
	      default:
	         fprintf (stderr, "sorry %o not implemented\n", 
	                                           int32_t (command & 0xFF));
	         break;
	   }
	}
}

QString	RadioInterface::stringFrom (QByteArray a) {
QString Result;
int16_t	i;

	for (i = 0; i < a. size (); i ++) {
	   if (a [i] == '\0')
	      return Result;
	   Result. append (char (a [i]));
	}
	return Result;
}

void	RadioInterface::showMessage	(int m) {
QByteArray Message;
	Message. resize (3);
	Message [0] = 0266;	// start of message
	Message [1] = 03;	// length of Message
	Message [2] = m;
	client	-> write (Message. data (), Message. size ());
}

void	RadioInterface::showMessage	(int m, int v) {
QByteArray Message;
	Message. resize (7);	
	Message [0]	= 0266;	// start of message
	Message [1]	= 07;	// length of Message
	Message [2]	= m;
	Message [3]	= (v >> 24) & 0xFF;
	Message [4]	= (v >> 16) & 0xFF;
	Message [5]	= (v >>  8) & 0xFF;
	Message [6]	=         v & 0xFF;
	client	-> write (Message. data (), Message. size ());
}

void	RadioInterface::showMessage	(int m, QString s) {
int16_t	l	= s. length ();
QByteArray Message;
int16_t	i;

	Message. resize (l + 1 + 3);
	for (i = 0; i < l; i ++)
	   Message [i + 3] = s. toLatin1 (). data () [i];
	Message [l + 3] = '\0';

	Message [0]	= 0266;		// new message
	Message [1]	= l + 1 + 3;	// length
	Message [2]	= m;
	client	-> write (Message. data (), Message. size ());
}

