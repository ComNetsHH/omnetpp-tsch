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

package tsch.mobility;

import inet.mobility.static.StationaryMobility;

@namespace(tsch);

simple ReSaMobility extends StationaryMobility
{
    parameters:
        @class(ReSaMobility);
        double x_coord_offset @unit("m") = default(0);
        double y_coord_offset @unit("m") = default(0);
        string moduleType = default("");
        string ReSaLayoutPath = default("");
        string coordList = default("");
        bool enableReadingFromFile = default(false);
        int typeColidx = default(0);
        int xColidx = default(0);
        int yColidx = default(0);
        int zColidx = default(0);
}

