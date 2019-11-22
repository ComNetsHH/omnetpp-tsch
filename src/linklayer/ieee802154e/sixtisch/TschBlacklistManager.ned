package tsch.linklayer.ieee802154e.sixtisch;

module TschBlacklistManager {
    parameters:
        @class(TschBlacklistManager);

        @statistic[cell_reassignment_successful](type=bool; record=stats,vector);

        // records each time the private/neighbor blacklist was updated
        @statistic[blacklist_entry_updated_private](type=int;record=count,vector);
        @statistic[blacklist_entry_updated_neighbor](type=int;record=count,vector);
}
