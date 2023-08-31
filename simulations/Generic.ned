//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//           (C) 2021  Yevhenii Shudrenko
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


package tsch.simulations;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.configurator.ipv6.Ipv6FlatNetworkConfigurator;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.node.inet.WirelessHost;
import tsch.linklayer.ieee802154e.TschHopping;
import rpl.RplRouter;

network Generic
{
    parameters:
        @display("bgb=154.54666,130.87334");
        int numSinks = default(1);
        int numHosts = default(30);
        double lambda = default(1); // optional parameter for multi-hop delay testing 
    submodules:
        configurator: Ipv6FlatNetworkConfigurator {
            parameters:
                @display("p=131.76666,54.94;is=s");
        }
        channelHopping: TschHopping {
            @display("p=131.76666,12.506666;is=s");
        }
        radioMedium: <> like IRadioMedium {
            parameters:
                @display("p=131.31999,33.053333;is=s");
        }

        sink[numSinks]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=32.16,41.093334");
        }
        host[numHosts]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=32.16,20.1");
        }
}
