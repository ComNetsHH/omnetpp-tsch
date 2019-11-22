//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package tsch.linklayer.ieee802154e.sixtisch;

@namespace(tsch::sixtisch);

//
// TODO auto-generated module
//
simple TschSpectrumSensing
{
    // path to radio, used to query e.g. bandwidth and get other required modules (such as the radio medium)
    string radioModule = default("^.^.^.radio");
    // path to channelPlan, used to e.g. translate channels to frequencies
    string channelPlanModule = default("^.^.channelHopping");
    // time to listen on one channel
    double ccaDetectionTime @unit(s) = default(0.000128 s); // 8 symbols
    // time between measurements on different channels
    double measurementInterval @unit(s) = default(0.000128 s);
    // sweep interval (only in use if continous mode is on)
    double sweepInterval @unit(s) = default(0.000128 s);
    
    @class(TschSpectrumSensing);
    
    @signal[power-*](type=double);
    @statisticTemplate[powerStats](record=stats,vector; interpolationmode=none);
}
