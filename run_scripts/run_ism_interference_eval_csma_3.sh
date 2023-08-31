opp_runall -j6 src/tsch -u Cmdenv -c ReSA_Blacklisted_ISM_3_CSMA -r '$activeUserRatio == 0.4 && $repetition < 24' -n simulations:src:${HOME}/omnetpp-5.6.2/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2/samples/inet4/src -f simulations/omnetpp.ini

