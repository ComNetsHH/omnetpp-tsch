# TSCH

OMNeT++ simulation model for IEEE 802.15.4e Time Slotted Channel Hopping (TSCH)

## Compatibility

This model is developed and tested with the following library versions

*  OMNeT++ [5.5.1](https://omnetpp.org/software/2019/05/31/omnet-5-5-released.html)
*  OMNeT++ [5.6](https://omnetpp.org/software/2020/01/13/omnet-5-6-released.html)
*  INET [4.2](https://github.com/inet-framework/inet/releases/download/v4.2.0/inet-4.2.0-src.tgz)

**Known error in Windows with INET 4.2**: please note that `inet/common/ModuleAccess.h` does not compile. To fix this:
In Line 130-131# declare the function without defining it. 
Then define it in the next line without the INET_API type and without =0 in the parameters.
So the previous 130-131# line should look like this:
```
template<typename T>
INET_API cGate *findConnectedGate(cGate *gate, int direction = 0);
```
And the definition after that should look like this: 
```
template<typename T>
cGate *findConnectedGate(cGate *gate, int direction)
{...
}
```
The same change needs to be done to the following functions in this file:
```
template<typename T>
INET_API cGate *getConnectedGate(cGate *gate, int direction = 0);
```

```
template<typename T>
INET_API T *findConnectedModule(cGate *gate, int direction = 0);
```

```
template<typename T>
INET_API T *getConnectedModule(cGate *gate, int direction = 0);
```
