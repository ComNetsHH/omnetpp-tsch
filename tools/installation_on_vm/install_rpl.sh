git clone git@collaborating.tuhh.de:e-4/Research/omnetpp-rpl.git
cd omnetpp-rpl
source replace_inet_files.sh ../inet4
cd ../inet4
source install_inet.sh
cd ../omnetpp-rpl/src
opp_makemake -f --deep --make-so -DINET_IMPORT -I. -I../../inet4/src  -L../../inet4/src  -lINET$\(D\) --mode release
cd ../../
