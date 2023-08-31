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
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.node.inet.WirelessHost;

network SimpleWaic
{
    parameters:
        @display("bgb=40,85");
        int numSinks = default(1);
        int numHosts = default(30);
    submodules:
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=27.91269,1.9802921;is=s");
        }
        radioMedium: <> like IRadioMedium {
            parameters:
                @display("p=28.006989,6.9781723;is=s");
        }
        sink[numSinks]: WirelessHost {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=17.63403,16.219536");
        }
        host[numHosts]: WirelessHost {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=17.63403,16.219536");
        }
}
