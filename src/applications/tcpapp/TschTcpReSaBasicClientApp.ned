//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2022  G�kay Apusoglu
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

package tsch.applications.tcpapp;

import inet.applications.tcpapp.TcpBasicClientApp;

@namespace(tsch);

simple TschTcpReSaBasicClientApp extends TcpBasicClientApp
{
    parameters:
    	@class(TschTcpReSaBasicClientApp);
    	double activeUserRatio = default(1);
}