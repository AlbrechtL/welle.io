

When configuring and building with gui_2 selected, the resulting program will  be controlle completely from the
command line, there is no GUI at all. Note that since the program is a variant of the the dab-rpi program, it
is still a Qt based program. While it does not show a GUI, it uses quite some Qt classes and the Qt libraries are still needed,
(obviously it can be rewritten such that Qt libraries are not needed anymore, feel free to do so).

The gui_2 configuration is still pretty experimental, and may change almost daily.

Parameters can be set through the command line on starting the program:

-D device, selects the device (one of the configured ones, default a dabstick);

-B Band, selects the DAB band (default Band III),

-M Mode, selects the DAB Mode (default Mode 1),

-I Ip address, only useful when selecting rtl_tcp as input device, port 1234 will be used.

-C the channel the default is 11C, the channel I am listening to mostly,

-P the program name, a prefix suffices. For e.g. "Classic FM" it suffices to give "classic". However, when passing on a non-unique prefix (e.g. "radio" for "Radio Maria" and "Radio Veronica") the software will select one arbitrarily. Note that letter case is unimportant. Note that the names of the programs in the ensemble being received in the selected channel will be printed during recognition.
Important: If no program names are found, or if no match can be made between the
program name and the list of program names, the program has no other choice than halt.

-G the gain to be applied on the device, in the range from 1 .. 100.

-A the output channel, again as with the program name, a prefix of the name suffices. As with the programs, the names of the sound channels identified will be printed. Note, however, that not all names appearing on the namelist are useful,
some of them will just not work, a well known  issue with the combination portaudio/alsa under Linux. 
Important: If a name is selected for a channel that cannot be opened the program will try to open the default output device.

Important:
_________

For each of the parameters there is a default. Any setting of a parameter
in the command line will be stored and remembered for a next time.
I.e., if the command

	./linux/dab-rpi-0.997 -M 1 -B "BAND III"  -D airspy -C 12C -P "Radio 4" -G 80 -A default
	
is given, the next time a command

	./linux/dab-rpi-0.997
	
will "know" to use the airspy as device, will select Mode, Band, channel, gain and -if available - will make "Radio 4" audible
through audiochannel "default", i.e. as set by the previous incarnation of the program.

Output - if any - is either through the soundcard or through the tcp streamer,
depending on the configuration. If the tcp streamer is in the configuration, the "-A" option does not have any effect,
the output, i.e the stream of PCM samples, is sent through port 20040.

For building GUI_2 using qmake/make, see GUI_1

