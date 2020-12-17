# Install DAQ system

This document is the installation log for Ubuntu 18.04 PC.  For Ubuntu 16 or 20, The procedures are almost same as the following.  Main difference is package name for ***"apt install"*** command.  Sometimes, the apt command suggest the right package name.  And also you can google to find the right package name.  This installation is based on the information on [DAQ-Middleware](https://daqmw.kek.jp/) and [ROOT](https://root.cern.ch/) web pages.
At this moment, the system was tested on Ubuntu 18.04, Ubuntu 16.04, Ubuntu 20.04, and CentOS 8.  Ubuntu is the one of the famous distributions in general, CentOS is also very common distribution for nuclear, particle, high-energy physics people.

## Install necessary software
### From package manager
On the terminal, type following.  
***sudo apt install omniorb omniidl omniorb-nameserver libomniorb4-dev libxalan-c-dev libtool-bin uuid-dev autogen libboost-all-dev bc libxml2-utils libxml2-dev xinetd emacs git cmake-qt-gui doxygen automake swig dpkg-dev g++ gcc binutils libx11-dev libxpm-dev libxft-dev libxext-dev python libssl-dev apache2\* libapache2-mod-wsgi***  
These packages are listed by [Dependencies page of ROOT](https://root.cern/install/dependencies/) and [DAQ-Middleware intallation for Raspberry Pi](https://daqmw.kek.jp/raspberrypi/DAQ-MWonRasp4b-rep.txt).  Almost all of DAQ-Middleware page is written in Japanese, please try to use Chrome or some web browsers with translation. Probably you will need python2-dev for Ubuntu20.   

### From source, manual instalation
Install following software
* OpenRTM  
The version 1.2.2 (The latest version at December 2020) is sued.  You can find the install script for Ubuntu at [Release note](https://www.openrtm.org/openrtm/en/download/openrtm-aist-cpp/openrtm-aist-cpp_1_2_2_release).  
From source, you need to do following.  
***wget https://github.com/OpenRTM/OpenRTM-aist/releases/download/v1.2.2/OpenRTM-aist-1.2.2.tar.gz***  
***tar zxvf OpenRTM-aist-1.2.2.tar.gz***  
***cd OpenRTM-aist-1.2.2***  
***./configure***  
***make -j12*** (12 means using 12 threads to compile)  
***sudo make install***

* DAQ-Middleware  
For CentOS, you can find the source code from [GitHub](https://github.com/h-sendai/DAQ-Middleware-CentOS8).  For Ubuntu, You can download from [KEK](https://daqmw.kek.jp/src/), DAQ-Middleware-1.4.4.tar.gz is the latest version in December 2020.  This installation shows how to install for Ubuntu 18.04.  
***wget https://daqmw.kek.jp/src/DAQ-Middleware-1.4.4.tar.gz***  
***tar zxvf DAQ-Middleware-1.4.4.tar.gz***  
***cd DAQ-Middleware-1.4.4***  
***make*** (If you compile with option -j, you will get a trouble.).  
***sudo make install***  

* ROOT  
You can install by [some ways](https://root.cern/install/).  Here, I describe the installation from source.  Also I use not release version.  In the case of you want the stable version, please download the source code as same as the [ROOT installation page](https://root.cern/install/build_from_source/).  Also I install ROOT at /opt/ROOT like a old style CERN software.  
***git clone https://github.com/root-project/root.git***  
***mkdir build_root***  
***cd build_root***  
***cmake -DCMAKE_INSTALL_PREFIX=/opt/ROOT ../root***  
***make -j12*** (If you will get the error messages, try other stable version of ROOT.)  
***sudo mkdir -p /opt/ROOT***  
***sudo make install***  
After installation, add following line into ~/.bashrc.  Not command.  
**source /opt/ROOT/bin/thisroot.sh**  
***source ~/.bashrc*** (This should be on terminal, command.)  

* MongoDB drivers (MongoC and MongoCXX)  
These are needed to read and write the Mongo DB for results upload.  
  * Mongo C driver  
  I used the latest release version.  
  ***wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.3/mongo-c-driver-1.17.3.tar.gz***  
  ***tar zxvf mongo-c-driver-1.17.3.tar.gz***  
  ***cd mongo-c-driver-1.17.3***  
  ***cmake .***  
  ***make -j12***  
  ***sudo make install***  
  
  * Mongo C++ driver  
  I download from [the developper page](https://github.com/mongodb/mongo-cxx-driver/releases).  Check the installation [page](http://mongocxx.org/mongocxx-v3/installation/linux/).  
  ***wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.6.2/mongo-cxx-driver-r3.6.2.tar.gz***  
  ***tar zxvf mongo-cxx-driver-r3.6.2.tar.gz***  
  ***cd mongo-cxx-driver-r3.6.2***  
  ***cmake . -DCMAKE_INSTALL_PREFIX=/usr/local -DBSONCXX_POLY_USE_MNMLSTC=1***  
  ***sudo make*** (Some files will be downloaded and stored at INSTALL_PREFIX,/usr/local.  We need sudo to access there.)  
  ***sudo make install***  
  
## Preparing the running env  
* Making directory for input and output  
***sudo mkdir /DAQ***  
***sudo mkdir /DAQ/Output***  
***sudo chmod 777 /DAQ -R***  
input and output are at the same place...  Stupid?  

* Edit system library path.  Usually, /usr/local/lib is not included in default.   
**/etc/ld.so.conf.d/daqmw.conf**  
Adding following lines  
**/usr/local/lib**  
**/opt/ROOT/lib**  
And type ldconfig command.  
***sudo ldconfig***  

* Adding network component controll  
**/etc/services**  
Adding following  
**bootComps       50000/tcp                       # boot Comps**  
Adding bootComps into the system  
***sudo cp /usr/share/daqmw/etc/remote-boot/bootComps-xinetd.sample /etc/xinetd.d/bootComps***  
In the bootComps file, Change the user name.  Default is daq.  
**/etc/xinetd.d/bootComps**  
**daq -> hpge**  

* Set up Apache2 HTTP server  
Enable headers and CORS. This is needed for remote monitoring and controll.  
First, enable the header.  
***sudo a2enmod headers***  
Next, Editting the config file **etc/apache2/apache2.conf**.  Adding following line in the **<Directory /var/www/>** section.  
**Header set Access-Control-Allow-Origin "*"**  

* Edit and set up daqmw web script  
In Ubuntu, the default setting of web pages are at **/var/www/html** not **/var/www**.  Place the scripts at the right place.  
***sudo cp -a /var/www/daqmw /var/www/html/***  
Edit config files of HTTP server  
**/etc/apache2/onf.d/daq.conf**  
Change the location of script files (www -> www/html) as same as folloing two lines.   
**WSGIScriptAlias /daqmw/scripts "/var/www/html/daqmw/scripts"  
WSGIPythonPath  /var/www/html/daqmw/scripts**  
Also add above 2 lines into following config files.  
**/etc/apache2/sites-available/000-default.conf  
/etc/apache2/sites-enabled/000-default.conf**  
Restart the HTTP server  
