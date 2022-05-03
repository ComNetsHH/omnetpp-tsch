//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2022  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//           (C) 2022  G�kay Apusoglu
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
import inet.physicallayer.common.packetlevel.RadioMedium;

import inet.node.wireless.AccessPoint;

import inet.node.ipv6.StandardHost6;
import inet.node.inet.Router;
import inet.node.ipv6.Router6;
import inet.node.inet.StandardHost;
import inet.node.wireless.Probe;
import inet.environment.common.PhysicalEnvironment;
import inet.visualizer.environment.PhysicalEnvironmentCanvasVisualizer;
network ReSA
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
        int numAircrafts = default(1);
        bool enableObstacles = default(false);
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
        physicalEnvironment: PhysicalEnvironment if enableObstacles==true {
            parameters:
                @display("p=7.6,3.1166666;is=s");
        }
        PhysicalEnvironmentCanvasVisualizer: PhysicalEnvironmentCanvasVisualizer if enableObstacles==true {
            parameters:
                @display("p=8.6,3.1166666;is=s");
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
        server1: StandardHost6 if numUsers>0 {
            @display("p=20.02,12.04;i=device/mainframe");
        }
        user1[numUsers]: StandardHost6 if numUsers>0 {
            @display("p=13.86,36.12;is=n;i=misc/person3");
            numWlanInterfaces = 1;
        }
        ap1[3]: AccessPoint if numUsers>0 {
            @display("p=14.56,20.02;i=device/accesspoint,cyan;is=s");
        }
		r1 : Router6 if numUsers>0 {
            parameters:
                @display("p=14.56,20.02;is=s");
        }
    
        gw2[numGW]: RplRouter if numAircrafts>=2{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
		am2[numAM]: RplRouter if numAircrafts>=2{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        hs2[numHS]: RplRouter if numAircrafts>=2{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        sos2[numSoS]: RplRouter if numAircrafts>=2{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        bm2[numBM]: RplRouter if numAircrafts>=2{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
        was2[numWAS]: RplRouter if numAircrafts>=2{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
        server2: StandardHost if (numAircrafts>=2 && numUsers>0) {
            @display("p=20.02,12.04;i=device/mainframe");
        }
        user2[numUsers]: RplRouter if (numAircrafts>=2 && numUsers>0) {
            @display("p=13.86,36.12;is=n;i=misc/person3");
        }
        ap2[3]: AccessPoint if (numAircrafts>=2 && numUsers>0) {
            @display("p=14.56,20.02;i=device/accesspoint,cyan;is=s");
        }
		r2 : Router6 if (numAircrafts>=2 && numUsers>0) {
            parameters:
                @display("p=14.56,20.02;is=s");
        }

        gw3[numGW]: RplRouter if numAircrafts>=3{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
		am3[numAM]: RplRouter if numAircrafts>=3{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        hs3[numHS]: RplRouter if numAircrafts>=3{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        sos3[numSoS]: RplRouter if numAircrafts>=3{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        bm3[numBM]: RplRouter if numAircrafts>=3{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
        was3[numWAS]: RplRouter if numAircrafts>=3{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
        server3: StandardHost if (numAircrafts>=3 && numUsers>0) {
            @display("p=20.02,12.04;i=device/mainframe");
        }
        user3[numUsers]: RplRouter if (numAircrafts>=3 && numUsers>0) {
            @display("p=13.86,36.12;is=n;i=misc/person3");
        }
        ap3[3]: AccessPoint if (numAircrafts>=3 && numUsers>0) {
            @display("p=14.56,20.02;i=device/accesspoint,cyan;is=s");
        }
		r3 : Router6 if (numAircrafts>=3 && numUsers>0) {
            parameters:
                @display("p=14.56,20.02;is=s");
        }
        
        gw4[numGW]: RplRouter if numAircrafts>=4{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
		am4[numAM]: RplRouter if numAircrafts>=4{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        hs4[numHS]: RplRouter if numAircrafts>=4{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        sos4[numSoS]: RplRouter if numAircrafts>=4{
            parameters:
                @6tisch;
                @display("r=,,#707070;p=28.48,34.354");
        }
        bm4[numBM]: RplRouter if numAircrafts>=4{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
        was4[numWAS]: RplRouter if numAircrafts>=4{
            parameters:
                @6tisch;
                @display("p=28.48,20.114;r=,,#707070");
        }
        server4: StandardHost if (numAircrafts>=4 && numUsers>0) {
            @display("p=20.02,12.04;i=device/mainframe");
        }
        user4[numUsers]: RplRouter if (numAircrafts>=4 && numUsers>0) {
            @display("p=13.86,36.12;is=n;i=misc/person3");
        }
        ap4[4]: AccessPoint if (numAircrafts>=4 && numUsers>0) {
            @display("p=14.56,20.02;i=device/accesspoint,cyan;is=s");
        }
		r4 : Router6 if (numAircrafts>=2 && numUsers>0) {
            parameters:
                @display("p=14.56,20.02;is=s");
        }        
    connections:
        ap1[0].ethg++ <--> {  datarate = 100Mbps; } <--> r1.ethg++ if numUsers>0;
        ap1[1].ethg++ <--> {  datarate = 100Mbps; } <--> r1.ethg++ if numUsers>0;
        ap1[2].ethg++ <--> {  datarate = 100Mbps; } <--> r1.ethg++ if numUsers>0;
        ap2[0].ethg++ <--> {  datarate = 100Mbps; } <--> r2.ethg++ if (numAircrafts>=2 && numUsers>0);
        ap2[1].ethg++ <--> {  datarate = 100Mbps; } <--> r2.ethg++ if (numAircrafts>=2 && numUsers>0);
        ap2[2].ethg++ <--> {  datarate = 100Mbps; } <--> r2.ethg++ if (numAircrafts>=2 && numUsers>0);
        ap3[0].ethg++ <--> {  datarate = 100Mbps; } <--> r3.ethg++ if (numAircrafts>=3 && numUsers>0);
        ap3[1].ethg++ <--> {  datarate = 100Mbps; } <--> r3.ethg++ if (numAircrafts>=3 && numUsers>0);
        ap3[2].ethg++ <--> {  datarate = 100Mbps; } <--> r3.ethg++ if (numAircrafts>=3 && numUsers>0);
        ap4[0].ethg++ <--> {  datarate = 100Mbps; } <--> r4.ethg++ if (numAircrafts>=4 && numUsers>0);
        ap4[1].ethg++ <--> {  datarate = 100Mbps; } <--> r4.ethg++ if (numAircrafts>=4 && numUsers>0);
        ap4[2].ethg++ <--> {  datarate = 100Mbps; } <--> r4.ethg++ if (numAircrafts>=4 && numUsers>0);
        r1.ethg++ <--> {  datarate = 100Mbps; } <--> server1.ethg++ if numUsers>0;
        r2.ethg++ <--> {  datarate = 100Mbps; } <--> server2.ethg++ if (numAircrafts>=2 && numUsers>0);
        r3.ethg++ <--> {  datarate = 100Mbps; } <--> server3.ethg++ if (numAircrafts>=3 && numUsers>0);
        r4.ethg++ <--> {  datarate = 100Mbps; } <--> server4.ethg++ if (numAircrafts>=4 && numUsers>0);
        
}