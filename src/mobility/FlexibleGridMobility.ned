

package tsch.mobility;

import inet.mobility.static.StaticGridMobility;

@namespace(tsch);

simple FlexibleGridMobility extends StaticGridMobility
{
    parameters:
        @class(FlexibleGridMobility);
        
        // the index of the node at which Y coordinate will be reset to allow creating several disjoint grids vertically aligned 
        int resetRowAtNodeIndex = default(0);
        
        // parameters to help align seatbelts grid with the center of the aircraft from ReSA Demo 
        double gridOffsetX = default(0);
        double gridOffsetY = default(0);
        
        bool rotateHorizontally = default(true); // rotates the seat belt grid horizontally from left to right 
}

