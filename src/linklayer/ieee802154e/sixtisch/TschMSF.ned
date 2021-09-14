package tsch.linklayer.ieee802154e.sixtisch;
import tsch.linklayer.ieee802154e.sixtisch.TschSF;

simple TschMSF like TschSF
{
    parameters:
        @class(TschMSF);
        
        @signal[rplParentChanged](type=unsigned long);
        @signal[queueUtilization](type=double);
        @signal[failed6pAdd];
        
        @statistic[rplParentChanged](title="Number of rpl parents changed"; source="rplParentChanged"; record=count; interpolationmode=none);
        @statistic[queueUtilization](title="Queue utilization"; source=queueUtilization; record=mean?, vector?; interpolationmode=none);
        @statistic[failed6pAdd](title="Failed 6P ADD requests"; source=failed6pAdd; record=count, vector?; interpolationmode=none);
        
        // the time after which an active, unfinished transaction expires
        // (needs to be defined by every SF as per the 6P standard)
        int timeout @unit("ms") = default(25000ms);
        
        int numMinCells = default(5); // number of minimal cells (broadcast / control traffic) to schedule
        double upperCellUsageLimit = default(0.75);
        double lowerCellUsageLimit = default(0.25);
        int housekeepingPeriod @unit("s") = default(60s); // how often should we check cell PDR 	 
        double relocatePdrThresh = default(0.50); // if cell exhibits PDR worse than this, relocate it!
        int maxNumCells = default(intuniform(4, 10)); // sliding window size to esmitate cell utilization
        int maxNumTx = default(254); // maximum number of elapsed cells to consider for PDR calculation
//        int slotframeLen = default(101);
        int cellListRedundancy = default(3); // number of additional cells to pad CELL_LIST in 6P transcations

        int cellsToAdd = default(1); // number of cells to add with neighbor if cell usage is excessive
        bool disable = default(false);
        // determines whether auto TX cells to neighbors are scheduled on demand (per transmission), or globally during initialization stage
        bool autoCellOnDemand = default(true); // WARNING: may lead to cell collisions (adding AUTO cell on top of dedicated cell)  
        bool disableHousekeeping = default(false);
        bool showDedicatedTxCells = default(true); // show scheduled TX cells coordinates above node pictorgram
    	bool showTxCellCount = default(false); // instead of displaying TX cells coordinates, show their count only
        bool showQueueUtilization = default(false); // displays queue utilization for preferred parent (if available)
        
     	bool handleRankUpdates = default(false); // TEST parameter, only needed for delay-testing scenarios 

		double startTime @unit("s") = default(uniform(1s, 2s));
		
		bool estimateQueueUtil = default(false); // use queue utilization instead of cell util
		double queueEstimationPeriod @unit("s") = default(uniform(10s, 20s));
		double queueUtilUpperThresh = default(0.95);
		
		int maxRetries = default(3); // 6P request retry threshold
		int initialNumCells = default(1); // number of cells to schedule with RPL preferred parent initially
		
		int trackFailed6pAddByNum = default(0); // sequence number of specific 6P ADD request to track 
		
		// custom parameter to add random delay before 6P requests to minimize
		// probability of simulatenous transactions in both directions between two nodes
		bool send6pDelayed = default(false);  
		
		// Module parameters required by the interface but are actually unused
		int initNumcells = default(6);
		int maxRetryBackoff @unit(ms) = default(60000ms);
}