This is tested with Ubuntu18.  For CENTOS, official docyments are enough for JAPANESE.

sudo apt install omniorb omniidl omniorb-nameserver libomniorb4-dev libxalan-c-dev libtool-bin uuid-dev autogen libboost-all-dev bc libxml2-utils libxml2-dev xinetd emacs git cmake-qt-gui doxygen automake swig dpkg-dev g++ gcc binutils libx11-dev libxpm-dev libxft-dev libxext-dev python libssl-dev apache2* libapache2-mod-wsgi

Install following
OpenRTM
DAQ-Middleware
ROOT
MongoDB drivers (MongoC and MongoCXX)

Edit config file
/etc/ld.so.conf.d/daqmw.conf
Adding following lines
/usr/local/lib
/somewhere/ROOT/lib

Edit /etc/services
Adding following
bootComps       50000/tcp                       # boot Comps

Adding bootComps
sudo cp /usr/share/daqmw/etc/remote-boot/bootComps-xinetd.sample /etc/xinetd.d/bootComps

In the bootComps file, CHange the user name.  Default is daq.
daq -> hpge

Usually, config file and output files are at /DAQ
mkdir /DAQ
chmod 777 /DAQ
input and output are at the same place...  Stupid?

Set up Apache2
Enable headers and CORS
sudo a2enmod headers
Header set Access-Control-Allow-Origin "*"

Edit and set up daqmw web pages
In Ubuntu, the default setting of web pages are at
/var/www/html
not
/var/www
Place the scripts at the right place
sudo cp -a /var/www/daqmw /var/www/html/
Edit config files
/etc/apache2/onf.d/daq.conf
Change the location of files (www -> www/html)

/etc/apache2/sites-available/000-default.conf
/etc/apache2/sites-enabled/000-default.conf
WSGIScriptAlias /daqmw/scripts "/var/www/html/daqmw/scripts"
WSGIPythonPath  /var/www/html/daqmw/scripts
