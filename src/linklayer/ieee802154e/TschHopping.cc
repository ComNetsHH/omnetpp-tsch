/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Leo Krueger
 *           (C) 2004-2006 Andras Varga
 *           (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

#include "TschHopping.h"
#include "inet/common/InitStages.h"
#include <omnetpp/cstringtokenizer.h>

namespace tsch {

Define_Module(TschHopping);

#define CHANNEL_MIN 11
#define CHANNEL_MAX 26
#define FREQ_CHANNEL_MIN 2.405e9
#define CHANNEL_SPACING 5.0e6

TschHopping::TschHopping() {}

TschHopping::~TschHopping() {}

void TschHopping::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        const char *patternstr = par("pattern").stringValue();
        pattern = omnetpp::cStringTokenizer(patternstr).asIntVector();

        // at least one channel in hopping sequence
        assert(pattern.size() >= 1);
    }
}

int TschHopping::channel(int64_t asn, int channelOffset)
{
    assert(asn >= 0);
    assert(channelOffset >= 0);

    return pattern[((asn + channelOffset) % pattern.size())];
}

double TschHopping::frequency(int channel)
{
   assert(channel >= CHANNEL_MIN);
   assert(channel <= CHANNEL_MAX);

   return FREQ_CHANNEL_MIN + ((channel - CHANNEL_MIN) * CHANNEL_SPACING);
}

} // namespace inet

