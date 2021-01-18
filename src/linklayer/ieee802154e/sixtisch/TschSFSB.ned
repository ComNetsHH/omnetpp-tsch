//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//           (C) 2017  Lotte Steenbrink
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

package tsch.linklayer.ieee802154e.sixtisch;

import tsch.linklayer.ieee802154e.sixtisch.TschSF;

simple TschSFSB like TschSF
{
    parameters:
        @class(TschSFSB);
        // the time after which an active, unfinished transaction expires
        // (needs to be defined by every SF as per the 6P standard)
        int timeout = default(25000);

        // the number of cells to be initially scheduled between all nodes
        int initNumcells = default(6);

        // The maximum time in ms that will pass until another initial ADD
        // request is sent if the last one wasn't successful
        int maxRetryBackoff @unit("ms") = default(60000ms);

        // The number of Spectrum Sensing results to consult for a
        // blacklisting decision
        int numSensingResults = default(15);

        // If the channel has been busy >= blacklistThreshold % of the last
        // numSensingResults, it should be blacklisted
        double blacklistThreshold = default(75.0);

        // Time (in ms) after which data MUST be sent as a Signal request if
        // it couldn't be piggybacked successfully.
        int blacklistUpdateTimeout @unit("ms") = default(30ms);

        // The default lifetime (in ms) of a blacklist entry, if nothing else
        // is  specified in the blacklist update
        int blacklistLifetime @unit("ms") = default(3600ms);

        // "simple"   : idle on blacklisted channels
        // "extended" : jump to CHANNEL_SWAP_OFFSET on blacklisted channels
        string version = default("simple");
        int channelSwapOffset = default(1);
        
        int senseInterval @unit("ms") = default(200ms);
        int startSenseTime @unit("s") = default(1s);
        bool disable = default(false);

        // emitted each time the schedule setup with a neighbor is complete,
        // i.e. all cells have been allocated in both directions.
        //@statistic[initial_schedule_complete](record=count,vector);
}