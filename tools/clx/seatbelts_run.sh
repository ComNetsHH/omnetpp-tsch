opp_runall -j16 src/tsch-master -u Cmdenv -c SeatbeltStatus -r '$hosts == 200' -n simulations:src:${HOME}/omnetpp-5.6.2/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2/samples/inet4/src simulations/wireless/waic/omnetpp.ini