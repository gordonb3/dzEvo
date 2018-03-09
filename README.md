# dzEvo

Honeywell Evohome companion app for Domoticz

## Description

This project extends controllability of Evohome through the Domoticz API


## Implementation

To build dzEvo simply run ` make ` from the project main folder. You can change various default values by making use of the EXTRAFLAGS parameter, e.g.

    make EXTRAFLAGS=' -DCONFIGFILE=\"dzEvo.conf\" -DHARDWAREFILTER=\"Evohome\" '

These are actually the default values if you are not on Windows. File locations that do not start with a root (` / `) reference ( windows: ` <drive>: ` ) are relative to the application rather than whatever may be the current directory.


## Prerequisites for building dzEvo

A computer running Domoticz of course! If you compile your own beta versions of Domoticz you should be all set already for building this client. If not, you'll need a C++ compiler and supporting developer library curl with ssl support. For Debian based systems such as Raspbian this should get you going:

    apt-get install build-essential
    apt-get install libcurl4-openssl-dev


## Running dzEvo

You need to fill in some details in dzEvo.conf. A sample file with instructions is included in the source and binary archives.

1. update host and port if applicable (e.g. different port, different server, different webroot, user credentials) or supply a fully qualified url to your Domoticz installation

Run ` dzEvo help ` for usage info

## Feedback Welcome!

If you have any problems, questions or comments regarding this project, feel free to contact me! (gordon@bosvangennip.nl)

