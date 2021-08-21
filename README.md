# TSCH

OMNeT++ simulation model for IEEE 802.15.4e Time Slotted Channel Hopping (TSCH) with 6P protocol and scheduling functions

## Compatibility

This model is developed and tested with the following library versions

*  OMNeT++ [5.6.X](https://omnetpp.org/download/)
*  INET [4.2.X](https://github.com/inet-framework/inet/releases/download/v4.2.5/inet-4.2.5-src.tgz)

## Installation
1. Add INET to project references by navigating TSCH project 'Properties' -> 'Project References'
2. Add RPL project source directory to compile options of TSCH project by navigating `Properties` -> `OMNeT++` -> `Makemake` -> (select `src` folder) -> `Build Makemake Options...` -> `Compile` tab -> add absolute path containing RPL 'src' folder, e.g. '/home/yevhenii/omnetpp-5.6.2/samples/omnetpp-rpl/src'. Also make sure `Add include paths exported from referenced projects` is enabled
3. In RPL `Properties`->`OMNeT++`-> `Makemake`-> (select `src` folder) -> `Build Makemake Options...`: 
   - Set `Target` to `Shared library` and enable `Export this shared library...`


