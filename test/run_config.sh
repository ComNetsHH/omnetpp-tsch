TEST_CONFIG=""

case $1 in

  --clx-up)
    TEST_CONFIG="ReSA_Low_Latency"
    ;;

  --clx-down)
    TEST_CONFIG="ReSA_Downlink_Optimization"
    ;;

  --hpq)
    echo -n "ReSA_HPQ"
    ;;

  *)
    echo -n "unknown test config requested"
    exit 1
    ;;
esac

opp_runall -j7 ../src/tsch -u Cmdenv -c "$TEST_CONFIG" -r '$repetition < 2' -n ../simulations:../src:${HOME}/omnetpp-5.6.2/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2/samples/inet4/src -f ../simulations/wireless/waic/omnetpp.ini
opp_runall -j7 ../src/tsch -u Cmdenv -c ReSA -r '$repetition < 2' -n ../simulations:../src:${HOME}/omnetpp-5.6.2/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2/samples/inet4/src -f ../simulations/wireless/waic/omnetpp.ini

