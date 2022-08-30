from random import randint
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

def generate_schedule(node_id, gw_addr = "0A:AA:00:00:00:01", num_channels = 16, slotframe_size = 101, num_min_cells = 5, add_auto = True):
    schedule = ET.Element("TSCHSchedule")
    sf = ET.SubElement(schedule, "Slotframe", macSlotframeSize="101")
    node_addr = mac_to_hex(mac_to_int(gw_addr) + node_id + 1)

    # for each node add a dedicated link
    if node_id >= 0: # skip gateway/sink indicated by -1
        add_link(sf, gw_addr, node_id + 1, randint(0, num_channels-1), False)

    # an auto RX one (37 is a random offset to avoid overlapping dedicated TX with auto RX cell)
    if add_auto:
        add_link(sf, mac_to_str(node_addr), (node_id + 37) % slotframe_size, randint(0, num_channels-1), True, False, True, True)

    # and some minimal cells
    min_cell_slofs = [x for x in range(0, slotframe_size, int(slotframe_size / num_min_cells))]
    for i in min_cell_slofs:
        add_link(sf, "FF:FF:FF:FF:FF:FF", i, randint(0, num_channels-1), True, True, True, False)

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

################ nodes ################
num_nodes = 1
for i in range(num_nodes):
    s = generate_schedule(i, num_min_cells=num_min_cells)
    sf = s.find("Slotframe")
    sf[:] = sorted(sf, key=lambda child: int(child.get('slotOffset')))
    node_schedules.append(s)
    xmlstr = minidom.parseString(ET.tostring(s)).toprettyxml(indent="   ")
    with open(f"host_{i}.xml", "w+") as f:
        f.write(xmlstr)


################ gateway ################
gws = generate_schedule(-1, num_min_cells=num_min_cells)
gw_sf = gws.find("Slotframe")
# add an auto tx cell to each child and a dedicated RX cell
for i, s in enumerate(node_schedules):
    arx = get_auto_rx_link(s) # read the auto-rx cell from child's schedule
    node_addr_str = mac_to_str(mac_to_hex( mac_to_int(start_addr)+i+1) )
    add_link(gw_sf, node_addr_str, arx.get("slotOffset"), arx.get("channelOffset"), True, True, False, True )

    dedicated_rx = get_dedicated_tx_links(s)
    for l in dedicated_rx:
        add_link(gw_sf, node_addr_str, l.get("slotOffset"), l.get("channelOffset"), False, False, True, False)

gw_sf[:] = sorted(gw_sf, key=lambda child: int(child.get('slotOffset')))

xmlstr = minidom.parseString(ET.tostring(gws)).toprettyxml(indent="   ")
with open(f"sink.xml", "w+") as f:
    f.write(xmlstr)
