common_params='$nbChannels == 40 && $N == 1 && $l == 1' # number of channels doesn't matter here, just to avoid redundant runs

configs=( 50 100 200 )
for i in "${configs[@]}"
do
	opp_runall -j7 src/tsch -u Cmdenv -c "SlotHopping_Housekeeping_${i}Hz" -r "$common_params" -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini
done
