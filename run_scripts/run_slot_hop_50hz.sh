common_params='$nbChannels =='"$1"' && $N == 1 && $l == 1'

configs=( NoAlt 50Hz_Both 50Hz_CH 50Hz_SH 50Hz_None )
for i in "${configs[@]}"
do
	opp_runall -j7 src/tsch -u Cmdenv -c "SlotHopping_${i}" -r "$common_params" -n simulations:src:${HOME}/omnetpp-5.6.2-new/samples/omnetpp-rpl/src:${HOME}/omnetpp-5.6.2-new/samples/inet4/src -f simulations/omnetpp.ini
done
