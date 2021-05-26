package tsch.linklayer.ieee802154e.sixtisch;
import tsch.linklayer.ieee802154e.sixtisch.TschSF;

simple TschMSF like TschSF
{
    parameters:
        @class(TschMSF);
        
        @signal[rplParentChanged](type=unsigned long);
        @statistic[rplParentChanged](title = "Number of rpl parents changed"; source="rplParentChanged"; record=count; interpolationmode=none);
        
        // the time after which an active, unfinished transaction expires
        // (needs to be defined by every SF as per the 6P standard)
        int timeout @unit("ms") = default(25000ms);

        // DEPRECATED the number of cells to be initially scheduled between all nodes, 
        int initNumcells = default(6);

        // The maximum time in ms that will pass until another initial ADD
        // request is sent if the last one wasn't successful
        int maxRetryBackoff @unit("ms") = default(60000ms);
        
        int NUM_CH_OFFSET = default(16);
        int KA_PERIOD @unit("s") = default(60s);
        int numMinCells = default(5);
        double upperCellUsageLimit = default(0.75);
        double lowerCellUsageLimit = default(0.25);
        int housekeepingPeriod @unit("s") = default(60s);
        double RELOCATE_PDRTHRES = default(0.50);
        double clearReservedCellsTimeout = default(50000);
//        int SLOTFRAME_LENGTH = default(101);
        int QUARANTINE_DURATION @unit("s") = default(300s);
        int WAITDURATION_MIN @unit("s") = default(30s);
        int WAITDURATION_MAX @unit("s") = default(60s);
        int maxNumCells = default(intuniform(4, 10));
        int MAX_NUMTX = default(254);
        int slotframeLen = default(101);
        int cellListRedundancy = default(3); // number of additional cells to pad CELL_LIST in 6P transcations
        int cellBandwidthIncrement = default(1); // number of cells to add with neighbor if cell usage is excessive
        bool disable = default(false);
        bool disableHousekeeping = default(true);
        bool showDedicatedTxCells = default(true); // show scheduled TX cells locations above node
        bool allowMinimalCellsOverride = default(false);    	 

		double startTime @unit("s") = default(uniform(0s, 10s));
}