//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//			 (C) 2021  Yevhenii Shudrenko
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
import rpl.RplRouter;

network WaicBasicRpl
{
    parameters:
        @display("bgb=450,531");
    submodules:
        configurator: Ipv6FlatNetworkConfigurator {
            parameters:
                @display("p=100,100;is=s");
        }
        radioMedium: <> like IRadioMedium {
            parameters:
                @display("p=100,200;is=s");
        }

        sink: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=251.45,72.76");
        }
        host2: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=295.32,155.15");
        }
        host3: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=210.79001,225.77");
        }
        host4: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=295.32,302.81");
        }
        host5: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=210.79001,384.13");
        }
}
