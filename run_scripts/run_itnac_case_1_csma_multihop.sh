
Query='$repetition < 10 && $activeUserRatio'
opp_runall -j5 src/tsch -u Cmdenv -c ITNAC_case_2_csma_downlink -r '$repetition < 10 && $activeUserRatio < 0.06' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini
