package tsch.linklayer.ieee802154e.sixtisch;
import tsch.linklayer.ieee802154e.sixtisch.TschSF;

simple TschMSF like TschSF
{
    parameters:
        @class(TschMSF);
        
        // the time after which an active, unfinished transaction expires
        // (needs to be defined by every SF as per the 6P standard)
        int timeout = default(20000); // was 25s
        int initNumcells = default(5); // redundant parameter, required by SF interface
        int maxRetryBackoff @unit("ms") = default(60000ms); // redundant parameter, required by SF interface
        int KA_PERIOD @unit("s") = default(60s);
        int numMinCells = default(5);
        double upperCellUsageLimit = default(0.75);
        double lowerCellUsageLimit = default(0.25);
        int HOUSEKEEPINGCOLLISION_PERIOD @unit("s") = default(60s);
        double RELOCATE_PDRTHRES = default(0.50);
        double clearReservedCellsTimeout = default(15);
        int QUARANTINE_DURATION @unit("s") = default(300s);
        int WAITDURATION_MIN @unit("s") = default(30s);
        int WAITDURATION_MAX @unit("s") = default(60s);
        int maxNumCells = default(intuniform(4, 10));
        int MAX_NUMTX = default(254);
        int cellListRedundancy = default(5); // number of additional cells to include in 6P ADD/RELOCATE proposals
        // Custom, non-standard parameter defining number of cells to add while adapting to traffic
        int cellBandwidthIncrement = default(1); 
        bool disable = default(false);
        bool disableHousekeeping = default(false); // cell performance monitoring on/off 
        bool allowCellRemoval = default(true);

		double startTime @unit("s") = default(uniform(0s,10s));
}