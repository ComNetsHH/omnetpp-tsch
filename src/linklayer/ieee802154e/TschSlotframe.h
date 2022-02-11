/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2021  Yevhenii Shudrenko
 *           (C) 2019  Leo Krueger
 *           (C) 2004-2006 Andras Varga
 *           (C) 2000  Institut fuer Telematik, Universitaet Karlsruhe
 *           (C) 2000-2001 Jochen Reber, Vincent Oberle
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

#ifndef __TSCH_TSCHSLOTFRAME_H
#define __TSCH_TSCHSLOTFRAME_H

#include <vector>

#include <omnetpp.h>
#include "inet/common/lifecycle/ILifecycle.h"
#include "TschLink.h"
#include "sixtisch/WaicCellComponents.h"
#include "TschVirtualLink.h"


using namespace omnetpp;
using namespace inet;

namespace tsch {

/**
 *
 */
class TschSlotframe : public cSimpleModule, protected cListener, public ILifecycle
{
  protected:
    int macSlotframeSize;
    int macSlotframeHandle;
    typedef std::vector<TschLink *> LinkVector;

  private:
    // Subclasses should use internalAddLink() and internalRemoveLink() methods
    // to modify the vectors storing links, but they can not access them directly.
    LinkVector links;

    const char* fp;

  protected:

    // displays summary above the icon
    virtual void refreshDisplay() const override;

    // helper for sorting routing table, used by addRoute()
    class LinkLessThan
    {
        const TschSlotframe &c;
      public:
        LinkLessThan(const TschSlotframe& c) : c(c) {}
        bool operator () (const TschLink *a, const TschLink *b) { return c.linkLessThan(a, b); }
        //bool operator () (const TschVirtualLink *a, const TschVirtualLink *b) { return c.linkLessThan(a, b); }
    };
    bool linkLessThan(const TschLink *a, const TschLink *b) const;

    //bool linkLessThan(const TschVirtualLink *a, const TschVirtualLink *b) const;

    // helper functions:
    void internalAddLink(TschLink *entry);

    //void internalAddLink(TschVirtualLink *entry);

    TschLink *internalRemoveLink(TschLink *entry);

    //TschVirtualLink *internalRemoveLink(TschVirtualLink *entry);

  public:
    TschSlotframe() {}
    virtual ~TschSlotframe();

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    /**
     * Raises an error.
     */
    virtual void handleMessage(cMessage *) override;

    /**
     * Called by the signal handler whenever a change of a category
     * occurs to which this client has subscribed.
     */
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    inline int getOffsetFromASN(int64_t asn) { return asn % macSlotframeSize; };

    /**
     * Returns iterator to the link scheduled for the ASN given
     * or end() if nothing is scheduled.
     */
    std::vector<TschLink *>::iterator getLinkFromASNInternal(int64_t asn);

    std::vector<TschLink *>::iterator getNextLinkFromASNInternal(int64_t asn);

  public:
    /**
     * For debugging
     */
    virtual void printSlotframe() const;

    /**
     * Returns the total number of links
     */
    virtual int getNumLinks() const { return links.size(); }

    virtual LinkVector getLinks() const { return links; }

    TschLink* getLinkByCellCoordinates(offset_t slotOf, offset_t chOf, MacAddress neighborAddr);

    /**
     * Returns the kth link.
     */
    virtual TschLink *getLink(int k) const;

    /**
     * Returns all dedicated (non-shared) links
     */
    virtual std::vector<MacAddress> getMacDedicated();
    /**
     *
     */
    virtual void addLink(TschLink *entry);
    /**
     *
     */
    virtual void addLink(TschVirtualLink *entry);

    /**
     * Removes the given route from the routing table, and returns it.
     * nullptr is returned if the route was not in the routing table.
     */
    virtual TschLink *removeLink(TschLink *entry);


    /**
     * Removes the given route from the routing table, and delete it.
     * Returns true if the route was deleted, and false if it was
     * not in the routing table.
     */
    virtual bool deleteLink(TschLink *entry);

    /**
     * Deletes invalid routes from the routing table. Invalid routes are those
     * where the isValid() method returns false.
     */
    virtual void purge();

    /**
     * To be called from route objects whenever a field changes. Used for
     * maintaining internal data structures and firing "routing table changed"
     * notifications.
     */
    virtual void linkChanged(TschLink *entry, int fieldCode);
    /**
     * ILifecycle method
     */
    virtual bool handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback) override;

    virtual TschLink *createLink() { return new TschLink(); }

    virtual TschVirtualLink *createVirtualLink() { return new TschVirtualLink(); }

    int getMacSlotframeHandle() const {
        return macSlotframeHandle;
    }

    void setMacSlotframeHandle(int macSlotframeHandle) {
        this->macSlotframeHandle = macSlotframeHandle;
    }

    int getMacSlotframeSize() const {
        return macSlotframeSize;
    }

    void setMacSlotframeSize(int macSlotframeSize) {
        this->macSlotframeSize = macSlotframeSize;
    }

    /**
     * Returns the link scheduled for the ASN given
     * or null if nothing is scheduled.
     */
    TschLink *getLinkFromASN(int64_t asn);

    std::vector<TschLink*> getLinksFromASN(int64_t asn);

    /**
     * Get the next scheduled link considering the given ASN.
     */
    TschLink *getNextLink(int64_t asn);

    /**
     * Get the ASN of the next scheduled link.
     * Useful to suspend execution of MAC until the next scheduled link.
     */
    int64_t getASNofNextLink(int64_t asn);

    /**
     * @brief Removes the link assigned to @param cell.
     *
     * @return true is returned if the link was removed, false otherwise
     */
    bool removeLinkAtCell(cellLocation_t cell, uint64_t neighborId);

    /**
     * Checks if there is a link scheduled for the given Macaddress
     * True if found
     * False if not found
     */
    bool hasLink(inet::MacAddress macAddress);

    std::vector<TschLink*> allTxLinks(inet::MacAddress macAddress);
    std::vector<TschLink*> getDedicatedLinksForNeighbor(inet::MacAddress neigbhorMac);
    bool removeAutoLinkToNeighbor(inet::MacAddress neigbhorMac);

    void xmlSchedule();

  private:
};

} // namespace inet

#endif // ifndef __INET_IPV4ROUTINGTABLE_H

