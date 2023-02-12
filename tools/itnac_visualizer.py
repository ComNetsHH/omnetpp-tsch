import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import json
import argparse
import os
import re
import sys

sys.path.append(os.path.expanduser('~/tsch_delays_modeling/results_processing'))
from tsch_utils import tsch_service_time

plt.rcParams.update({'lines.markeredgewidth': 2})
plt.rcParams.update({'errorbar.capsize': 22.5})
plt.rcParams.update({'font.size': 24})

# Taken from Seaborn's "Paired" palette
palette = {
    'CSMA (up)': '#a6cee3',          # lightblue
    'CSMA (down)': '#1f78b4',        # blue
    'TSCH (up)': '#b2df8a',          # ligthgreen
    'TSCH (down)': '#33a02c',        # green
    'TSCH (expected)': '#fb9a99',    # pink
    'TSCH (blacklisted)': '#e31b1c', # red
}

itnac_imgs_path = os.path.expanduser("~/yevhenii_koo_tsch_survey/imgs")

parser = argparse.ArgumentParser(description='Plotting ITNAC simualtion results.')
parser.add_argument('result_file', metavar='f', type=str, help='path to the json file with exported simulation results')
parser.add_argument('-pdr', '--pdr', help='plot PDR', action='store_true')
parser.add_argument('-d', '--delay', help='plot e2e delay', action='store_true')
parser.add_argument('-hue', '--hue', help='type of the legend variable: either "mac" or "traffic"', nargs="?", const='MAC', default='MAC')
parser.add_argument('-v', '--verbose', help='detailed output', action='store_true')
parser.add_argument('-s', '--save', help='save the plot under auto-generated name or provide one (with extension)', nargs="?", const="auto")
parser.add_argument('-e', '--expec', help='add theoretical expectations of end-to-end delay where applicable', action='store_true')
parser.add_argument('-bl', '--blacklisted', help='add theoretical performance of blacklisted TSCH', action='store_true')
parser.add_argument('-b', '--both', help='combine up-/ and downlink in a single bar chart', action='store_true')
parser.add_argument('-ng', '--no-legend', help='remove legend completely', action='store_true')
parser.add_argument('-cd', '--cooldown', help='timestamp threshold to discard values at, sort of "cooldown" phase' \
    + 'IMPORTANT: choose wisely as, e.g., selecting the timestamp of a packet transmission will almost surely leave that packet in the queue', nargs="?", type=float) 

args = parser.parse_args()
fig = plt.figure(figsize=(13, 9), dpi=600 if args.save else 100)
ax = fig.gca()

# Mapping between active user ratio (AUR) and WLAN channel utilization
aur_to_util = {
    '0': 0,
    '0.03': 33,
    '0.06': 66,
    '0.09': 100
}

def get_chart_title(res_filename):
    res_filename = res_filename.lower()
    sim_case = "Case 1" if 'case_1' in res_filename else "Case 2"
    traffic = ', uplink' if 'uplink' in res_filename else ', downlink' if 'downlink' in res_filename else ''

    return f'{sim_case}{traffic}'

def gen_filename_to_save(cmdargs):
    file_in = cmdargs.result_file
    kpi = ''
    traffic = ''
    case = re.search("case_(\d+)", file_in).group(1)

    if cmdargs.pdr:
        kpi = 'pdr'
    if cmdargs.delay:
        kpi = 'delay'

    if 'uplink' in file_in:
        traffic = 'uplink'
    if 'downlink' in file_in:
        traffic = 'downlink'
    
    return f'{kpi}_case_{case}' + (f'_{traffic}' if traffic else '') + '.jpg'
    
def get_closest_element_index(l, val):
    """
    Return the index of the first element > val
    This hackish solution is from https://stackoverflow.com/questions/2236906/first-python-list-index-greater-than-x
    """
    return next(x[0] for x in enumerate(l) if x[1] > val)

def json_to_df(data, aur_to_util):
    df = []
    for sim_run in data:
        received = 0
        sent = 0
        avg_delays = []
        for vec in data[sim_run]["vectors"]:
            lastIndex = len(vec['value'])
        
            # optionally discard some values from the end of the simulation as a "cooldown" phase
            if args.cooldown and args.cooldown > 0 and args.cooldown < max(vec['time']):
                lastIndex = get_closest_element_index(vec['time'], args.cooldown)
                # print(f'{vec["name"]} skpping {len(vec["value"]) - lastIndex} values from the end')

            if 'endToEnd' in vec['name']:
                received += len(vec['value'][:lastIndex])
                avg_delays += vec['value'][:lastIndex]

            if 'packetSent' in vec['name']:
                sent += len(vec['value'][:lastIndex])

        if args.verbose:
            print(f"{data[sim_run]['attributes']['configname']} AUR={data[sim_run]['itervars']['activeUserRatio']} #{data[sim_run]['attributes']['repetition']}: sent = {sent}, rec = {received}")

        mac_name = 'TSCH' if 'tsch' in data[sim_run]['attributes']['configname'].lower() else 'CSMA'
        traffic = 'down' if 'downlink' in data[sim_run]['attributes']['configname'].lower() else 'up'
        if args.both:
            mac_name += f' ({traffic})'

        df.append(
            {
                'MAC': mac_name,
                'Traffic': traffic,
                'Util': aur_to_util[data[sim_run]['itervars']['activeUserRatio']],
                'PDR': float(received) / sent,
                'Delay': np.mean(avg_delays),
                'Repetition': data[sim_run]['attributes']['repetition']
            }
        )

    return pd.DataFrame(df)

