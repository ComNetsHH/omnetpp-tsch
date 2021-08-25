//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
// Control messages sent by the 6top sublayer to the mac layer
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//			 (C) 2021  Yevhenii Shudrenko
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

simple Tsch6TopSublayer
{
    parameters:
	    int piggybackingBackoff @unit(ms) = 0;
	   	double sfstarttime @unit(s) = default(0.0);
	   	bool stats = false;	           	
	   	@class(Tsch6topSublayer);
	   	
	   	@signal[sent6pAdd];
	   	@signal[sent6pRelocate];
	   	@signal[sent6pDelete];
	   	@signal[sent6pClear];
	   	@signal[sent6pResponse];
	   	
        @statistic[sent6pAdd](title="Sent 6P ADD requests"; source=sent6pAdd; record=vector?, count; interpolationmode=none);
        @statistic[sent6pRelocate](title="Sent 6P RELOCATE requests"; source=sent6pRelocate; record=vector?, count; interpolationmode=none);
        @statistic[sent6pDelete](title="Sent 6P DELETE requests"; source=sent6pDelete; record=vector?, count; interpolationmode=none);
        @statistic[sent6pClear](title="Sent 6P CLEAR requests"; source=sent6pClear; record=vector?, count; interpolationmode=none);
        @statistic[sent6pResponse](title="Sent 6P responses"; source=sent6pResponse; record=vector?, count; interpolationmode=none);
	   	
    gates:
        input linkInfoControlIn;
   	    //input sensingControlIn;		
   	    //output sensingControlout;
   	    input lowerLayerIn;
   	    output lowerLayerOut;
   	    input lowerControlIn;
   	    //output lowerControlOut;
}

