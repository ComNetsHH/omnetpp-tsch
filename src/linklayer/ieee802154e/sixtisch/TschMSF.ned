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
        int cellListRedundancy = default(3); // number of additional cells to include in 6P ADD/RELOCATE proposals
        int cellBandwidthIncrement = default(1); // number of cells to add with neighbor if cell usage is excessive
        bool disable = default(false);
        // determines whether auto TX cells to neighbors are scheduled on demand (per transmission), or globally during initialization stage
        bool autoCellOnDemand = default(true); // WARNING: may lead to cell collisions (adding AUTO cell on top of dedicated cell)  
        bool disableHousekeeping = default(false);
        bool showDedicatedTxCells = default(true); // show scheduled TX cells coordinates above node pictorgram
    	bool showTxCellCount = default(false); // instead of all TX cells locations, show only their count
        bool showQueueUtilization = default(false); // displays queue utilization for preferred parent (if available)
        bool allowMinimalCellsOverride = default(false); 
        
     	bool handleRankUpdates = default(false); // TEST parameter, only needed for delay-testing scenarios 

		double startTime @unit("s") = default(uniform(0s, 10s));
}