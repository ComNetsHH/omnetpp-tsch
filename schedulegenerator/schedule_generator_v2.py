from random import randint
import os
import xml.etree.cElementTree as ET
from xml.dom import minidom

def mac_to_hex(mac_int):
    return "{:012x}".format(mac_int)

def mac_to_int(mac_hex):
    return int(mac_hex.translate(mac_hex.maketrans("", "", ":.- ")), 16)

def mac_to_str(mac_hex):
    return ":".join(mac_hex[i:i+2] for i in range(0, len(mac_hex), 2)).upper()

def mac_conversion_demo(sample_addr="0A:AA:00:00:00:01"):
    mac_int = mac_to_int(sample_addr)
    mac_hex = mac_to_hex(mac_int)
    mac_str = mac_to_str(mac_hex)

    print(f'int: {mac_int}, hex: {mac_hex}, str: {mac_str}')

def generate_schedule(node_id, gw_addr = "0A:AA:00:00:00:01", num_channels = 16, slotframe_size = 101, num_min_cells = 5, add_auto = True, ch_override = None):
    schedule = ET.Element("TSCHSchedule")
    sf = ET.SubElement(schedule, "Slotframe", macSlotframeSize="101")
    node_addr = mac_to_hex(mac_to_int(gw_addr) + node_id + 1)
    chof = ch_override if ch_override else randint(0, num_channels-1)

    # a dedicated link to the gateway
    if node_id >= 0: # skip the gw itself
        add_link(sf, gw_addr, node_id + 1, chof, False)

    # an auto RX one (37 is a random offset to avoid overlapping dedicated TX with auto RX cell)
    if add_auto:
        add_link(sf, mac_to_str(node_addr), (node_id + 37) % slotframe_size, chof, True, False, True, True)

    # and some minimal cells
    min_cell_slofs = [x for x in range(0, slotframe_size, int(slotframe_size / num_min_cells))]
    for i in min_cell_slofs:
        add_link(sf, "FF:FF:FF:FF:FF:FF", i, 0, True, True, True, False)

    return schedule

def add_link(xml_parent, dest, slof, chof, is_shared = False, is_tx = True, is_rx = False, is_auto = False):
    link = ET.SubElement(xml_parent, "Link", slotOffset=str(slof), channelOffset=str(chof))
    opt = ET.SubElement(link, "Option", tx=str(is_tx).lower(), rx=str(is_rx).lower(), shared=str(is_shared).lower(), auto=str(is_auto).lower())
    prio = ET.SubElement(link, "Virtual", id="0")
    ltype = ET.SubElement(link, "Type", normal="true", advertising="false", advertisingOnly="false")
    nbr = ET.SubElement(link, "Neighbor", address=dest)

def get_auto_rx_link(schedule):
    return schedule.find('.//**[@auto="true"]/..')

def get_dedicated_tx_links(schedule):
    return schedule.findall('.//**[@auto="false"][@shared="false"][@tx="true"]/..')

start_addr = "0A:AA:00:00:00:01" # gateway MAC address
node_schedules = []
num_min_cells = 1
# tsch_path = "/Users/yevhenii/omnetpp-5.6.2-new/samples/tsch/" # TODO: use environmental variables

# folder_to_save = os.path.expanduser("$tsch/simulations/wireless/waic/schedules/itnac/blacklisted-2")
folder_to_save = "./blacklisted-2"

if os.path.exists(folder_to_save):
    print("the path to save schedules is there!")
else:
    os.makedirs(folder_to_save)
    print("The new directory is created at " + folder_to_save)

# fixed channel for blacklisted schedule
ch_override = 15

################ nodes ################
num_nodes = 100
for i in range(num_nodes):
    s = generate_schedule(i, num_min_cells=num_min_cells, ch_override=ch_override)
    sf = s.find("Slotframe")
    sf[:] = sorted(sf, key=lambda child: int(child.get('slotOffset')))
    node_schedules.append(s)
    xmlstr = minidom.parseString(ET.tostring(s)).toprettyxml(indent="   ")
    with open(f"{folder_to_save}/host_{i}.xml", "w+") as f:
        f.write(xmlstr)


################ gateway ################
gw_schedule = generate_schedule(-1, num_min_cells=num_min_cells, ch_override=ch_override)
gw_sf = gw_schedule.find("Slotframe")

for i, s in enumerate(node_schedules):
    # read the auto-rx cell directly from child's schedule
    arx = get_auto_rx_link(s) 

    # calculate neighbor MAC by assuming it's index is added to the starting MAC address
    node_addr_str = mac_to_str(mac_to_hex( mac_to_int(start_addr)+i+1) ) 

    # auto-TX
    add_link(gw_sf, node_addr_str, arx.get("slotOffset"), arx.get("channelOffset"), True, True, False, True )

    # dedicated RX
    dedicated_rx = get_dedicated_tx_links(s)
    for l in dedicated_rx:
        add_link(gw_sf, node_addr_str, l.get("slotOffset"), l.get("channelOffset"), False, False, True, False)

gw_sf[:] = sorted(gw_sf, key=lambda child: int(child.get('slotOffset')))

xmlstr = minidom.parseString(ET.tostring(gw_schedule)).toprettyxml(indent="   ")
with open(f"{folder_to_save}/sink.xml", "w+") as f:
    f.write(xmlstr)
