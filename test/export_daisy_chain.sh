scavetool export -f 'module(**am1[*].app[*]) AND name(endToEndDelay:vector)' -o "$1" -F JSON ../simulations/wireless/waic/ReSA/*.vec