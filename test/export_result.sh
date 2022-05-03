
# export result files into a JSON,
# $1 - filename of the resulting JSON

# first remove the previous file if it exists, cause otherwise the contents will be just appended
if test -f "$1"; then
    rm "$1"
fi

scavetool export -f 'module(**am1[*].app[*]) AND name(endToEndDelay:vector)' -o 'test_data.json' -F JSON ../simulations/wireless/waic/ReSA/*.vec