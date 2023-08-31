opp_runall -j1 src/tsch -u Cmdenv -c ITNAC_case_2_tsch_bl -r '$repetition == 1 && $activeUserRatio == 0.03' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini --vector-recording=true

