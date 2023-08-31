opp_runall -j7 src/tsch -u Cmdenv -c RandomTopology -r '$topid == 20 && $l == 0.5 && $N == 1 && $repetition > 475' -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/wireless/waic/omnetpp.ini

