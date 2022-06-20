//
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalAnalogModel.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalNoise.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalSnir.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalTransmission.h"
#include "inet/physicallayer/common/packetlevel/BandListening.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"
#include "WaicDimensionalAnalogModel.h"
#include "inet/mobility/static/StationaryMobility.h"

using namespace inet;

using namespace physicallayer;

namespace tsch {

Define_Module(WaicDimensionalAnalogModel);

void WaicDimensionalAnalogModel::initialize(int stage)
{
    DimensionalAnalogModelBase::initialize(stage);
    EV_DEBUG << "Initializing stage" << stage << endl;
    if (stage == INITSTAGE_LAST) {
        //===========================================================================================================
        // Altimeter locations defined by: *.radioAltimeter[1].mobility.initialX /Y /Z = ... in ini-file
        // Collect all Altimeter locations involved into vector:
        // V_AltimeterLocation= getAltimeterLocation_v();  // Is considered obsolete ==> to many useless interference
                                                           // calculations between Altimeters...
         //===========================================================================================================
        // Altimeter locations defined by string "AltimeterLocations" in  WaicDimensionalAnalogModel.ned and ini-file
        // Read locations of radio altimeters into vector:
        V_AltimeterLocation= getAltimeterLocation_v_str();

        //===========================================================================================================
        // Read time offsets of radio altimeters into vector:
        RaOffSet=getRaOffSet();

        //===========================================================================================================
        // Read frequency shifts of radio altimeters into vector:
        Ra_Freq_OffSet=getRa_Freq_OffSet();

        //===========================================================================================================
        // Read chirp parameters of radio altimeters:
        T_chirp=par("T_chirp");
        f_chirp_min=par("f_chirp_min");
        f_chirp_max=par("f_chirp_max");
    }
}

const INoise *WaicDimensionalAnalogModel::computeNoise(const IListening *listening, const IInterference *interference) const
{
    EV_DEBUG << "==============>> Inside computeNoise 1: Interference" << endl;

    const BandListening *bandListening = check_and_cast<const BandListening *>(listening);
    Hz centerFrequency = bandListening->getCenterFrequency();
    Hz bandwidth = bandListening->getBandwidth();
    std::vector<Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>> receptionPowers;
    const DimensionalNoise *dimensionalBackgroundNoise = check_and_cast_nullable<const DimensionalNoise *>(interference->getBackgroundNoise());
    if (dimensionalBackgroundNoise) {
        const auto& backgroundNoisePower = dimensionalBackgroundNoise->getPower();
        receptionPowers.push_back(backgroundNoisePower);
    }
    const std::vector<const IReception *> *interferingReceptions = interference->getInterferingReceptions();

    for (const auto & interferingReception : *interferingReceptions) {
        const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(interferingReception);
        auto receptionPower = dimensionalReception->getPower();
        receptionPowers.push_back(receptionPower);
    }

    //==================================================================================================
    // Radio Altimeter (RA) Contribution of Interference in case of spectral overlap:
    // Time duraton of RA-Interference-Forecast:
    simsec startTime = simsec(listening->getStartTime());
    simsec endTime = simsec(listening->getEndTime());

    // should be zero, which causes trouble in division -> choosen value close enough to zero
    WpHz altimeterPowerSpectralDensity = WpHz(1e-30);
    // Check all participating RAs
    for (int k=0; k < V_AltimeterLocation.size(); k++){
        EV_DEBUG << "V_AltimeterLocation[" <<k <<"] = " << V_AltimeterLocation[k]  << endl;

        auto distToAltimeter = listening->getEndPosition().distance(V_AltimeterLocation[k]);

        if (isAltimeterInterfering(centerFrequency,  bandwidth, startTime, endTime, toDouble(RaOffSet[k]),Ra_Freq_OffSet[k])) {
            EV_DEBUG << "==============>> Altimeter is interfering:" << endl;
            altimeterPowerSpectralDensity += WpHz(AltimeterInterferingPower(distToAltimeter)/bandwidth.get());
            EV_DEBUG << "distance - " << distToAltimeter <<  ", power spectral density = " << altimeterPowerSpectralDensity << endl;

        }
    }

    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& altimeterPower =
        makeShared<ConstantFunction<WpHz, Domain<simsec, Hz>>>(altimeterPowerSpectralDensity);
    receptionPowers.push_back(altimeterPower);

    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisePower = makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(receptionPowers);
    const auto& bandpassFilter = makeShared<Boxcar2DFunction<double, simsec, Hz>>(simsec(listening->getStartTime()), simsec(listening->getEndTime()), centerFrequency - bandwidth / 2, centerFrequency + bandwidth / 2, 1);

    return new DimensionalNoise(listening->getStartTime(), listening->getEndTime(), centerFrequency, bandwidth, noisePower->multiply(bandpassFilter));
}

const Coord& WaicDimensionalAnalogModel::getAltimeterLocation() const {
    std::vector<Coord> V_AltimeterLocation;
    // Find NED network module
    auto simul = cModule::getParentModule()->getParentModule();

    // Find altimeter node (to be refactored with more efficient search later)
    cModule *altimeter;
    for (cModule::SubmoduleIterator it(simul); !it.end(); ++it) {
        altimeter = *it;
        std::string submFullName(altimeter->getFullName());

        if (submFullName.find(std::string("Altimeter")) != std::string::npos) {
           auto altiMobility = check_and_cast<StationaryMobilityBase*> (altimeter->getSubmodule("mobility"));
           auto altimeterLocation = new Coord(altiMobility->getCurrentPosition());
           V_AltimeterLocation.push_back(*altimeterLocation);
        }
    }

    for (auto i = V_AltimeterLocation.begin(); i != V_AltimeterLocation.end(); ++i)
        EV_DEBUG << *i << " \n";

    // In case no altimeter was found, the previous iterator will just return the last module,
    // so need to double-check
    std::string altiName(altimeter->getFullName());

    if (altiName.find(std::string("Altimeter")) == std::string::npos) {
        EV_WARN << "Altimeter not found, returning (0, 0, 0) position" << endl;
        return *(new Coord());
    }

    auto altiMobility = check_and_cast<StationaryMobilityBase*> (altimeter->getSubmodule("mobility"));
    auto altimeterLocation = new Coord(altiMobility->getCurrentPosition());

    return *altimeterLocation;
}

const std::vector<Coord> WaicDimensionalAnalogModel::getAltimeterLocation_v() const {

    EV_DEBUG << "==============>> Inside getAltimeterLocation_v" << endl;
    std::vector<Coord> V_AltimeterLocation;
    // Find NED network module
    auto simul = cModule::getParentModule()->getParentModule();

    // Find altimeter node (to be refactored with more efficient search later)
    cModule *altimeter;
    for (cModule::SubmoduleIterator it(simul); !it.end(); ++it) {
        altimeter = *it;
        std::string submFullName(altimeter->getFullName());

        if (submFullName.find(std::string("Altimeter")) != std::string::npos) {
           auto altiMobility = check_and_cast<StationaryMobilityBase*> (altimeter->getSubmodule("mobility"));
           auto altimeterLocation = new Coord(altiMobility->getCurrentPosition());
           V_AltimeterLocation.push_back(*altimeterLocation);
           //EV_DEBUG << "==============>> *altimeterLocation = " <<*altimeterLocation << endl;
        }
    }

    for (auto i = V_AltimeterLocation.begin(); i != V_AltimeterLocation.end(); ++i)
        EV_DEBUG << *i << " \n";

    // In case no altimeter was found, the previous iterator will just return the last module,
    // so need to double-check
    std::string altiName(altimeter->getFullName());

    if (altiName.find(std::string("Altimeter")) == std::string::npos) {
        EV_WARN << "Altimeter not found, returning (0, 0, 0) position" << endl;
        V_AltimeterLocation.push_back(*(new Coord()));
        return V_AltimeterLocation;
        //return *(new Coord());
    }

    auto altiMobility = check_and_cast<StationaryMobilityBase*> (altimeter->getSubmodule("mobility"));
    auto altimeterLocation = new Coord(altiMobility->getCurrentPosition());

//  EV_DEBUG << "==============>> *altimeterLocation = " <<*altimeterLocation << endl;
//    return *altimeterLocation;
    return V_AltimeterLocation;
}

const std::vector<Coord> WaicDimensionalAnalogModel::getAltimeterLocation_v_str() const {
    //===========================================================================================================
    // Altimeter locations defined by string "AltimeterLocations" in  WaicDimensionalAnalogModel.ned and ini-file
    // Read locations of radio altimeters into vector:
    std::vector<std::string> V_AltimeterLocation_testSTR = cStringTokenizer(par("AltimeterLocations")).asVector();
    std::vector<Coord> V_AltimeterLocation;

    for (auto i : V_AltimeterLocation_testSTR) {
        std::string str, STR;
        int xyz_c=0;
        Coord c;

        for (int jj=0; jj < i.size(); jj++){
            //EV <<"i[" <<jj <<"]=" <<i[jj] <<endl;
            STR=(i[jj]);
            if (STR=="("){
            }
            else{
                if ((STR==",") | (STR==")") ){
                   //EV <<"=================>>   str=" <<str <<endl;
                   xyz_c++;
                   if (xyz_c==1)
                      c.x=atof(str.c_str());
                   if (xyz_c==2)
                      c.y=atof(str.c_str());
                   if (xyz_c==3){
                      c.z=atof(str.c_str());
                      //EV <<"xyz_c=3, c=" <<c <<endl;
                      xyz_c=0;
                   }
                   str.clear();
                }
                else{
                    str.append(STR);  // ==>> (i[jj]); <<== does not work -> Type confusion??
                }
            }
        }
        // store Altimeter locations in vector:
        V_AltimeterLocation.push_back(c);
    }

    for (int k=0; k < V_AltimeterLocation.size(); k++){
        EV_DEBUG << "V_AltimeterLocation[" <<k <<"] = " << V_AltimeterLocation[k]  << endl;
    }
    //===========================================================================================================

    return V_AltimeterLocation;
}

const std::vector<simtime_t>  WaicDimensionalAnalogModel::getRaOffSet() const{
    //===========================================================================================================
    // Read time offsets of radio altimeters into vector:
    std::vector<simtime_t> RaOffSet;
    std::vector<std::string> RaOffSetStr = cStringTokenizer(par("RA_OffSet")).asVector();

    for (auto i : RaOffSetStr) {
        simtime_t t = simtime_t::parse(i.c_str());
        RaOffSet.push_back(t);
    }
    EV << "Read " << RaOffSet.size() << " parameters from \"RaOffSet\"" << std::endl;
    // show all read values
    for (int j=0; j<RaOffSet.size(); ++j) {
       EV <<"RaOffSet[" << j <<"] = " << RaOffSet[j]  << endl;
}
//===========================================================================================================

return RaOffSet;
}


const std::vector<double>  WaicDimensionalAnalogModel::getRa_Freq_OffSet() const{
    //===========================================================================================================
    // Read frequency shifts of radio altimeters into vector:
    std::vector<double> Ra_Freq_OffSet = cStringTokenizer(par("RA_Freq_OffSet")).asDoubleVector();

    EV << "Read " << Ra_Freq_OffSet.size() << " parameters from \"Ra_Freq_OffSet\"" << std::endl;
    // show all read values
    for (int j=0; j<Ra_Freq_OffSet.size(); ++j) {
       EV <<"Ra_Freq_OffSet[" << j <<"] = " << Ra_Freq_OffSet[j]  << endl;
}
//===========================================================================================================

return Ra_Freq_OffSet;
}

const bool WaicDimensionalAnalogModel::isAltimeterInterfering(Hz centerFrequency,  Hz bandwidth, simsec startTime, simsec endTime, double RA_OffSet, double Ra_Freq_OffSet) const {

    EV_DEBUG << "==============>> Inside isAltimeterInterfering" << endl;

    bool Interference_Impact_In_T_forecast=false;

    // Altimeter Impact:
    double Delta_F,
           f_0_0=0.0,
           f_1_0=0.0,
           f_0_1=0.0,
           f_1_1=0.0,
           current_channel_freq=centerFrequency.get(),
           channel_BW=bandwidth.get(),
           T_forecast,
           t_start=toDouble(startTime),
           t_end= toDouble(endTime),
           t_mod_start,
           t_mod_stop;

    T_forecast = t_end - t_start;

    // Calculate actual Frequncy Range(s) of Altimeter within the time intervall t_start + T_forecast + T_Offset
    Delta_F = f_chirp_max - f_chirp_min;
    t_mod_start=fmod(t_start+RA_OffSet,T_chirp);
    t_mod_stop=fmod(t_start+RA_OffSet+T_forecast,T_chirp);
    if (T_forecast >= T_chirp){
       // single box - full range:
       // EV_DEBUG <<"==========>> single box - full range \n";
       f_0_0=f_chirp_min+Ra_Freq_OffSet;
       f_1_0=f_chirp_max+Ra_Freq_OffSet;
    }
    else{
       if (t_mod_stop < t_mod_start){
           //EV_DEBUG <<"==========>> two boxes \n";
           f_0_0=f_chirp_min+Ra_Freq_OffSet;
           f_1_0=f_chirp_min+Ra_Freq_OffSet+Delta_F/T_chirp*t_mod_stop;
           f_0_1=f_chirp_min+Ra_Freq_OffSet+Delta_F/T_chirp*t_mod_start;
           f_1_1=f_chirp_max+Ra_Freq_OffSet;
       }
       else{
           //EV_DEBUG <<"==========>> single box - extenuated range \n";
           f_0_0=f_chirp_min+Ra_Freq_OffSet+Delta_F/T_chirp*t_mod_start;
           f_1_0=f_chirp_min+Ra_Freq_OffSet+Delta_F/T_chirp*t_mod_stop;
       }
    }

    // Is the actual frequency range of the RA crossing the current channel?
    if ( (((current_channel_freq+channel_BW/2.0) >= f_0_0 ) &&
         (f_1_0 >= (current_channel_freq-channel_BW/2.0)) ) |
         (((current_channel_freq+channel_BW/2.0) >= f_0_1 ) &&
         (f_1_1 >= (current_channel_freq-channel_BW/2.0)) ) ){
         Interference_Impact_In_T_forecast=true;
         //EV_DEBUG <<"=====================================>> RA Interference exists!" <<endl;
    }
    else {
         //EV_DEBUG <<"=====================================>> RA Interference does not exists!" <<endl;
         Interference_Impact_In_T_forecast=false;
    }

    EV_DEBUG <<"========================================================================" << endl;
    EV_DEBUG <<"isAltimeterInterfering(...):  Interference_Impact_In_T_forecast = " << Interference_Impact_In_T_forecast << endl;
    //EV_DEBUG <<"D = " << D <<",  d = " << d <<endl;
    EV_DEBUG <<"========================================================================" << endl;
    EV_DEBUG <<"Actual transmission:"<< endl;
    EV_DEBUG <<"startime        = " << t_start << endl;
    EV_DEBUG <<"endtime         = " << t_start+T_forecast << endl << endl;
    EV_DEBUG <<"centerFrequency = " << centerFrequency << endl;
    EV_DEBUG <<"bandwidth       = " << bandwidth << endl;
    //EV_DEBUG <<"channel_BW      = " << channel_BW << endl;
    EV_DEBUG <<"------------------------------------------------------------------------" << endl;
    EV_DEBUG <<"Actual RA:"<< endl;
    EV_DEBUG <<"T_chirp         = " << T_chirp  <<" s" << endl;
    EV_DEBUG <<"f_chirp_min     = " << f_chirp_min <<" Hz" << endl;
    EV_DEBUG <<"f_chirp_max     = " << f_chirp_max <<" Hz" << endl;
    EV_DEBUG <<"RA_OffSet       = " << RA_OffSet <<" s" << endl;
    EV_DEBUG <<"Ra_Freq_OffSet  = " << Ra_Freq_OffSet  <<" Hz" << endl;
    //EV_DEBUG <<"------------------------------------------------------------------------" << endl;
    EV_DEBUG <<"T_forecast      = " << T_forecast  <<" s" << endl;
    EV_DEBUG <<"Delta_F         = " << Delta_F/1e6 <<" MHz" << endl << endl;
    EV_DEBUG <<"f_0_0 = " << f_0_0/1e6 <<" MHz, f_1_0 = " << f_1_0/1e6 <<" MHz" << endl;
    EV_DEBUG <<"f_1_0 - f_0_0 = " << (f_1_0 - f_0_0)/1e6 <<" MHz"<< endl;
    EV_DEBUG <<"f_0_1=" << f_0_1/1e6 <<" MHz, f_1_1=" << f_1_1/1e6 <<" MHz" << endl;
    EV_DEBUG <<"f_1_1 - f_0_1 = " << (f_1_1 - f_0_1)/1e6 <<" MHz" << endl;
    EV_DEBUG <<"========================================================================" << endl;

    return Interference_Impact_In_T_forecast;
}


const double WaicDimensionalAnalogModel::AltimeterInterferingPower(double distance) const {

    double ipl_dB=0, IPL_fun_coe_a=1.5959,
           IPL_fun_coe_b=83.7078, P_RA_dBm=27.78,
            P_rx_lin, P_rx_dB;

    ipl_dB=IPL_fun_coe_a * distance + IPL_fun_coe_b;
    P_rx_dB=P_RA_dBm - ipl_dB;
    P_rx_lin=pow(10.0, P_rx_dB/10.0);

    EV_DEBUG <<"========================================================================" << endl;
    EV_DEBUG <<"Hello from AltimeterInterferingPower, distance to Altimeter is: " << distance << endl;
    EV_DEBUG <<"P_rx_dB = " << P_rx_dB <<" dB, P_rx_lin = " << P_rx_lin << endl;
    EV_DEBUG <<"========================================================================" << endl;

    return P_rx_lin;
}

}



