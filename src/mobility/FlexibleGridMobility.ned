

package tsch.mobility;

import inet.mobility.static.StaticGridMobility;

@namespace(tsch);

simple FlexibleGridMobility extends StaticGridMobility
{
    parameters:
        int resetRowAt = default(0);
        @class(FlexibleGridMobility);
}