def plot_pdr_vector(df, hue='MAC', hue_order=['CSMA', 'TSCH']):
    if args.blacklisted:
        hue_order.append('TSCH (blacklisted)')
        for i in range(3):
            df = df.append({
                'MAC': 'TSCH (blacklisted)',
                'Delay': tsch_service_time(1),
                'Util': 100,
                'PDR': 1
            }, ignore_index=True)

    g = sns.barplot(data=df, x='Util', y='PDR', capsize=.15, hue=hue, hue_order=hue_order, palette=palette)
    g.set_xlabel("Wi-Fi Channel Utilization (%)")
    g.grid(axis='y')
    g.legend(loc='lower right')
    if args.no_legend:
        g.get_legend().remove()

def plot_delay_vector(df, hue='MAC', hue_order=['CSMA', 'TSCH']):
    if args.expec:
        df = df.append({
            'MAC': 'TSCH (expected)',
            'Delay': tsch_service_time(1),
            'Util': 0,
        }, ignore_index=True)
        hue_order.append('TSCH (expected)')

    if args.blacklisted:
        hue_order.append('TSCH (blacklisted)')
        for i in range(3):
            df = df.append({
                'MAC': 'TSCH (blacklisted)',
                'Delay': tsch_service_time(1),
                'Util': 100,
            }, ignore_index=True)
        
    tsch_up = np.mean(df[(df['Util'] == 100) & (df['MAC'] == 'TSCH (up)')]['Delay'].tolist())
    tsch_down = np.mean(df[(df['Util'] == 100) & (df['MAC'] == 'TSCH (down)')]['Delay'].tolist())


    csma_0 = np.mean(df[(df['Util'] == 0) & (df['MAC'] == 'CSMA (up)')]['Delay'].tolist())
    csma_33 = np.mean(df[(df['Util'] == 33) & (df['MAC'] == 'CSMA (up)')]['Delay'].tolist())

    print('mean delay of TSCH (up) with 100% Wi-Fi activity: ', tsch_up)
    print('mean delay of TSCH (down) with 100% Wi-Fi activity: ', tsch_down)
    print('increase in %: ', (tsch_down - tsch_up) / tsch_up * 100 )

    print('mean delay of CSMA (up) with 0% Wi-Fi activity: ', csma_0)
    print('mean delay of CSMA (up) with 33% Wi-Fi activity: ', csma_33)
    print('increase in %: ', (csma_33 - csma_0) / csma_0 * 100 )

    g = sns.barplot(data=df, x='Util', y='Delay', capsize=.15, hue=hue, hue_order=hue_order, ax=ax)
    g.set_xlabel("Wi-Fi Channel Utilization (%)")
    g.set_ylabel("Delay (s)")
    g.set(yscale='log')
    g.set_ylim([None, 5])
    g.grid(axis='y')

    # for l in len()g.lines():
    #     xs = l.get_xdata()
    #     ys = l.get_ydata()
    #     print('x: ', xs, ', y: ', ys)

    g.legend(loc='lower right')
    if args.no_legend:
        g.get_legend().remove()

    # if args.both:
    #     g.legend(loc='lower left', bbox_to_anchor=(0.6, 0.8))

hue_order = ['CSMA (up)', 'CSMA (down)', 'TSCH (up)', 'TSCH (down)'] if args.both else ['CSMA', 'TSCH']
if args.both:
    p = sns.set_palette('Paired')

if "json" in args.result_file:
    data = json.load(open(args.result_file))
else:
    data = pd.read_csv(args.result_file, sep='\t')

if "json" in args.result_file:
    data = json_to_df(data, aur_to_util)

if args.pdr and "json" in args.result_file:
    plot_pdr_vector(data, args.hue, hue_order)

if args.delay and "json" in args.result_file:
    plot_delay_vector(data, args.hue, hue_order)


plt.title(get_chart_title(args.result_file))
plt.tight_layout()

if args.save:
    to_save = gen_filename_to_save(args) if args.save == "auto" else args.save
    print('saving figure under: ', to_save)
    plt.savefig(itnac_imgs_path + '/' + to_save)
else:
    plt.show()


