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
import rpl.RplRouter;

network Branches
{
    parameters:
        @display("bgb=1050,1131");
    submodules:
        configurator: Ipv6FlatNetworkConfigurator {
            parameters:
                @display("p=863.58997,57.46;is=s");
        }
        radioMedium: <> like IRadioMedium {
            parameters:
                @display("p=863.58997,157.17;is=s");
        }

        sink: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=392.6,49.399998");
        }
        host1: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=504.4,124.799995");
        }
        host2: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=578.5,200.2");
        }
        host3: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=604.5,305.5");
        }
        host4: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=630.5,410.8");
        }
        host5: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=382.19998,159.9");
        }
        host6: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=382.19998,262.6");
        }
        host7: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=379.6,360.1");
        }
        host8: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=379.6,461.5");
        }
        host9: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=267.8,124.799995");
        }
        host10: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=171.59999,210.59999");
        }
        host11: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=130,321.1");
        }
        host12: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=122.2,430.3");
        }
        host13: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=122.2,527.8");
        }
        host14: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=379.6,559");
        }
        host15: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=643.5,508.3");
        }
        host16: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=122.2,639.6");
        }
        host17: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=379.6,652.6");
        }
        host18: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=643.5,614.89996");
        }
        host19: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=122.2,734.5");
        }
        host20: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=380.9,742.3");
        }
        host21: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=643.5,726.7");
        }
        host22: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=122.2,835.89996");
        }
        host23: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=379.6,839.8");
        }
        host24: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=643.5,835.89996");
        }
        host25: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=122.2,924.3");
        }
        host26: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=380.9,932.1");
        }
        host27: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=643.5,932.1");
        }
        host28: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=122.2,1028.2999");
        }
        host29: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=378.3,1025.7");
        }
        host30: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=643.5,1034.7999");
        }
}
