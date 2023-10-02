//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2021  G�kay Apusoglu
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

#include "ReSaMobility.h"


using namespace inet;

Define_Module(tsch::ReSaMobility);

namespace tsch {

ReSaMobility::ReSaMobility() {
    // TODO Auto-generated constructor stub
}

ReSaMobility::~ReSaMobility() {
    // TODO Auto-generated destructor stub
}

void tsch::ReSaMobility::setCoords(std::ifstream& inStream)
{
    inStream.open(ReSaLayoutPath);

    if (!inStream) {
       std::cerr << "Unable to open referenced ReSaLayout file!";
       exit(1);
    }

    std::string s;
    int ctr = 0;

    while (inStream.good())
    {
        getline(inStream, s, '\n');
        if (s.find(moduleType) == 0)
        {
            if (ctr == subjectModule->getIndex())
            {
                std::istringstream iss(s);
                std::string pos_str;
                int col_ctr = 0;
                while (getline(iss, pos_str, '\t'))
                {
                    if (pos_str != moduleType)
                    {
                        ++col_ctr;
                        if (col_ctr == xColidx){
                            lastPosition.x = std::stod(pos_str)/100 + x_coord_offset;
                        }
                        else if(col_ctr == yColidx){
                            lastPosition.y = std::stod(pos_str)/100 + y_coord_offset;
                        }
                        else if(col_ctr == zColidx){
                            lastPosition.z = std::stod(pos_str)/100;
                        }
                    }
                }
//                std::cout << subjectModule->getFullName() << "is located at " << lastPosition << "\n";
            }
            ++ctr;
        }
    }
    inStream.close();

}

void tsch::ReSaMobility::setCoords(std::istringstream& inStream){

    std::string s;
    int ctr = 0;

    while (inStream.good())
    {
        getline(inStream, s, '\n');
        if (s.find(moduleType) == 0)
        {
            if (ctr == subjectModule->getIndex())
            {
                std::istringstream iss(s);
                std::string pos_str;
                int col_ctr = 0;
                while (getline(iss, pos_str, '\t'))
                {
                    if (pos_str != moduleType)
                    {
                        ++col_ctr;
                        if (col_ctr == xColidx){
                            lastPosition.x = std::stod(pos_str)/100 + x_coord_offset;
                        }
                        else if(col_ctr == yColidx){
                            lastPosition.y = std::stod(pos_str)/100 + y_coord_offset;
                        }
                        else if(col_ctr == zColidx){
                            lastPosition.z = std::stod(pos_str)/100;
                        }
                    }
                }
//                std::cout << subjectModule->getFullName() << "is located at " << lastPosition << "\n";
            }
            ++ctr;
        }
    }

}

void tsch::ReSaMobility::setInitialPosition(){

    ReSaLayoutPath = par("ReSaLayoutPath");
    coordList = par("coordList");
    updateFromDisplayString = par("updateFromDisplayString");
    x_coord_offset = par("x_coord_offset");
    y_coord_offset = par("y_coord_offset");
    moduleType = par("moduleType");
    typeColidx = par("typeColidx");
    xColidx = par("xColidx");
    yColidx = par("yColidx");
    zColidx = par("zColidx");

    constraintAreaMax.x = 100;
    constraintAreaMax.y = 100;
    constraintAreaMax.z = 6;

    if (enableReadingFromFile){
        std::ifstream inStream;
        setCoords(inStream);
        }
    else {
        std::istringstream inStream(coordList);
        setCoords(inStream);
        }

}

}

// namespace tsch
