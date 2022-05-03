# first remove the previous file if it exists, cause otherwise the contents will be just appended
rm "$1"
scavetool export -f 'module(**am1[*].app[*]) AND name(endToEndDelay:vector)' -o "$1" -F JSON ../simulations/wireless/waic/ReSA/*.vec