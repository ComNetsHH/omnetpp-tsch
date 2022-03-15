# TSCH

OMNeT++ simulation model for IEEE 802.15.4e Time Slotted Channel Hopping (TSCH) with 6P protocol and scheduling functions

## Compatibility

This model is developed and tested with the following library versions

*  OMNeT++ [5.6.X](https://omnetpp.org/download/)
*  INET [4.2.X](https://github.com/inet-framework/inet/releases/download/v4.2.5/inet-4.2.5-src.tgz)

## Installation
1. To use with 6TiSCH, first import RPL project from https://github.com/ComNetsHH/omnetpp-rpl, checkout `6tisch-clx` branch and follow respective installation instructions.
2. Add INET to project references by navigating project Properties -> Project References.
3. Add RPL project **source directory** to compile options by navigating Properties -> OMNeT++ -> Makemake -> (select `src` folder) -> Build Makemake Options... -> Compile tab -> add absolute path containing RPL `src` folder, e.g. '/home/yevhenii/omnetpp-5.6.2/samples/omnetpp-rpl/src'. Also make sure "Add include paths exported from referenced projects" is enabled.
4. In RPL Properties -> OMNeT++ -> Makemake -> (select `src` folder) -> Build Makemake Options...: 
   - Set Target to "Shared library" and enable "Export this shared library...".
5. Make sure the build mode is `release` by right-clicking on the project directory -> Build configurations -> Set active.


