/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TschParser.h"


namespace inet{

// Constants
const int MAX_FILESIZE = 100000;    // TBD lift hardcoded limit

// Methods / Functions
int TschParser::get_Tsch_num_Slotframes(){
    return  num_Slotframe;
};

TschParser::TschParser(){
    for(int i = 0; i < 50 ;i++){
        Slotframe[i].handle = 0;
        Slotframe[i].macSlotframeSize = 0;
        Slotframe[i].numLinks = 0;
        for(int j = 0; j < 50; j++){
            Slotframe[i].links[j].Neighbor_address = "";
            Slotframe[i].links[j].Neighbor_path = "";
            Slotframe[i].links[j].Option_rx = true;
            Slotframe[i].links[j].Option_shared = true;
            Slotframe[i].links[j].Option_timekeeping = false;
            Slotframe[i].links[j].Option_tx = true;
            Slotframe[i].links[j].SlotOffset = 0;
            Slotframe[i].links[j].Type_advertising = false;
            Slotframe[i].links[j].Type_advertisingOnly = false;
            Slotframe[i].links[j].Type_normal = true;
            Slotframe[i].links[j].Virtual_id = 0;
            Slotframe[i].links[j].channelOffset = 0;
        }
    }
}


TschParser::~TschParser() {

}

//TschParser::Tsch_Slotframe  TschParser::get_Tsch_param(){
//    return Slotframe;
//};


int TschParser::streq(const char *str1, const char *str2)
{
    return strncmp(str1, str2, strlen(str2)) == 0;
}

std::string TschParser::find_token(char *text, int& charpointer, std::string token){

    std::string str="";
    // conversion of string 2 char*
    char * str_char_wri = new char[token.size() + 1];
    std::copy(token.begin(), token.end(), str_char_wri);
    str_char_wri[token.size()] = '\0';

    // forwinding until token
    charpointer=charpointer+strlen(str_char_wri);
    while (text[charpointer] != '\"'){
        str=str+text[charpointer];
        charpointer++;
    }
    delete[] str_char_wri;
    return str;
}


int TschParser::extract_Tsch_param(Tsch_Slotframe &Slotframe, int tsch_token, char *text, int& charpointer) {

    std::string str, neighbor_path="", neighbor_address="", token;
    bool option_tx = true, option_rx = true, option_shared = true, type_normal = true;
    bool option_timekeeping = false, type_advertising = false, type_advertisingOnly = false;
    int handle = 0;
    int macSlotframeSize = 0;
    int slotOffset = 0, channelOffset = 0;
    int virtual_id = -2; // -2 indicates no virtualLinkID

    bool show=false; // show results 4 "Printf-Debugging"

    switch (tsch_token) {
        case 1:  // Slotframe
                 // Identify & extract slotframe-data:
                 if (streq(text + charpointer, "Slotframe")==1){
                    while ((text[charpointer] != '\n')) {
                        charpointer++ ;
                        if (streq(text + charpointer, "handle=\"")==1){
                            str=find_token(text, charpointer, "handle=\"");
                            handle=std::stoi(str);
                        }
                        if (streq(text + charpointer, "macSlotframeSize=\"")==1){
                            str=find_token(text, charpointer, "macSlotframeSize=\"");
                            macSlotframeSize=std::stoi(str);
                            if (show){ printf("\nSlotframe: handle=%d, macSlotframeSize=%d\n",handle, macSlotframeSize);};
                        }
                    } // end of line
                 }
                 Slotframe.handle=handle;
                 Slotframe.macSlotframeSize=macSlotframeSize;

                 break;

        case 2:  // Link
                 // Identify & extract Link-data:
                 if (streq(text + charpointer, "Link")==1){
                    while ((streq(text + charpointer, "/Link")!=1)) {
                        //str.clear();
                        charpointer++ ;
                        if (streq(text + charpointer, "slotOffset=\"")==1){
                            str=find_token(text, charpointer, "slotOffset=\"");
                            slotOffset=std::stoi(str);
                            //Slotframe.links[Slotframe.numLinks].SlotOffset=std::stoi(str);
                        }
                        if (streq(text + charpointer, "channelOffset=\"")==1){
                            str=find_token(text, charpointer, "channelOffset=\"");
                            channelOffset=std::stoi(str);
                            //Slotframe.links[Slotframe.numLinks].channelOffset=std::stoi(str);
                        }
                        if (streq(text + charpointer, "tx=\"")==1){
                            str=find_token(text, charpointer, "tx=\"");
                            option_tx= (str=="true")? true : false;
                            //Slotframe.links[Slotframe.numLinks].Option_tx= (str=="true")? true : false;
                        }
                        if (streq(text + charpointer, "rx=\"")==1){
                            str=find_token(text, charpointer, "rx=\"");
                            option_rx= (str=="true")? true : false;
                            //Slotframe.links[Slotframe.numLinks].Option_rx= (str=="true")? true : false;
                        }
                        if (streq(text + charpointer, "shared=\"")==1){
                            str=find_token(text, charpointer, "shared=\"");
                            option_shared= (str=="true")? true : false;
                            //Slotframe.links[Slotframe.numLinks].Option_shared=(str=="true")? true : false;
                        }
                        if (streq(text + charpointer, "timekeeping=\"")==1){
                            str=find_token(text, charpointer, "timekeeping=\"");
                            option_timekeeping= (str=="true")? true : false;
                            //Slotframe.links[Slotframe.numLinks].Option_timekeeping= (str=="true")? true : false;
                        }
                        if (streq(text + charpointer, "normal=\"")==1){
                            str=find_token(text, charpointer, "normal=\"");
                            type_normal= (str=="true")? true : false;
                            //Slotframe.links[Slotframe.numLinks].Type_normal=(str=="true")? true : false;
                        }
                        if (streq(text + charpointer, "advertising=\"")==1){
                            str=find_token(text, charpointer, "advertising=\"");
                            type_advertising= (str=="true")? true : false;
                            //Slotframe.links[Slotframe.numLinks].Type_advertising=(str=="true")? true : false;
                        }
                        if (streq(text + charpointer, "advertisingOnly=\"")==1){
                            str=find_token(text, charpointer, "advertisingOnly=\"");
                            type_advertisingOnly= (str=="true")? true : false;
                            //Slotframe.links[Slotframe.numLinks].Type_advertisingOnly=(str=="true")? true : false;
                        }
                        if (streq(text + charpointer, "id=\"")==1){
                            str=find_token(text, charpointer, "id=\"");
                            virtual_id=std::stoi(str);
                            //Slotframe.links[Slotframe.numLinks].Virtual_id=std::stoi(str);
                        }
                        if (streq(text + charpointer, "path=\"")==1){
                            str=find_token(text, charpointer, "path=\"");
                            neighbor_path=str;
                            //Slotframe.links[Slotframe.numLinks].Neighbor_path= (neighbor_path.empty()==true)? "": str;
                            //std::cout << neighbor_path << endl;
                        }
                        if (streq(text + charpointer, "address=\"")==1){
                            str=find_token(text, charpointer, "address=\"");
                            //std::cout << str << endl;
                            neighbor_address=str;
                            //Slotframe.links[Slotframe.numLinks].Neighbor_address= (neighbor_address.empty()==true)? "00:00:00:00:00:00": str;

                        }
                    }
                    if (show){
                       printf("\n====================================>> numLinks: %d\n",Slotframe.numLinks);
                       printf("(neighbor_address.empty()==true): %d\n",(neighbor_address.empty()==true));
                       printf("(neighbor_path.empty()==true): %d\n",(neighbor_path.empty()==true));
                    }
                    // pigeon-hole all the stuff:
                    Slotframe.links[Slotframe.numLinks].SlotOffset=slotOffset;
                    Slotframe.links[Slotframe.numLinks].channelOffset=channelOffset;
                    Slotframe.links[Slotframe.numLinks].Option_tx=option_tx;
                    Slotframe.links[Slotframe.numLinks].Option_rx=option_rx;
                    Slotframe.links[Slotframe.numLinks].Option_shared=option_shared;
                    Slotframe.links[Slotframe.numLinks].Option_timekeeping=option_timekeeping;
                    Slotframe.links[Slotframe.numLinks].Type_normal=type_normal;
                    Slotframe.links[Slotframe.numLinks].Type_advertising=type_advertising;
                    Slotframe.links[Slotframe.numLinks].Type_advertisingOnly=type_advertisingOnly;
                    Slotframe.links[Slotframe.numLinks].Virtual_id=virtual_id;
                    Slotframe.links[Slotframe.numLinks].Neighbor_path= (neighbor_path.empty()==true)? "": neighbor_path;
                    Slotframe.links[Slotframe.numLinks].Neighbor_address= (neighbor_address.empty()==true)? "00:00:00:00:00:00": neighbor_address;
                    // Show results:
                    if (show){
                       printf("\nLink:           slotOffset=%d, channelOffset=%d\n",Slotframe.links[Slotframe.numLinks].SlotOffset, Slotframe.links[Slotframe.numLinks].channelOffset);
                       std::cout <<"Link_Option  :  tx=" << std::boolalpha << option_tx <<", rx=" << std::boolalpha << option_rx << std::endl;
                       std::cout <<"                shared=" << std::boolalpha << option_shared <<", timekeeping=" << std::boolalpha
                                 << option_timekeeping << std::endl;
                       std::cout <<"Link_Type    :  normal=" << std::boolalpha << type_normal <<", advertising=" << std::boolalpha << type_advertising << std::endl;
                       std::cout <<"                advertisingOnly=" << std::boolalpha << type_advertisingOnly << std::endl;
                       std::cout <<"Link_Virtual :  id=" << virtual_id << std::endl;
                       std::cout <<"Link_Neighbor:  path=" << Slotframe.links[Slotframe.numLinks].Neighbor_path <<", address=" << Slotframe.links[Slotframe.numLinks].Neighbor_address << std::endl;
                    }
                 }
                 break;
        default: throw cRuntimeError("Unknown token in Tsch_Schedule xml file");
    }
    return 0;
}

int TschParser::readTschParmFromXmlFile(const char *filename)
{
    FILE *fp;
    //TODO: num_Slotframe is only declared and not defined, probably will lead to errors
    int charpointer;
    int num_Links = 0;
    num_Slotframe=0;
    //TschParser::Tsch_Link link[100]; // To be decided: storage of values in Tsch_Link struct??
    //TschParser::Tsch_Slotframe Slotframe[100]; // To be decided: storage of values in Tsch_Link struct??

    char *file = new char[MAX_FILESIZE];
    int c, last_c, n;
    bool show=false;

    // read the whole xml document into file[] char-array and erase "<", "/>"
    // and ">" at the same time
    fp = fopen(filename, "r");
    if (fp == nullptr)
        throw cRuntimeError("Error opening xml file `%s'", filename);

    for (charpointer = 0; (c = getc(fp)) != EOF; charpointer++){
        if ((charpointer == 0 || file[charpointer - 1] == '\n')){
            while ( isspace(c) || c=='<')
                c=getc(fp);
        }
        if  (c == '>'){
            c=getc(fp);
            if  (last_c=='/')
                charpointer--;
        }
        file[charpointer] = c;
        last_c=c;
    }
    for ( ; charpointer < MAX_FILESIZE; charpointer++)
        file[charpointer] = '\0';

    fclose(fp);
    // plain cleaned text is now in char array named file:
    //
    // Extract TSCH-Parameter from file
    for (charpointer = 0;
        (charpointer < MAX_FILESIZE) && (file[charpointer] != '\0');
         charpointer++){
        // check for tokens (Slotframe and Link) at beginning of the line:
        //
        if ((charpointer == 0 || file[charpointer - 1] == '\n')) {
            // 1)
            // Extract values of  Slotframe: handle, macSlotframeSize
            if (streq(file + charpointer, "Slotframe")==1){
                num_Links=0;
                extract_Tsch_param(Slotframe[num_Slotframe], 1, file, charpointer);

               //num_Slotframe++
            }
            // 2)
            // Extract values of Link:  SlotOffset, channelOffset
            //                          Option tx, Option_rx, Option_shared=, Option_timekeeping
            //                          Type_normal, Type_advertising, Type_advertisingOnly
            //                          Virtual id
            //                          Neighbor_path / Neighbor_address
            if (streq(file + charpointer, "Link")==1){
               extract_Tsch_param(Slotframe[num_Slotframe], 2, file, charpointer);
               num_Links++;
               Slotframe[num_Slotframe].numLinks=num_Links;
               //printf("Number of Links found: %d, Slotframe[num_Slotframe].numLinks=%d\n",num_Links, Slotframe[num_Slotframe].numLinks);
            }
            if (streq(file + charpointer, "/Slotframe")==1){
               num_Slotframe++;
            }
            // terminate procedure when /TSCHSchedule is reached
            if (streq(file + charpointer, "/TSCHSchedule")==1){
               break;
            }
        }
    }
    if (show){

       int Num_Slotframe=num_Slotframe;
       for (num_Slotframe=0; num_Slotframe < Num_Slotframe; num_Slotframe++){
           printf("============================================================================================\n");
           printf("\nnum_Slotframe = %d,  numLinks=%d\n", num_Slotframe, Slotframe[num_Slotframe].numLinks);
           printf("============================================================================================\n");
           printf("Slotframe[num_Slotframe].handle :            %d\n",Slotframe[num_Slotframe].handle);
           printf("Slotframe[num_Slotframe].macSlotframeSize:   %d\n",Slotframe[num_Slotframe].macSlotframeSize);

           for (n=0; n < Slotframe[num_Slotframe].numLinks; n++){
               printf("\nLink No: %d      slotOffset=%d, channelOffset=%d\n",n+1, Slotframe[num_Slotframe].links[n].SlotOffset,
                     Slotframe[num_Slotframe].links[n].channelOffset);
               std::cout <<"Link_Option  :  tx=" << std::boolalpha << Slotframe[num_Slotframe].links[n].Option_tx <<", "
                         "rx=" << std::boolalpha << Slotframe[num_Slotframe].links[n].Option_rx << std::endl;
               std::cout <<"                shared=" << std::boolalpha << Slotframe[num_Slotframe].links[n].Option_shared <<", timekeeping=" << std::boolalpha
                         << Slotframe[num_Slotframe].links[n].Option_timekeeping << std::endl;
               std::cout <<"Link_Type    :  normal=" << std::boolalpha << Slotframe[num_Slotframe].links[n].Type_normal <<", advertising=" << std::boolalpha
                         << Slotframe[num_Slotframe].links[n].Type_advertising << std::endl;
               std::cout <<"                advertisingOnly=" << std::boolalpha << Slotframe[num_Slotframe].links[n].Type_advertisingOnly << std::endl;
               std::cout <<"Link_Virtual :  id=" << Slotframe[num_Slotframe].links[n].Virtual_id << std::endl;
               std::cout <<"Link_Neighbor:  path=" << Slotframe[num_Slotframe].links[n].Neighbor_path <<", address="
                         << Slotframe[num_Slotframe].links[n].Neighbor_address << std::endl;
           }
       }
    }
    delete[] file;
    return num_Slotframe;
} // readTschParmFromXmlFile
} // namespace inet
