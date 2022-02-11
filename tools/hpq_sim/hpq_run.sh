NUM_HOSTS='$numHosts =='
NUM_SINKS='$numSinks =='
QUERY_STR="$NUM_HOSTS $1 && $NUM_SINKS $2"
echo "$QUERY_STR"

if [ "$3" = '--on' ]; then
    opp_runall -j7 ../../src/tsch -u Cmdenv -c HPQ -r ${QUERY_STR} -n ../../simulations:../../src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f ../../simulations/wireless/waic/omnetpp.ini 
fi
if [ "$3" = '--off' ]; then
    opp_runall -j7 ../../src/tsch -u Cmdenv -c HPQ_Off -r ${QUERY_STR} -n ../../simulations:../../src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f ../../simulations/wireless/waic/omnetpp.ini 
fi
if [ "$3" = '--all' ]; then
    opp_runall -j7 ../../src/tsch -u Cmdenv -c HPQ_Off -r ${QUERY_STR} -n ../../simulations:../../src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f ../../simulations/wireless/waic/omnetpp.ini 
    opp_runall -j7 ../../src/tsch -u Cmdenv -c HPQ -r ${QUERY_STR} -n ../../simulations:../../src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f ../../simulations/wireless/waic/omnetpp.ini 
fi

