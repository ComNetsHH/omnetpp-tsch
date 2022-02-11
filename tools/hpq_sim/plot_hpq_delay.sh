echo "plotting HPQ delays for ${1} nodes and ${2} sinks"
cd ../../simulations/wireless/waic
if [[ -f "hpq_$1_$2_delay.json" ]] && [[ "$3" != "-o" ]]; then
  python3 ../../../tools/parser.py hpq_$1_$2_delay.json --delay $3
else
  scavetool export -f "itervar:numHosts(${1}) AND itervar:numSinks(${2}) AND (module(**sink[*].app[*]) AND name(endToEndDelay:vector))" -o hpq_$1_$2_delay.json -F JSON ./HPQ/*hosts=$1*.vec
  python3 ../../../tools/parser.py hpq_$1_$2_delay.json --delay $4
fi   
cd ../../../tools/hpq_sim





