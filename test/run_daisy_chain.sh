opp_runall -j7 ../src/tsch -u Cmdenv -c ReSA -r '$repetition < 2' -n ../simulations:../src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f ../simulations/wireless/waic/omnetpp.ini
opp_runall -j7 ../src/tsch -u Cmdenv -c ReSA_Low_Latency -r '$repetition < 2' -n ../simulations:../src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f ../simulations/wireless/waic/omnetpp.ini