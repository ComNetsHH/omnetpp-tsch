cd omnetpp-5.6.2/samples
git clone git@collaborating.tuhh.de:e-4/Research/tsch.git
cd tsch
git checkout 6tisch
cd tools
source replace_inet_files.sh ../../inet4
cd ../../inet4
make MODE=release
cd ../tsch/src
touch makefrag
echo 'MSGC:=$(MSGC) --msg6' > makefrag
opp_makemake -f --deep -DINET_IMPORT -I. -I../../inet4/src -I../../omnetpp-rpl/src -I../../omnetpp-rpl  -L../../inet4/src -L../../omnetpp-rpl/src  -lINET$\(D\) -lomnetpp-rpl$\(D\) -Xlinklayer/ieee802154e/sixtisch/clx -Xlinklayer/ieee802154e/sixtisch/blacklisting --mode release
make MODE=release

