#opp_runall -j7 src/tsch -u Cmdenv -c ReSA_Blacklisted_ISM_1 -r '$repetition < 21 && $activeUserRatio == 0.4 && $r == 3' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini

#opp_runall -j7 src/tsch -u Cmdenv -c ReSA_Blacklisted_ISM_1_CSMA -r '$repetition < 21 && $activeUserRatio == 0.4 && $r == 3' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini

opp_runall -j7 src/tsch -u Cmdenv -c ReSA_Blacklisted_ISM_3 -r '$repetition < 21 && $activeUserRatio == 0.4 && $r == 3' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini

opp_runall -j7 src/tsch -u Cmdenv -c ReSA_Blacklisted_ISM_3_CSMA -r '$repetition < 21 && $activeUserRatio == 0.4 && $r == 3' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini