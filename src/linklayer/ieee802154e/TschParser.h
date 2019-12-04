/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Louis Yin
 *           (C) 2000  Institut fuer Telematik, Universitaet Karlsruhe
 *           (C) 2004  Andras Varga
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __INET_TschParser_H
#define __INET_TschParser_H

#include "inet/common/INETDefs.h"
#include <array>

namespace inet{
/*
 * Parses an xml Tsch_Schedule into Slotframe/Link(s)
 *
 */

//class TschParser
//class INET_API TschParser
class TschParser
{
  public:
    /**
     * Constructor
     */
    TschParser();

    /**
     * Destructor
     */
    virtual ~TschParser();

    /**
     * Read Tsch xml Schedule file and extract plain text.
     * ?? return 0 on success, -1 on error
     */
    virtual int readTschParmFromXmlFile(const char *filename);

  protected:
    // Parsing functions

    // Return 1 if beginning of str1 and str2 is equal up to str2-len,
    // otherwise 0.
    //
    int streq(const char *str1, const char *str2);

  public:

    // save extracted Tsch parameter in array of struct ? Or directly into Slotframe/Links class????
    struct Tsch_Link {
      int SlotOffset;
      int channelOffset;
      bool Option_tx;
      bool Option_rx;
      bool Option_shared;
      bool Option_timekeeping;
      bool Type_normal;
      bool Type_advertising;
      bool Type_advertisingOnly;
      int Virtual_id;
      std::string Neighbor_path;
      std::string Neighbor_address;
      // ==>  isempty(str):  str.empty()
    };

    // save extracted Tsch parameter in array of struct ? Or directly into Slotframe/Links class????
    struct Tsch_Slotframe {
      int handle;
      int macSlotframeSize;
      int numLinks;
      Tsch_Link links[100];
    };
    Tsch_Slotframe Slotframe[100];

  private:


  protected:

    // Extract Slotframe parameters and Links parameters from xml file:
    // Return value is not fixed yet...
    int extract_Tsch_param(Tsch_Slotframe &Slotframe, int tsch_token, char *text, int& charpointer);

    // forwinding in text until token is found and returning value of token
    std::string  find_token(char *text, int& charpointer, std::string);

  public:
    int num_Slotframe;
    Tsch_Slotframe & get_Tsch_param();
    int get_Tsch_num_Slotframes();


//    struct Tsch_Slotframes {
//    };

//    static int testing_struct(Tsch_Link &link);

};

} // namespace inet

#endif // ifndef __INET_TschParser_H
