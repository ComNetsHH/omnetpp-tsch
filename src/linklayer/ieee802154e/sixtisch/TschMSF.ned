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
        
        int numMinCells = default(5); // number of minimal cells (broadcast / control traffic) to schedule
        double upperCellUsageLimit = default(0.75);
        double lowerCellUsageLimit = default(0.25); 
        int housekeepingPeriod @unit("s") = default(60s); // how often should we check cell PDR
        double relocatePdrThresh = default(0.50); // if cell exhibits PDR worse than this, relocate it!
        int maxNumTx = default(254); // maximum number of elapsed cells to consider for PDR calculation
        int maxNumCells = default(intuniform(4, 10)); // sliding window size to esmitate cell utilization
        int cellListRedundancy = default(3); // number of additional cells to pad CELL_LIST in 6P transcations
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
		
		// custom parameter to add random delay before 6P requests to minimize
		// probability of simulatenous transactions in both directions between two nodes
		bool send6pDelayed = default(false);  
		
		// Module parameters required by the interface but is actually unused
		
		int initNumcells = default(6);
		int maxRetryBackoff @unit(ms) = default(60000ms);
}