dab-rpi
=====================
This repository contains the implementation of a simple DAB/DAB+ receiver. 
It is  derived from the sdr-j-dab software, optimized for embedded
systems like Raspberry Pi 2 and Raspberry Pi 3 but it runs on regulars PC as well.

The receiver supports terrestrial DAB and DAB+ reception with as input either the  samplestream from an AIRSPY, a SDRplay, a dabstick (rtl_sdr), a rtl_tcp server or a (prerecorded) file, and it will output through the selected soundcard 0r - if configured - through a TCP connection.

There are three versions of the dab-rpi software. 

1. GUI_1  is a GUI by using regular QT widgets, it can be build with Qt4 and Qt5;
2. GUI_2  is not a GUI at all, the program is controlled remotely, a simple remote controller is included in the sources; 
it can be build using Qt4 and Qt5;
3. GUI_3  is a touch and high DPI display optimized GUI based in QT QML. But it runs also well on regular PCs. It can be build using Qt5.7 and higher.

The GUI is selected in the configuration file. the "dab-rpi.pro" file. Note that the CMakeLists.txt file currently only supports creating an executable with the GUI_1 profile.


unix {

CONFIG		+= dabstick

 ......
 
CONFIG		+= gui_1 (or gui_2 or gui_3)

DESTDIR		= ./linux-bin

 ....
 
LIBS		+= -lfaad

}


For a detailed description of how to build the "GUI_1" version, see "README.GUI_1.
For a description of how to build the "GUI_2" version, see "README.GUI_2.
For a detailed description of how to build the "GUI_3" version, see "README.GUI_3.

