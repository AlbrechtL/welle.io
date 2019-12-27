---
#
# Use the widgets beneath and the content will be
# inserted automagically in the webpage. To make
# this work, you have to use › layout: frontpage
#
layout: page-fullwidth
header:
  image_fullwidth: banner5.jpg
lang: en
ref: index
#widget1:
#  title: "Widget 1"
#  url: 'http://domain.de/must-be-absolut-url-like-this-one/'
#  image: 'http://dummyimage.com/302x183/334d5c/efc94c.png&text=Placeholder'
#  text: ''
#widget2:
#  title: "Widget 2"
#  url: 'http://domain.de/must-be-absolut-url-like-this-one/'
#  image: 'http://dummyimage.com/302x183/334d5c/efc94c.png&text=Placeholder'
#  text: ''
#widget3:
#  title: "Widget 3"
#  url: 'http://domain.de/must-be-absolut-url-like-this-one/'
#  image: 'http://dummyimage.com/302x183/334d5c/efc94c.png&text=Placeholder'
#  text: ''
#
# Use the call for action to show a button on the frontpage
#
# To make internal links, just use a permalink like this
# url: /getting-started/
#
# To style the button in different colors, use no value
# to use the main color or success, alert or secondary.
# To change colors see sass/_01_settings_colors.scss
#
#callforaction:
#  url: https://tinyletter.com/feeling-responsive
#  text: Inform me about new updates and features ›
#  style: alert

permalink: /index.html
---
welle.io is an open source DAB and DAB+ software defined radio (SDR) with support for rtl-sdr (RTL2832U) and airspy. It supports high DPI and touch displays and it runs even on cheap computers like Raspberry Pi 2/3 and 100€ China Windows 10 tablets.

![welle-io_standard_mode.png](images/welle-io_standard_mode.png)

Main Features
---
* DAB and DAB+
* Windows 10, Linux, macOS, Android
* Runs on small devices like Raspberry Pi 2/3 or mobile phones
* [Airspy R2, Airspy Mini](/devices/airspy), [rtl-sdr (RTL2832U)](/devices/rtl_sdr), [rtl_tcp](/devices/rtl_tcp), [SoapySDR](/devices/soapysdr) and [rawfile](/devices/rawfile) support
* Touch optimized GUI
* Channel scan
* Slide show (MOT slide show)
* Radio text (dynamic label)

Demo Videos
---
 * Running under Windows 10: [https://youtu.be/NvKreB6w_H8](https://youtu.be/NvKreB6w_H8)
 * Running under Android: [https://youtu.be/3qhwzORfq7k](https://youtu.be/3qhwzORfq7k)

Expert Mode
---
![welle-io_expert_mode.png](images/welle-io_expert_mode.png)

Download
---
##### Stable binaries
* [welle.io for **Windows**, **Linux** and **Android (APK)**](http://github.com/AlbrechtL/welle.io/releases) 
* [welle.io for **Android at Google Play**](https://play.google.com/store/apps/details?id=io.welle.welle)
* welle.io for **macOS**
  * [through **MacPorts port**](https://ports.macports.org/port/welle.io/summary) (requires [MacPorts](https://www.macports.org/)) 
    * `sudo port install welle.io`

If you discovered an issue please open a new [issue](https://github.com/AlbrechtL/welle.io/issues).

##### Developer version
welle.io is under heavy development. You can also try the latest developer builds. But PLEASE BE WARNED the builds are automatically created and untested.
 * [welle.io nightly builds for *Windows* & *Linux AppImage*](https://bintray.com/albrechtl/welle.io/welle.io_nightly#files)
 * welle.io devel builds on *macOS MacPorts* are updated perdiodically manually and can be installed through [port welle.io-devel](https://ports.macports.org/port/welle.io-devel/summary). The port has no maintainer so please feel free to update it yourself in case you need to use a more recent devel version.
   * `sudo port install welle.io-devel`

##### Compilation from source
To use it on a **Raspberry Pi** you have to compile welle.io directly from the sources. See [instructions to compile for Raspberry Pi](https://github.com/AlbrechtL/welle.io#raspberry-pi-2-and-3).  
Note that welle.io source also ships with a [**Homebrew**](https://brew.sh/) formula for macOS. See [instructions to install with Homebrew](https://github.com/AlbrechtL/welle.io#homebrew).  
In the welle.io [sources repository](https://github.com/AlbrechtL/welle.io) you can find [more instructions to compile welle.io on various systems](https://github.com/AlbrechtL/welle.io#building).

Support
---
### Forum

If you need help don't hesitate and open a [new issue](https://github.com/AlbrechtL/welle.io/issues) on Github.

### FAQ
Visit the [FAQ list](/faq).

Development
----------
**Any contributions and pull requests are welcome!**  
Please take a look into the [task list](https://github.com/AlbrechtL/welle.io/wiki/Open-Tasks) and the [issues](https://github.com/AlbrechtL/welle.io/issues). But also any new ideas are welcome!

### Sources
Please visit the GitHub repository: [https://github.com/albrechtl/welle.io](https://github.com/albrechtl/welle.io)

### Wiki
You will find more details inside the [Wiki](http://github.com/AlbrechtL/welle.io/wiki) (under construction).


