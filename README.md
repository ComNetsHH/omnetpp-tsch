# TSCH

OMNeT++ simulation model for IEEE 802.15.4e Time Slotted Channel Hopping (TSCH)

## Compatibility

This model is developed and tested with the following library versions

*  OMNeT++ [5.5.1](https://omnetpp.org/software/2019/05/31/omnet-5-5-released.html)
*  OMNeT++ [5.6](https://omnetpp.org/software/2020/01/13/omnet-5-6-released.html)
*  INET [4.2](https://github.com/inet-framework/inet/releases/download/v4.2.0/inet-4.2.0-src.tgz)

**Known error in Windows with INET 4.2**: the commit [6ac2b9a](https://github.com/inet-framework/inet/commit/6ac2b9af073b308bf2ba58e3c4da50dd2e3e30b4) from the INET master branch has to be patched to INET 4.2 to avoid a compile error.

## 6TiSCH
In order to use 6TiSCH version: 
1. Add RPL project source directory to compile options of TSCH project by navigating 'Properties'->'OMNeT++'->Makemake-> (select 'src' folder) -> 'Build Makemake Options...' -> 'Compile' tab -> add absolute path containing RPL 'src' folder, e.g. '/home/yevhenii/omnetpp-5.6.2/samples/omnetpp-cross-layer-6tisch/rpl/src'. Make sure 'Add include paths exported from referenced projects' enabled
2. Make sure for RPL 'Properties'->'OMNeT++'->Makemake-> (select 'src' folder) -> 'Build Makemake Options...': 
   - 'Target' set to 'Shared library' and 'Export this shared library...' is enabled
   - Under 'Compile', 'Export include paths for other projects' and 'Add include paths exported from referenced projects' are set 



