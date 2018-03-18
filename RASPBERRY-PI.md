welle.io on Raspberry Pi 2/3  
============================
This guide borrows parts from [Qt wiki](https://wiki.qt.io/RaspberryPi2EGLFS).  
This guide is a work in progress.  
If you have issues or something is completely wrong, please let us know at the [forums.](https://forum.welle.io/)  
---------------------------------------------------------------------------------
Table of contents  
=================

  * [Description](#description)
  * [Hardware requirements](#hardware-requirements)
  * [Setup](#setup)
  * [Building](#building)
  * [Power supply](#power-supply)
  * [Troubleshooting](#troubleshooting)
  * [Known issues](#known-issues)

Description
===========
This guide will help you compile and run welle.io on your Raspberry Pi 2/3 using the [Qt environment](https://www.qt.io) with Qt Creator.  
Qt will be set up for cross compiling from a desktop Linux computer since Qt most likely will not be able to be natively compiled on a Raspberry Pi due to memory constraints.  

Tested configurations are:
* Raspberry Pi 3 Model B
* Qt 5.9.3
* Qt 5.10.0
* Ubuntu 17.10 x64
* Raspbian Stretch

Hardware requirements
---------------------

**Raspberry Pi:**
* Raspberry Pi 2 or 3. The original Pi may, or may not work
* [Adequate power supply](#power-supply)
* 4GB or larger MicroSD card
* A RTL2832U DVB-T stick to receive radio signals  
(If you are using an Airspy device, compile and install the airspy library.)  
(For details please see [here](https://github.com/airspy/host/#how-to-build-the-host-software-on-linux))  
  
**Host computer:**
* Desktop/laptop computer running Ubuntu or an equivalent Debian Linux system  
(Other distributions will work, but is not tested)  
* A way to read/write MicroSD cards on a computer
* Wireless or wired connection between desktop computer and Raspberry Pi


Setup
-----

First we need to get Raspbian up and running on the Raspberry Pi.  
Only [Raspbian Stretch](https://www.raspberrypi.org/downloads/raspbian/) or newer will work, since Raspbian Jessie or older have a too old GCC/GLIBC version.  
The easiest way is to download [NOOBS.](https://www.raspberrypi.org/downloads/noobs/)  

1. Download [NOOBS.](https://downloads.raspberrypi.org/NOOBS_latest)  
   Make sure it's version 2.4.5 or later.  
2. Follow the [official quick start guide](https://www.raspberrypi.org/learning/software-guide/quickstart/) for installing Raspian.  
   ([Additional information](https://www.raspberrypi.org/documentation/installation/installing-images/README.md))  
   ([Additional information](https://www.raspberrypi.org/documentation/installation/noobs.md))  

**ON RASPBERRY PI:**

3. Update Raspbian and update to the latest packages.  
   ```
   sudo rpi-update
   sudo apt update
   sudo apt dist-upgrade
   ```
   Reboot your Pi
   ```
   sudo reboot
   ```
   (Optional but highly recommended: Use raspi-config and set up SSH to your Pi so you can remote control it from another computer)  
   (Optional: Use raspi-config and set GPU memory to 256)  
   (More info about raspi-config [here](https://www.raspberrypi.org/documentation/configuration/raspi-config.md))  
   ```
   sudo raspi-config
   ```
4. Install the required packages for welle.io and RTL-SDR.  
   ```
   sudo apt install libfaad-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev rtl-sdr
   ```   
5. Install a bunch of development files.  
   (For simplicity we use build-dep, not everything is really needed, but it is easier this way.)  
   Edit sources list in /etc/apt/sources.list and uncomment (remove #) the **deb-src** line:  
   ```
   sudo nano /etc/apt/sources.list
   ```
   Run update again to include deb-src in the repository list:  
   ```
   sudo apt update
   ```
   Install required libraries:
   ```
   sudo apt-get build-dep qt4-x11
   sudo apt-get build-dep qtbase-opensource-src
   ```
   ```
   sudo apt install libudev-dev libinput-dev libts-dev libxcb-xinerama0-dev libxcb-xinerama0
   ```
6. Prepare our target directory  
   ```
   sudo mkdir /usr/local/qt5pi
   sudo chown pi:pi /usr/local/qt5pi
   ```

Now we need to set up the toolchain, environments and directories on our host computer.  

**ON HOST COMPUTER:**

7. Make a raspi folder.  
   ```
   mkdir ~/raspi
   cd ~/raspi
   ```
8. Create a sysroot.  
   Using rsync we can properly keep things synchronized in the future as well.  
   Replace "raspberrypi.local" with the address of the Pi.  
   Depending on your connection speed, this might take a while, since you are basically copying the entire contents of your Pi, to your host PC.  
   It is important that you have already downloaded the required packages for welle.io on your Pi before you start rsync,
   because the cross compiling environment will use this sysroot folder as its source instead of the packages you might have installed on your host PC.  
   ```
   mkdir sysroot sysroot/usr sysroot/opt
   rsync -avz pi@raspberrypi.local:/lib sysroot
   rsync -avz pi@raspberrypi.local:/usr/include sysroot/usr
   rsync -avz pi@raspberrypi.local:/usr/lib sysroot/usr
   rsync -avz pi@raspberrypi.local:/opt/vc sysroot/opt
   ```
9. Adjust symlinks to be relative. Use provided script.  
   ```
   wget https://raw.githubusercontent.com/riscv/riscv-poky/master/scripts/sysroot-relativelinks.py
   chmod +x sysroot-relativelinks.py
   ./sysroot-relativelinks.py sysroot
   ```
10. Get a GCC toolchain for ARM systems.  
   ```
   wget https://releases.linaro.org/components/toolchain/binaries/latest-5/arm-linux-gnueabihf/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf.tar.xz
   tar -xvf gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf.tar.xz
   ```
   Or check the [website](https://releases.linaro.org/components/toolchain/binaries/latest-5/arm-linux-gnueabihf/) for the latest toolchain.  
   If you are using a 32-bit system, download the 32-bit version. (i686)  
 
Building
--------

11. Get Qt source.  
    We will use the entire Qt system instead of only qtbase, which is probably overkill, but makes it way easier to manage.  
	(Additional information about building Qt from source [here.](https://wiki.qt.io/Building_Qt_5_from_Git))  
    The target directory is /usr/local/qt5pi on the Pi, the host tools like qmake will go to ~/raspi/qt5, while make install will target ~/raspi/qt5pi (this is what we will sync to the device).
	Don't forget to adjust paths if you changed that. For some reason the ~/ in the paths may not work, if this the case just use full paths.
	```
	git clone git://code.qt.io/qt/qt5.git
	cd qt5
	git checkout 5.10
	```
	Currently, Qt version 5.10 is working.  
	If the branch version does not exist, try another/newer branch number.  
	Branch information for Qt is located [here.](http://code.qt.io/cgit/qt/qt5.git/)  
	Now we need to initialize the repository, which will download all the submodules we need for Qt.  
	We will not download qtwebkit or qtwebengine since these modules tend to create problems later, and are not needed for welle.io anyway.  
	```
	./init-repository --module-subset=default,-qtwebkit,-qtwebkit-examples,-qtwebengine
	```
	If the init failed due to network errors or similar, run the command again, but append **-f** at the end.  
12. Configure Qt.  
	You need to change **rpi-version** with a proper Raspberry Pi version.  
	Use: **linux-rasp-pi-g++** for RPi, **linux-rasp-pi2-g++** for RPi2 and **linux-rasp-pi3-g++** for RPi3.  
	If your system is 32-bit you may also edit device option to:  
	```
	-device-option CROSS_COMPILE=~/raspi/gcc-linaro-5.5.0-2017.10-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
	```
	Now it's time to set up and configure Qt for cross compiling to the ARM platform.  
	Don't forget to change **rpi-version**.  
	```
    ./configure -release -opengl es2 -device <rpi-version> -device-option CROSS_COMPILE=~/raspi/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf- -sysroot ~/raspi/sysroot -opensource -confirm-license -no-use-gold-linker -make libs -prefix /usr/local/qt5pi -extprefix ~/raspi/qt5pi -hostprefix ~/raspi/qt5 -v
	```
13. Compile Qt.  
    (Optional: use switch **-j** to tell make how many cores your cpu has.)  
	(Example, ***make -j4*** tells make to use four cpu cores, which greatly speeds up compile time.)  
    ```
	make
	make install
	```
	If anything failed, you can clean everything with:  
	```
	git clean -dfx
	```
14. Deploy Qt to the device.  
    We simply rsync everything from ~/raspi/qt5pi to the prefix we configured above.  
    ```
	cd ..
    rsync -avz qt5pi pi@raspberrypi.local:/usr/local
	```
	Now we will build an example to test if everything went well.  
	After the building is complete, we will copy the executable example to the device.  
	```
    cd qt5/qtbase/examples/opengl/qopenglwidget
    ~/raspi/qt5/bin/qmake
    make
    scp qopenglwidget pi@raspberrypi.local:/home/pi
    ```
	
Now we need to fix various links and issues on the Raspberry Pi.  

**ON RASPBERRY PI:**

15. Update the device to let the linker find the Qt libs:  
	```
    echo /usr/local/qt5pi/lib | sudo tee /etc/ld.so.conf.d/qt5pi.conf
    sudo ldconfig
	```
    If you're facing issues with running the example, try to use 00-qt5pi.conf instead of qt5pi.conf, to introduce proper order.  
16. Fix the EGL/GLES libraries.  
	The device may have the Mesa version of libEGL and libGLESv2 in /usr/lib/arm-linux-gnueabihf, resulting Qt apps picking these instead of the real thing from /opt/vc/lib.  
	This may be fine for X11 desktop apps not caring about OpenGL performance but is totally useless for windowing system-less, fullscreen embedded apps.  
	You may want to save the originals somewhere, just in case.  
	```
	sudo mv /usr/lib/arm-linux-gnueabihf/libEGL.so.1.0.0 /usr/lib/arm-linux-gnueabihf/libEGL.so.1.0.0_backup
	sudo mv /usr/lib/arm-linux-gnueabihf/libGLESv2.so.2.0.0 /usr/lib/arm-linux-gnueabihf/libGLESv2.so.2.0.0_backup
	sudo ln -s /opt/vc/lib/libEGL.so /usr/lib/arm-linux-gnueabihf/libEGL.so.1.0.0
	sudo ln -s /opt/vc/lib/libGLESv2.so /usr/lib/arm-linux-gnueabihf/libGLESv2.so.2.0.0
	```
	Please make sure to also add missing symbolic links:  
	```
	sudo ln -s /opt/vc/lib/libEGL.so /opt/vc/lib/libEGL.so.1
    sudo ln -s /opt/vc/lib/libGLESv2.so /opt/vc/lib/libGLESv2.so.2
	```
17. Run qopenglwidget example, that we've built before.  
    At this point it should just work at fullscreen with 60 FPS and mouse, keyboard, and possibly touch support.  
    ```
	sudo chmod +x /home/pi/qopenglwidget
	./qopenglwidget
	```

If the example is running smoothly, (or rather, running at all), congratulations, you now have Qt 5 on your Raspberry Pi.  
This is only half the battle though. Now we continue on to set up our host computer for welle.io cross compiling.  

**ON HOST COMPUTER:**

18. We now need the Qt environment for our host computer, including Qt Creator.  
	The easiest way to obtain this, is to download a precompiled Qt binary for our operating system. In this case, Ubuntu.  
	Go to [Qt website](https://www1.qt.io/download-open-source/#section-2) and download the appropriate package for your system.  
	For Ubuntu, Use [Online Installer Linux 64-bit.](http://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run)  
	Or the [32bit version](http://download.qt.io/official_releases/online_installers/qt-unified-linux-x86-online.run) if you have such a system.  
	Before we begin, start another instance of terminal for a fresh start, or **cd ..** your way back to your home folder.  
	```
	wget http://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
	sudo chmod +x qt-unified-linux-x64-online.run
	./qt-unified-linux-x64-online.run
	```
	Follow the instructions and install Qt, including submodules and Qt Creator.  
	Select version 5.9.3 or higher, and a QT Creator version.  
19. Clone welle.io.  
    ```
	git clone https://github.com/AlbrechtL/welle.io.git
	```
20. Start Qt Creator.  
21. Open project and select the welle.io folder that you just cloned and click on welle.io.pro.  
22. When the **configure project** screen comes up, click on **manage kits**  
    Then go to the **Qt Versions** tab and click **Add...**  
	Move to the **~/raspi/qt5/bin/qmake** folder and select the qmake there.  
	**Details** on the bottom should say something along the lines of "Qt version 5.9.3 for Embedded linux"  
	
	Now, go over to the **Compilers** tab and add **GCC C**  
	Name it "GCC ARM" or similar so we can easily identify it later.  
	In **Compiler path** browse to ~/raspi/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin and select **arm-linux-gnueabihf-gcc**  
	ABI should read "arm-linux-generic-elf-32bit"  
	
	Once again, click on **Add** and add a **GCC C++** compiler.  
	Name it "G++ ARM" or similar.  
	Compiler path should be the same as the previous one, but select **arm-linux-gnueabihf-g++** instead of "arm-linux-gnueabihf-gcc"  
	ABI should read "arm-linux-generic-elf-32bit", same as the previous one.  
	
	The last thing we could set up is in the **Debuggers** tab. This one isn't really needed, but lets do it anyway.  
	Press **Add** and name it "ARM GDB" or similar and the path should be the same as previous, but select **arm-linux-gnueabihf-gdb**.  
	With all this done, we can make a new kit.  
	
	Click on the **Kits** tab and **add**.  
	Name it "Raspberry Pi" or similar.  
	Device type: **Generic Linux Device**  
	Sysroot: **~/raspi/sysroot**  
	Compiler: C: **GCC ARM** (Or what you named your GCC compiler.)  
	Compiler: C++: **G++ ARM** (Or what you named your G++ compiler.)  
	Debugger: **ARM GDB** (Or what you named your GDB debugger.)  
	Qt version: **Qt 5.9.3 (qt5)** (The one you made, not the default one which most likely already was present in the "Qt Versions" tab.)  

    With all this done, click apply and ok.  
	Now your newly created "Raspberry Pi" kit should appear in the **Configure project** section. Select it and press **configure project.**  
	Stuff should happen and you should be greeted with a projects tree view and some other stuff, probably a Project "MESSAGE" message of some sort too.  
23. In order to compile welle.io for Raspberry Pi successfully, we need to do one adjustment to the source code.  
    Double click on **welle.io.pro** in the tree view,  
	scroll down to where it says **unix:!macx:!android:** find the line which says **CONFIG += airspy**  
	and comment it out by putting a **#** in front of it.  
	(Unless you need Airspy support.)  
	```
	#CONFIG += airspy
	```
	Save file (ctrl+s)  
24.	Now click on the monitor icon at the left hand side, most likely saying "welle.io debug" or similar, change the build to **release**.  
	Wait a couple of seconds until the green arrows lights up again.  
25. Click on the hammer icon, which means **build project**.  
    Qt Creator will build welle.io and output the finished program into a "release" folder most likely named "build-welle-io-Raspberry_Pi-Release".  
26. Open a new terminal and navigate to the release folder.  
    Transfer the program over to the Raspberry Pi.
    ```
	cd ~/build-welle-io-Raspberry_Pi-Release
	scp welle-io pi@raspberrypi.local:/home/pi
	```
	
**ON RASPBERRY PI:**

27. Find the file **welle-io** and run it to enjoy welle.io on your Raspberry Pi.  
    Open a new terminal and type:  
    ```
	./welle-io --disable-splash
	```
    The **--disable-splash** argument is used because otherwise welle.io will crash on Raspberry Pi.  
    See the troubleshooting section at the bottom for details.  
    	
Power supply
============

Raspberry Pi 3 requires significantly more power than the original Pi, which could easily be run off a generic USB phone charger.  
The problem with such chargers is the fact that the USB cable is almost always too thin to reliably supply the required 5 volts the Pi 3 needs to run.  
Also, a phone charger is not necessarily a good power supply in general.  
Even powerful Apple 12W iPad chargers which outputs amps and amps of current, fails to deliver more than 4.6-4.8 volts to the Pi, which forces the Pi to go into "undervoltage" mode.  
In undervoltage mode, the Pi reduces the speed of the CPU and GPU, which affects the entire operation of the system.  
Hiccups in other parts of the system might occur, such as in RAM and I/O chips.  
Too low voltage is noticeable by a lightning bolt icon in the top right corner of the screen.  
More info about the power requirements [here](https://www.raspberrypi.org/help/faqs/#powerReqs).  
The official [Raspberry Pi universal power supply](https://www.raspberrypi.org/products/raspberry-pi-universal-power-supply/) is a more beefy (and proper) power supply that provides 5.1 volts over a thicker cable, which mitigates the power loss experienced in thinner generic USB cables.  
However, when running with lots of peripherals, like a touch screen, mouse, keyboard, gamepad, bluetooth dongle, Wi-Fi dongle, RTL-SDR dongle and whatnot, even an official power supply struggles.  
The solution is to somehow add another official power supply, or an even beefier power supply that can output 5.1 volt, 4+ amps.  

When the Pi goes into undervoltage mode, it may or may not affect the SDR dongle, and its operation with welle.io, causing random malfunctions in radio reception.  
The SD card might be corrupt if it is written to while the Pi experiences undervoltage.  
Using the Pi in undervoltage mode is not the end of the world, but expect the unexpected.

Troubleshooting
===============

* If you have no audio out when using an external sound card like a HiFiBerry DAC+, IQAudIO PiDAC+, PiMoroni pHAT DAC, JustBoom or any USB DAC, try installing Pulseaudio.  
  ```
  sudo apt install pulseaudio
  ```
* If you for some reason don't have any text in either the qopenglwidget example or in welle.io and get a message like this in the terminal:  
  ```
  QFontDatabase: Cannot find font directory /usr/local/qt5pi/lib/fonts.
  Note that Qt no longer ships fonts. Deploy some (from http://dejavu-fonts.org for example) or switch to fontconfig.
  ```
  It means the fonts folder for the Qt environment does not exist.
  To fix this, we simply make the folder and populate it with some fonts.
  ```
  mkdir /usr/local/qt5pi/lib/fonts
  ```
  Then we steal a font from the system font folder.  
  ```
  cp /usr/share/fonts/truetype/freefont/FreeSans.ttf /usr/local/qt5pi/lib/fonts/FreeSans.ttf
  ```
  You can basically put any font you want in the fonts folder.  
* If your screen goes blank after a set time, have a look at the [official documentation](https://www.raspberrypi.org/documentation/configuration/screensaver.md) for screensavers.  
* Error: **EGLFS: OpenGL windows cannot be mixed with others.** appears, this means the splash screen is not working correctly on Raspberry Pi.  
To work around this issue, start welle.io with the argument **--disable-splash**.  
```
./welle-io --disable-splash
```
* When using VNC or another form of remote desktop software,  
OpenGLES will not output any Qt windows to the remote desktop,  
only a fullscreen window on an attached screen, such as HDMI or DSI port.  
To work around this issue, use argument **-platform xcb** when starting welle.io.  
You might have to install the proper xcb libraries if these are not present on your Raspberry Pi.
```
./welle-io -platform xcb
```

Known issues
-----------
* When using a touch screen, the user interface will be "transparent" to your screen touches.  
This means that you can accidentally click on icons on the desktop itself, behind the user interface of the program, while operating the program as normal.  
It is not known if this is a Qt issue, welle.io issue or a Raspbian driver issue.  
For now, take care when operating the user interface with a touch screen.
