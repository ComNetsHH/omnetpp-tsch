# First ensure there'll only be results from this test for analysis
RESULT_DIR=~/omnetpp-5.6.2-new/samples/tsch/simulations/wireless/waic/ReSA

if [ -d "$RESULT_DIR" ]; then
    rm -r "$RESULT_DIR"
    echo "cleared result folder"
else
    echo "no result folder found"
fi

TEST_CONFIG=""

case $1 in

  --clx-up)
    TEST_CONFIG="ReSA_Low_Latency"
    ;;

  --clx-down)
    TEST_CONFIG="ReSA_Downlink_Optimization"
    ;;

  --hpq)
  	TEST_CONFIG="ReSA_HPQ"
    ;;
    
  --ultimate)
    TEST_CONFIG="ReSA_Ultimate_Altimeter"
    ;;
    
  --ultimate-no-alt)
    TEST_CONFIG="ReSA_Ultimate_Dynamic_Cell_Bundling"
    ;;
    
  --ultimate-no-alt-2)
    TEST_CONFIG="ReSA_Ultimate_Dynamic_Cell_Bundling_2"
    ;;
    
  --ultimate-2)
    TEST_CONFIG="ReSA_Ultimate_Altimeter_2"
    ;;

  *)
    echo -n "unknown config requested"
    exit 1
    ;;
esac

opp_runall -j7 ../src/tsch -u Cmdenv -c "$TEST_CONFIG" -n ../simulations:../src:${HOME}/omnetpp-5.6.2/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2/samples/inet4/src -f ../simulations/wireless/waic/omnetpp.ini
opp_runall -j7 ../src/tsch -u Cmdenv -c ReSA -n ../simulations:../src:${HOME}/omnetpp-5.6.2/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2/samples/inet4/src -f ../simulations/wireless/waic/omnetpp.ini

