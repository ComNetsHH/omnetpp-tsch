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

package tsch.simulations.wireless.aircraft;
//CHIRP
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211DimensionalRadioMedium;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
//WAIC
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.node.inet.WirelessHost;


network Aircraft
{
    parameters:
        int numWaicHosts;
        @display("bgb=450,450");
    submodules:
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=50,45;is=s");
        }
        radioMedium: <> like IRadioMedium {
            parameters:
                @display("p=50,210;is=s");
        }
        wpanHost0: WirelessHost {
            parameters:
                @display("r=,,#707070;p=230,100;i=misc/sensor2");
        }
        wpanHost1: WirelessHost {
            parameters:
                @display("r=,,#707070;p=250,100;i=misc/sensor2");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=50,100;is=s");
        }
        wifiHost0: AdhocHost {
            @display("p=230,120;i=device/laptop");
        }
        wifiHost1: AdhocHost {
            @display("p=250,120;i=device/laptop");
        }
}

