
		DAB-RPI

This directory contains the implementation of a simple
dab/dab+ receiver that will run on an RPI 2.
The receiver supports terrestrial DAB reception with as input either
the stream from an AIRSPY, a SDRplay, a dabstick (direct
or through the rtl_tcp server) or a (prerecorded) file,
and it will output through the selected soundcard.

If configured for it - see ".pro" file or the CMakeLists.txt file -
the program will send its audio output to a tcp port.
A simple client is included - to be compiled and installed separately -
that can be used to map these PCI samples to a soundcard.
The client is available as a windows program in the windows-bin-dab folder
to be found on the website

Note:
The "latest" version is the version I am working on, it might be wise
to use the latest "stable" version.

Building:

Libraries -and their development files - that are needed are

qt		(4.8 or more)	tested with both Qt4 and Qt5
usbx	use a recent version
portaudio	0.19		some distros support by default an older version
fftw3f
faad
sndfile
zlib

Two possibilities for building the software are there: the Qt qmake tools
or the CMake tools.

QMake:
In the ".pro" file  one may select (or deselect) input devices by
uncommenting (commenting) the appropriate CONFIG= XXX lines.

A similar facility exists for the CMakeLists.txt file

Note that selecting a device requires installing the library and the
development files.

Tested on RPI with arch linux, and RPI with Raspbian Jessie, as well as under fedora 22 and Ubuntu 14.04
Cross compiled for W7/W10

Options:
Since an RPI is often run headless, an option is included to
configure such that the PCM output is sent to a simple TCP server, listening
at port 20040. 
uncommenting CONFIG+=TCP_STREAMER
will do here.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Note that the CMakeLists.txt file assumes Qt5, the sdr-j-dab.pro  file
(i.e. the qmake/make route) works fine with both Qt4 and Qt5 but
the names and locations of the libraries.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

Note on NOT handling
Comment or uncomment the line
DEFINE	+= MOT_BASICS__
DEFINE	+= MOT_DATA__
for excluding or including a preliminary handling of slides in DAB
Default is: commented out
##########################################################################

Extensions:

Depending on the settings in the dab-rpi.pro file, the output is
sent to the local "soundcard" or to a TCP port.
Add
CONFIG+=tcp-streamer
for having the output sent to port 20040. Note that in that
case there will be no sound output locally. I am using that feature
to have the RPI on a location different from where I am normally sitting.

A local "listener" program is available to catch the data and transfer it
to the soundcard. 

##########################################################################

Using the Windows executable is - obviously - only possible when the
required dll's are available. These dll's are not included here.

The executable is meant to replace the executable(s) in the distribution
(to me found in the website of the project).

