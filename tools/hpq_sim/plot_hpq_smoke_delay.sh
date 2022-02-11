echo "plotting HPQ smoke delay for ${1} nodes and variable number of sinks"
cd ../../simulations/wireless/waic
if [[ -f "hpq_$1_smoke_delay.json" ]] && [[ "$2" != "-o" ]]; then
  python3 ../../../tools/parser.py hpq_$1_smoke_delay.json --delay-smoke $2
else
  scavetool export -f "itervar:numHosts(${1}) AND (module(**sink[*].app[1]) AND name(endToEndDelay:vector))" -o hpq_$1_smoke_delay.json -F JSON ./HPQ/*.vec
  python3 ../../../tools/parser.py hpq_$1_smoke_delay.json --delay-smoke $3
fi
cd ../../../tools/hpq_sim





