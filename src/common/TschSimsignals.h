//
// Copyright (C) 2005 Andras Varga
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

#ifndef __TSCH_SIMSIGNALS_H
#define __TSCH_SIMSIGNALS_H

#include "inet/common/Simsignals.h"

using namespace inet;

namespace tsch {

/**
 * Signals for publish/subscribe mechanisms.
 */
extern simsignal_t
// link layer
    linkAddedSignal,
    linkDeletedSignal,
    linkChangedSignal;

} // namespace tsch

#endif // ifndef __TSCH_SIMSIGNALS_H

