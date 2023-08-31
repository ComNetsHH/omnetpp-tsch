opp_runall -j6 src/tsch -u Cmdenv -c ITNAC_case_1_tsch -r '$repetition < 6 && $activeUserRatio == 0' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini --vector-recording=true

