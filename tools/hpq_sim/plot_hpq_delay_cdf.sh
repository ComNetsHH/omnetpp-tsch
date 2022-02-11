echo "plotting HPQ delay CDF of smoke alarm for ${1} nodes and ${2} sinks, additional arguments: ${3} ${4}"
cd ../../simulations/wireless/waic
if [[ -f "hpq_$1_$2_delay.json" ]] && [[ "$3" != "-o" ]]; then
  python3 ../../../tools/parser.py hpq_$1_$2_delay.json --cdf $3
else
  scavetool export -f "itervar:numSinks(${2}) AND (module(**sink[*].app[1]) AND name(endToEndDelay:vector))" -o hpq_$1_$2_delay.json -F JSON ./HPQ/*.vec
  python3 ../../../tools/parser.py hpq_$1_$2_delay.json --cdf $4
fi   
cd ../../../tools/hpq_sim





