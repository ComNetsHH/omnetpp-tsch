echo "plotting HPQ PDR for ${1} nodes and ${2} sinks"
cd ../../simulations/wireless/waic

if [[ -f "hpq_${1}_${2}_pdr.json" ]] && [[ "$3" != "-o" ]]; then
  python3 ../../../tools/parser.py hpq_$1_$2_pdr.json --pdr $3
else
  scavetool export -f "itervar:numSinks(${2}) AND ((name(packetReceived:count) AND module(*.sink[*].app[*])) OR (name(packetSent:count) AND module(*.host[*].app[*])))" -o hpq_$1_$2_pdr.json -F JSON ./HPQ/*hosts=$1*.sca
  python3 ../../../tools/parser.py hpq_$1_$2_pdr.json --pdr $4
fi
cd ../../../tools/hpq_sim
