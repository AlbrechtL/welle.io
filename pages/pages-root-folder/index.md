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
welle.io is an open source DAB and DAB+ software defined radio (SDR) with support for rtl-sdr (RTL2832U) and airspy. It supports high DPI and touch displays and it runs even on cheap computers like Raspberry Pi 2+ and 100€ China Windows 10 tablets.

![welle-io_standard_mode.png](images/welle-io_standard_mode.png)

Main Features
---
* DAB and DAB+
* Supports Windows 10/11, Linux, macOS and Android
* Runs on small devices like Raspberry Pi 2+ or mobile phones
* [Airspy R2, Airspy Mini](/devices/airspy), [rtl-sdr (RTL2832U)](/devices/rtl_sdr), [rtl_tcp](/devices/rtl_tcp), [SoapySDR](/devices/soapysdr) and [rawfile](/devices/rawfile) support
* Touch optimized GUI
* Channel scan
* Slide show (MOT slide show)
* Radio text (dynamic label)
* Expert mode with a lot of technical data

Demo Video
---
 * Showing welle.io: [https://youtu.be/IJcgdmud-AI](https://youtu.be/IJcgdmud-AI)

Expert Mode
---
![welle-io_expert_mode.png](images/welle-io_expert_mode.png)

Download
---
### Stable binaries
* ### [**Windows**, **Linux**, **macOS** and **Android**](http://github.com/AlbrechtL/welle.io/releases) 
* **Debian** or **Ubuntu** 19.04+
  * `apt install welle.io`
* **Fedora** 35+ (uses the [RPM Fusion](https://rpmfusion.org) package sources)
  * `sudo dnf install -y https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm` (to enable the RPM Fusion sources)
  * `sudo dnf install --refresh welle-io`
* **macOS** (requires [MacPorts](https://www.macports.org/)) 
   * `sudo port install welle.io`
   * `sudo port install welle.io +cli` (if you wish to install also welle-cli)
* **FreeBSD**
  * `pkg install welle.io`
* [**Android at Google Play**](https://play.google.com/store/apps/details?id=io.welle.welle) (outdated)

If you discovered an issue please open a new [issue](https://github.com/AlbrechtL/welle.io/issues).

### Unstable developer version
welle.io is under development. You can also try the latest developer builds. But PLEASE BE WARNED the builds are automatically created and untested.

* #### [welle.io nightly builds](https://welle-io-nightlies.albrechtloh.de/) (Windows, Linux, macOS, Android)

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


