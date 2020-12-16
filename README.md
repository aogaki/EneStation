# EneStation
DAQ-Middleware components for the energy monitor.  The Bold and Italic style lines are the commands on the command prompt.  The user is assumed to use hpge.  If not, you need to modify the setting files, when you install.  
  
## Installation
See Install.md  

## Running
Going to the working directory  
***cd /home/hpge/DAQ/EneStation***

### Components  
Components are started up by DAQ-Middleware controller.  If needed, compile all components under EneStation.  
* Reader  
Data taking from a CAEN digitizer (DT5730).  Sending data to upstream.  This component can filter the data.  But now, the component only chop meaning less information from the digitizer.  
* Emulator  
Sending data to upstream, but the data is from the past measuring data.  Using ROOT library to generate the data.  For testing.  
* Dispatcher  
Receive data from down stream and send to upstream.  Receive (input) is one port, Transfer (output) is two ports.  
* Monitor  
The main aim of this component is fitting the photo peak and upload the result to a database.  This should be called Online analyzer or something.  But you can use this as the monitor.  Monitoring uses THttpServer of ROOT.  
* Recorder  
Save the data as a ROOT file.  Every one hour, the data is saved.  The storage place is local HDD now (/DAQ/Output/).  

Start up all components  
***run.py -l test.xml***

Open the controller [web page](http://172.18.4.109/controller)(http://172.18.4.109/controller).  [The controller page](http://172.18.4.109/controller) can be accessed only from ELI-NP network (Not Guest network).  If you can see the text box to input the IP address, please check the IP address of EneStation PC and check the log files of components.  All log files are at **/tmp/daqmw/**.  

First, press the **Config** button.  The Reader component will read the config file for the CAEN digitizer **/DAQ/PHA.conf**.  

Next, set the run number.  The event data is stored as ROOT file format at **/DAQ/Output** with file name "runXXXX_YY.root (XXXX is the run number, YY is the sub run number.  The output file is saved each one hour with incrementing the sub runnumber.)".  If there is the same run number file, the Recorder component will save the file with UNIX time stamp, not overwrite.  
Press **Start** button to start the data acquisition.  The raw data from the CAEN digitizer are shown in the monitor [page](http://172.18.4.109:8080/)(http://172.18.4.109:8080/).  The monitor page is constructed by THttpServer of ROOT.  

The fitting results are uploaded every one minutes (You can set the time interval in test.xml file).  The results are shown in the [dashboard page](http://172.18.4.56/dashboard).  The dashboard page will be modified simpler soon.  Now the page includes unnecessasry functions.  
