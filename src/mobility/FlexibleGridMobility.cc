/*
 * Copyright (C) 2006 Isabel Dietrich <isabel.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2013 OpenSim Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "FlexibleGridMobility.h"

using namespace inet;

Define_Module(tsch::FlexibleGridMobility);

namespace tsch {

void FlexibleGridMobility::initialize(int stage) {
    StaticGridMobility::initialize(stage);

    auto rotatedCoords = new Coord();

    // apply coordinates transformation only after initial positions are set
    if (stage == INITSTAGE_SINGLE_MOBILITY + 1) {

        auto network = getContainingNode(this)->getParentModule();

        if (getContainingNode(this)->getIndex() == 0) {
            const char * moduleName = this->getParentModule()->getName();
            auto hostModTopLeft = network->getSubmodule(moduleName, 0);
            auto hostMobilityTopLeft = check_and_cast<MobilityBase*> (hostModTopLeft->getSubmodule("mobility"));

            auto hostModBotRight = network->getSubmodule(moduleName, 98);
            auto hostMobilityBotRight = check_and_cast<MobilityBase*> (hostModBotRight->getSubmodule("mobility"));

            auto originCoords = getOriginCoordinates(hostMobilityTopLeft->getLastPosition(), hostMobilityBotRight->getLastPosition());

            network->par("gridCenterX") = originCoords->x;
            network->par("gridCenterY") = originCoords->y;
        }

        auto gridCenter = new Coord(network->par("gridCenterX").doubleValue(), network->par("gridCenterY").doubleValue());

        // take the location of a gateway as the centering point for seatbelt grid
        auto gwModule = network->getSubmodule("gw1", 0);
        EV_DETAIL << "Flexigrid mobility found gateway module - " << gwModule << endl;
        auto gwLocation = (check_and_cast<MobilityBase*> (gwModule->getSubmodule("mobility")))->getLastPosition();

        auto shiftX = gwLocation.x - gridCenter->x + par("gridOffsetX").doubleValue(); // gateway location is a bit offset to the right
        auto shiftY = gwLocation.y - gridCenter->y + par("gridOffsetY").doubleValue(); // and to the bottom

        auto rotatedPos = rotateAroundPoint(lastPosition, *gridCenter);

        lastPosition.x = rotatedPos->x + shiftX;
        lastPosition.y = rotatedPos->y + shiftY;
    }
}

void FlexibleGridMobility::setInitialPosition()
{
    EV_DETAIL << "Set initial position called" << endl;
    int numHosts = par("numHosts");
    double marginX = par("marginX");
    double marginY = par("marginY");
    double separationX = par("separationX");
    double separationY = par("separationY");
    int columns = par("columns");
    int rows = par("rows");
    int resetRowAtIndex = par("resetRowAtNodeIndex");
    if (numHosts > rows * columns)
        throw cRuntimeError("parameter error: numHosts > rows * columns");

    int index = subjectModule->getIndex();

    int row = (index >= resetRowAtIndex ? (index - resetRowAtIndex) : index) / columns;
    int col = index % columns;
    lastPosition.x = constraintAreaMin.x + marginX + col * separationX;
    lastPosition.y = constraintAreaMin.y + marginY + row * separationY;
    lastPosition.z = par("initialZ");
    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}

Coord* FlexibleGridMobility::rotateAroundPoint(Coord target, Coord origin) {
    EV_DETAIL << "Rotating " << target << " around " << origin;
    auto translated = new Coord(target.x - origin.x, origin.y - target.y);
    auto rotated = new Coord(-translated->y, translated->x);

    auto result = new Coord(origin.x + rotated->x, origin.y - rotated->y);

    EV_DETAIL << ": " << *result << endl;

    return result;
}

Coord* FlexibleGridMobility::getOriginCoordinates(Coord topLeftCorner, Coord bottomRightCorner) {
    auto centerCoords = new Coord( topLeftCorner.x + (double) (bottomRightCorner.x - topLeftCorner.x) / 2, topLeftCorner.y + (double) (bottomRightCorner.y - topLeftCorner.y) / 2 );
    EV_DETAIL << "Given top left corner at " << topLeftCorner << " and bottom right corner at " << bottomRightCorner
            << ",\ncalculated center at " << centerCoords->x << ", " << centerCoords->y << endl;
    return centerCoords;
}


} // namespace tsch
