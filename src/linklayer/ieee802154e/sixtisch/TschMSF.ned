package tsch.linklayer.ieee802154e.sixtisch;
import tsch.linklayer.ieee802154e.sixtisch.TschSF;

simple TschMSF like TschSF
{
    parameters:
        @class(TschMSF);
        
        @signal[rplParentChanged](type=unsigned long);
        @signal[queueUtilization](type=double);
        @signal[failed6pAdd];
        @signal[uncoverableGap];
        @signal[uplinkSlotOffset](type=int);
        
        @statistic[rplParentChanged](title="Number of rpl parents changed"; source="rplParentChanged"; record=count; interpolationmode=none);
        @statistic[queueUtilization](title="Queue utilization"; source=queueUtilization; record=mean?, vector?; interpolationmode=none);
        @statistic[failed6pAdd](title="Failed 6P ADD requests"; source=failed6pAdd; record=count, vector?; interpolationmode=none);
        @statistic[uncoverableGaps](title="Number of unrecovered gaps"; source=uncoverableGap; record=count, vector?; interpolationmode=none);
        @statistic[uplinkSlotOffsets](title="Slot offsets of dedicated cells"; source=uplinkSlotOffset; record=vector; interpolationmode=none);
        
        // the time after which an active, unfinished transaction expires
        // (needs to be defined by every SF as per the 6P standard)
        int timeout @unit("ms") = default(25000ms);
        
        int numMinCells = default(5); // number of minimal cells (broadcast / control traffic) to schedule
        double upperCellUsageLimit = default(0.75);
        double lowerCellUsageLimit = default(0.25);
        int housekeepingPeriod @unit("s") = default(60s); // how often should we check cell PDR
        int housekeepingStart @unit("s") = default(60s);
         	 
        double relocatePdrThresh = default(0.50); // if cell exhibits PDR worse than this, relocate it!
        int maxNumCells = default(intuniform(4, 10)); // sliding window size to esmitate cell utilization
        int maxNumTx = default(254); // maximum number of elapsed cells to consider for PDR calculation
        // determines how much the MAX_NUM_CELLS window size increases with respect to the number of cells scheduled
        double maxNumCellsScalingFactor = default(10);
//        int slotframeLen = default(101);

		// this value + number of cells required = CELL_LIST size in 6P transactions, 
		// e.g. requesting to schedule 1 cell would result in a 4 + 1 = 5 cells proposed to the neighbor
        int cellListRedundancy = default(4);   

        int cellsToAdd = default(1); // number of cells to add with neighbor if cell usage exceeds threshold
        bool disable = default(false);
        // determines whether auto TX cells to neighbors are scheduled on demand (per transmission), or globally during initialization stage
        bool clearQueueOnReset = default(false); // if enabled, completely flushes regular queue with a neighbor when the link is reset    
        bool disableHousekeeping = default(false);
             
     	bool handleRankUpdates = default(false); // TEST parameter, only needed for delay-testing scenarios
     	bool noRtxQueuing = default(false); // only for manual delay-testing scenarios to ensure no queuing through retransmissions is possible
     	
     	// node periodically checks the existence of a dedicated uplink cell until it one is successfully scheduled
     	bool scheduleUplinkManually = default(false);    
     	bool blacklistSlots = default(false); // slot offsets rejected by the destination node will not be proposed to it ever again 

		double startTime @unit("s") = default(uniform(1s, 2s));
		
		bool estimateQueueUtil = default(false); // use queue utilization instead of cell util
		double queueEstimationPeriod @unit("s") = default(uniform(10s, 20s));
		double queueUtilUpperThresh = default(0.95);
		
		int maxRetries = default(3); // 6P request retry threshold
		int initialNumCells = default(1); // number of cells to schedule with RPL preferred parent initially
		
		int trackFailed6pAddByNum = default(0); // sequence number of specific 6P ADD request to track
		int minCellChannelOffset = default(0); // channel offset for the minimal cells, esp. important in lossy environment
		
		// parameters for blacklisting, the range of white-listed channel offsets
		bool blacklistingEnabled = default(false);
		int chOfStart = default(0); // 802.15.4 ch. 11
		int chOfEnd = default(15); // 802.15.4 ch. 26
		
		// UI params
        bool showDedicatedTxCells = default(true); // show scheduled TX cells coordinates above node pictorgram
    	bool showTxCellCount = default(false); // instead of displaying TX cells coordinates, show their count only
        bool showQueueUtilization = default(false); // displays queue utilization for preferred parent (if available)
        bool showLinkResets = default(false); // display number of link resets
        bool showQueueSize = default(false);
		
		// custom parameter to add random delay before 6P requests to minimize
		// probability of simulatenous transactions in both directions between two nodes
		bool send6pDelayed = default(false);  
		bool downlinkDedicated = default(false);
		bool flushQueueOnAdd = default(false); // clear the packet queue after each successful 6P ADD (helps schedule exactly arrival_rate + 1 TX cells) 
		bool checkScheduleConsistency = default(true); // on each packet enqueued verify the synchronization between TschLinkInfo and TschSlotframe
		
		// Module parameters required by the interface but are actually unused
		int initNumcells = default(6);
		int maxRetryBackoff @unit(ms) = default(60000ms);
		
		// Misc
		bool lowLatencyMode = default(false);
		bool cellBundlingEnabled = default(false); // defines the rule for CELL_LIST creation
		bool handleCellBundlingSignal = default(false); // determines whether to react to cell-bundling signal from RPL
		int cellBundleSize = default(2);
		int lowLatencyStartingOffset = default(-1);
		int minimumUplinkBandwidth = default(1); // for nodes with "scheduleUplinkManually" enabled
		bool scheduleUplinkOnJoin = default(true);
		bool cellMatchingEnabled = default(false); // MSF attempts to schedule cells in a checkerboard manner - TX cell directly follows a RX one
		
		bool allowAutoCellDeletion = default(false);
		double stopAdaptationToTrafficAt @unit("s") = default(-1s); // required for scenarios where number of TX cells has to be frozen from a certain point 
		// apply hash function to the node MAC before taking the modulo by number of channels, 
		// diversifies auto RX channel offsets of nodes from the same module array 
		bool useHashChOffsets = default(false);
		string initCellOverride = default(""); // location of the first dedicated cell to be scheduled with pref. parent
		bool showNumPktSent = default(false); // show number of UDP packets sent above the node icon 
}