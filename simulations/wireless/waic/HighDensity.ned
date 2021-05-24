//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//           (C) 2019  Daniel Ploeger
//           (C) 2019  Leo Krueger
//           (C) 2006  Andras Varga
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


package tsch.simulations.wireless.waic;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.configurator.ipv6.Ipv6FlatNetworkConfigurator;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.node.inet.WirelessHost;
import rpl.configurator.RplNetConfigurator;
import rpl.RplRouter;
import rpl.Rpl;

network HighDensity
{
    parameters:
        @display("bgb=20,110");
        int numHosts = default(1);
        int numSinks = default(1);
    submodules:
        configurator: RplNetConfigurator {
            parameters:
                @display("p=1013.768,81.872;is=s");
        }
        radioMedium: <> like IRadioMedium {
            parameters:
                @display("p=1011.36,228.76;is=s");
        }
        sink[numSinks]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=391.62,121.98");
        }
        host[numHosts]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=313.04,416.58398");
        }
}
