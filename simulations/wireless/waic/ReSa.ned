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
import inet.node.inet.AdhocHost;
import rpl.RplRouter;
import rpl.configurator.RplNetConfigurator;
import tsch.mobility.ReSaMobility;
import tsch.linklayer.ieee802154e.TschHopping;

network ReSa
{
    parameters:
        @display("bgb=38.5,59.95");
        int numAM = default(1);
        int numHS = default(1);
        int numSoS = default(1);
        int numGW = default(1);
        int numBM = default(1);
        int numWAS = default(1);
    submodules:
        configurator: Ipv6FlatNetworkConfigurator {
            parameters:
                @display("p=32.45,3.1166666;is=s");
        }
        channelHopping: TschHopping;
        radioMedium: <> like IRadioMedium {
            parameters:
                @display("p=6.6,3.1166666;is=s");
        }
        gw1[numGW]: RplRouter {
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }                      
        am1[numAM]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        hs1[numHS]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        sos1[numSoS]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }                        
        bm1[numBM]: RplRouter {
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }                        
        was1[numWAS]: RplRouter {
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
//        interferer: WirelessHost {
//            parameters:
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//        wifiSink: WirelessHost {
//            parameters:
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//        gw2[numGW]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        } 
//        gw3[numGW]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        } 
//        gw4[numGW]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//		am2[numAM]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        am3[numAM]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        am4[numAM]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        hs2[numHS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        hs3[numHS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        hs4[numHS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        sos2[numSoS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        sos3[numSoS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        sos4[numSoS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("r=,,#707070;p=28.48,34.354");
//        }
//        bm2[numBM]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//        bm3[numBM]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//        bm4[numBM]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//        was2[numWAS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//        was3[numWAS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        }
//        was4[numWAS]: RplRouter {
//            parameters:
//                @6tisch;
//                @display("p=28.48,20.114;r=,,#707070");
//        }                                       
}
