//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2022  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//			 (C) 2022  Gökay Apusoglu
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

import inet.examples.wireless.throughput.WirelessAPWithSink;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.configurator.ipv6.Ipv6FlatNetworkConfigurator;
import inet.node.inet.SensorNode;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.physicallayer.common.packetlevel.RadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.node.inet.WirelessHost;
import inet.node.inet.AdhocHost;
import rpl.RplRouter;
import tsch.mobility.ReSaMobility;
import tsch.linklayer.ieee802154e.TschHopping;

// libraries for ISM interference
import inet.node.ethernet.Eth100M;
import inet.node.ethernet.EtherLink;
import inet.node.ethernet.EtherSwitch;

import inet.applications.contract.IApp;
import inet.common.misc.ThruputMeteringChannel;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;

import inet.node.wireless.AccessPoint;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;

network ReSA_ism_test
{
    parameters:
        @display("bgb=38.5,59.95");
        int numAM = default(1);
        int numHS = default(1);
        int numSoS = default(1);
        int numGW = default(1);
        int numBM = default(1);
        int numWAS = default(1);
        int numHosts = default(0);
        int numUsers = default(0);
        double gridCenterX = default(0);
        double gridCenterY = default(0);

    submodules:

        configurator: Ipv6FlatNetworkConfigurator {
            parameters:
                @display("p=32.45,3.1166666;is=s");
        }
        channelHopping: TschHopping {
            @display("p=18.62,4.2");
        }
        radioMedium: RadioMedium {
            parameters:
                @display("p=6.6,3.1166666;is=s");
        }
        gw1[numGW]: RplRouter {
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
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
        am1[numAM]: RplRouter {
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        host[numHosts]: RplRouter {
            parameters:
                @6tisch;
                @display("p=28.48,20.114");
        }
//        server1: StandardHost {
//            @display("p=28.48,20.114;i=device/mainframe");
//        }
        user1[numUsers]: AdhocHost {
            @display("p=28.48,20.114;is=n;i=misc/person3");
        }
        dummySink: AdhocHost {
            @display("p=28.48,20.114;is=n;i=misc/sensorgateway");
        }        
//        ap1[1]: AccessPoint {
//            @display("p=14.56,20.02;i=device/accesspoint,cyan;is=s");
//        }

//    connections:
//        ap1[0].ethg++ <--> {  datarate = 100Mbps; } <--> server1.ethg++;
//        ap1[1].ethg++ <--> {  datarate = 100Mbps; } <--> server1.ethg++;
//        ap1[2].ethg++ <--> {  datarate = 100Mbps; } <--> server1.ethg++;

}
