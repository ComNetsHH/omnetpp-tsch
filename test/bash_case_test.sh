echo "input argument - $1"

case $1 in

  --clx-up)
    echo -n "6TiSCH-CLX (up)"
    ;;

  --clx-down)
    echo -n "6TiSCH-CLX (down)"
    ;;

  --hpq)
    echo -n "6TiSCH-HPQ"
    ;;

  *)
    echo -n "unknown"
    ;;
esac