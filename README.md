# TSCH

OMNeT++ simulation model for IEEE 802.15.4e Time Slotted Channel Hopping (TSCH) with 6P protocol and scheduling functions

## Compatibility

This model is developed and tested with the following library versions

*  OMNeT++ [5.7.X](https://omnetpp.org/download/)
*  INET [4.2.X](https://github.com/inet-framework/inet/releases/download/v4.2.10/inet-4.2.10-src.tgz)

## Installation
1. Add INET to project references by navigating TSCH project Properties -> Project References
2. Replace files in the INET src using the ones from "inet" folder of this repo, strictly following the folder structure.
3. Add [RPL project](https://github.com/ComNetsHH/omnetpp-rpl) source directory to compile options of TSCH project by navigating Properties -> OMNeT++ -> Makemake -> (select "src" folder) -> Build Makemake Options... -> Compile -> add absolute path containing RPL "src" folder, e.g. "/home/yevhenii/omnetpp-5.7/samples/omnetpp-rpl/src". Also make sure _Add include paths exported from referenced projects_ is enabled
4. In RPL project Properties -> OMNeT++ -> Makemake -> (select "src" folder) -> Build Makemake Options...: 
   - Set Target to Shared library and enable "Export this shared library..."
