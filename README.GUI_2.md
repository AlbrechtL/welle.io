

When configuring and building with selected gui_2,
the dab program itself does not have a GUI.
It is controlled through the command line.

Some parameters can be set through the command line on starting the program

-D device, selects the device (one of the configured ones, default a dabstick);

-B Band, selects the DAB band (default Band III),

-M Mode, selects the DAB Mode (default Mode 1),

-I Ip address, only useful when selecting rtl_tcp as input device,

-C the channel,
-P the program name, a prefix suffices. For e.g. "Classic FM" it suffices
to give "classic". However, when passing on a non-unique prefix (e.g. "radio" for "Radio Maria" and "Radio Veronica") the software will select one arbitrarily. Note that letter case is unimportant.

-G the gain to be applied on the device, in the range from 1 .. 100.

Important:
_________
For each of the parameters there is a default. Any setting of a parameter
in the command line will be remembered for a next time.
I.e., if the command

	./linux/dab-rpi-0.997 -D airspy -C 12C -P "Radio 4" -G 80
	
is given, the next time a command

	./linux/dab-rpi-0.997
	
will "know" to use the airspy as device, will select channel 12C and -if available - will make "Radio 4" audible.

Output - if any - is either through the soundcard or through the tcp streamer,
depending on the configuration.
If the tcp streamer is configured, the output, i.e the stream of PCM samples, is sent through port 20040.

For building GUI_2 using qmake/make, see GUI_1

