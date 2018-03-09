# dzEvo

Honeywell Evohome companion app for Domoticz

## Description

This project extends controllability of Evohome through the Domoticz API


## Implementation

With the completion of the Honeywell Evohome API library this project now also features a full blown Evohome client for Domoticz, offering the same functionality as the python scripts on the Domoticz wiki and more. And most important: with a significant increased performance.

To build dzEvo yourself simply run ` make ` from the project main folder. You can change various default values by making use of the EXTRAFLAGS parameter, e.g.

    make EXTRAFLAGS=' -DCONFIGFILE=\"dzEvo.conf\" -DHARDWAREFILTER=\"Evohome\" '

These are actually the default values if you are not on Windows. File locations that do not start with a root (` / `) reference ( windows: ` <drive>: ` ) are relative to the application rather than whatever may be the current directory.


## Prerequisites for building dzEvo yourself

A computer running Domoticz of course! If you compile your own beta versions of Domoticz you should be all set already for building this client. If not, you'll need a C++ compiler and supporting developer library curl with ssl support. For Debian based systems such as Raspbian this should get you going:

    apt-get install build-essential
    apt-get install libcurl4-openssl-dev


## Running dzEvo

You need to fill in some details in dzEvo.conf. A sample file with instructions is included in the source and binary archives.

1. update url and port if applicable (e.g. different port, different server, different webroot, user credentials)

Run ` dzEvo --init ` to add your evohome installation to Domoticz (if you migrated from the python scripts you can skip this step). The client's default action, i.e. when run without parameters, is to update Domoticz with the current status values of your evohome installation. Note that evohome schedules are cached, so if you change those on the controller you will need to refresh them in the client to have Domoticz show the correct until time.<br>

Run ` dzEvo help ` to view more options.

## Feedback Welcome!

If you have any problems, questions or comments regarding this project, feel free to contact me! (gordon@bosvangennip.nl)

