
When configuring and building with selected gui_2,
the dab program itself does not have a GUI, but is is controlled remotely.

Some parameters can be set through the command line on starting the program
-D device, selects the device (one of the configured ones, default a dabstick);
-B Band, selects the DAB band (default Band III);
-M Mode, selects the DAB Mode (default Mode 1);
-I Ip address, only useful when selecting rtl_tcp as input device

The interface is simple:
commands to the dab-rpi program are transmitted through port 20020,and there are only a few.

The output, i.e the stream of PCM samples, is sent through port 20040. The "demo" controller for which the sources are available 
listens to that port and will send the samples to the selected soundcard on the controlling computer.

Commands are passed as packets with a very simple structure
1 byte for the command
1 byte for the length of the message
if needed N bytes for parameters

Commands are

    0170 : no parameters, call setStart
    0171 : no parameters, call terminate process
    0172 : one parameter, a sequence of characters forming the name of 
           the channel to be selected
    0173 : one parameter, a sequence of characters forming the name of
           the program in the current ensemble to be selected.
    0174 : one (byte) parameter, the value for the gain (attenuation) to be used.

The dab-rpi, when configured with the gui_2, will send messages back to the same port out,
for the remore control to be interpreted.

The format of the messages is
	first byte	0266
	second byte	message type
	third byte	length of the message
Rge messages themselves are
	COARSE_CORRECTOR	= 1,	// parameter is an int
	CLEAR_ENSEMBLE		= 2,	// no parameter
	ENSEMBLE_NAME		= 3,	// parameter is a QString
	PROGRAM_NAME		= 4,	// parameter is a QString
	SUCCESS_RATE		= 5,	// parameter is an int
	SIGNAL_POWER		= 6,	// parameter is an int
	SYNC_FLAG		= 7,	// parameter is an int
	STATION_TEXT		= 8,	// parameter is unknown
	FIC_FLAG		= 9,	// parameter is an int
	STEREO_FLAG		= 10	// parameter is an int

For building gui_2 using qmake/make. Note that this version
still strongly depends on Qt, so although there are no visual signs,
the Qt libraries need to be installed.

