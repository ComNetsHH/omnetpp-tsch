import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import re
import math

plt.rcParams['axes.ymargin'] = 0.2
plt.rcParams.update({'errorbar.capsize': 3.5})
plt.rcParams.update({'font.size': 24})

def service_time(m):
    return (m + 2)/(2 * pow(m + 1, 2))

def waiting_time_dd1(l, lim_cell_high):
    m = math.ceil(l * 1/lim_cell_high)

    return service_time(m)

def waiting_time_md1(l, lim_cell_high):
    m = math.ceil(l * 1/lim_cell_high)

    rho = l/m

    queuing = rho/(2 * m * (1 - rho))
    serv_time = service_time(m)

    w_md1 = queuing + serv_time

    # print(f'l = {l}, op = {round(1/lim_cell_high, 2)}, m = {m}, w_md1 = {serv_time} (service) + {queuing} (queuing)')

    return w_md1

def plot_expected_waiting_time(q = 'DD1', ax = None, lim_high = -1, rates = []):
    lims = [0.25, 0.50, 0.75, 0.9]
    if len(rates) == 0:
        rates = [x for x in range(1, 30)]

    if not ax:
        fig, ax = plt.subplots(figsize=(10, 8), dpi=100)

    if lim_high > 0:
        if q == 'DD1':
            wt = [waiting_time_dd1(x, lim_high) for x in rates]
        else:
            wt = [waiting_time_md1(x, lim_high) for x in rates]
        sns.lineplot(x=rates, y=wt, ax=ax, ls='dashed', label=f'OP = {round(1/lim_high, 2)} exp.')
        return

    for lim_cell_high in lims:
        if q == 'DD1':
            wt = [waiting_time_dd1(x, lim_cell_high) for x in rates] # l = 1..30
        else:
            wt = [waiting_time_md1(x, lim_cell_high) for x in rates]

        sns.lineplot(x=rates, y=wt, ax=ax, ls='dashed', label=f'OP = {1/lim_cell_high} exp.')

def plot_packet_loss_adaptation(csv_data):
    # add a column with link collision probabilities as floats
    max_num_cells = [int(re.search('\$maxCells=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    cell_to_add = [int(re.search('\$cellToAdd=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    traffic_rate = [float(re.search('\$l=(\d+(\.\d+)?)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Max cells'] = max_num_cells
    csv_data['Cells to add'] = cell_to_add
    csv_data['Traffic rate'] = traffic_rate

    df = pd.DataFrame(columns=['Experiment', 'Max cells', 'Cells to add', 'Traffic rate', 'Packets lost', 'Repl'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
                packets_sent = repl_group[repl_group['Name'].str.contains('Sent')]['Value'].sum()
                packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Value'].sum()
             
                pdr_entry = {
                    'Experiment': e,
                    'Max cells': megroup['Max cells'].tolist().pop(),
                    'Cells to add': megroup['Cells to add'].tolist().pop(),
                    'Traffic rate': megroup['Traffic rate'].tolist().pop(),
                    'Repl': int(re.search('#(\d+)', repl).group(1)),
                    'Packets lost': packets_sent - packets_recv,
                }            

                df = df.append(pdr_entry, ignore_index=True)

    fig, ax = plt.subplots(figsize=(10, 8), dpi=100)
    graph = sns.lineplot(data=df, x='Traffic rate', y='Packets lost', ci=95, err_style='bars', hue='Max cells', legend='brief')

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)
    fig.tight_layout()

def plot_pdr_lossy(csv_data):
    # add a column with link collision probabilities as floats
    p_c = [float(re.search('\$pc=(\d+(\.\d+)?)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['p_c'] = p_c

    # add a column with retransmission thresholds as integers
    max_retries = [int(re.search('\$rtxThresh=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Max retries'] = max_retries

    pdr_df = pd.DataFrame(columns=['Experiment', 'p_c', 'Max retries', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
                packets_sent = repl_group[repl_group['Name'].str.contains('Sent')]['Value'].sum()
                packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Value'].sum()
                pdr = round(packets_recv / packets_sent, 3)
             
                pdr_entry = {
                    'Experiment': e,
                    'p_c': megroup['p_c'].tolist().pop(),
                    'Max retries': megroup['Max retries'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'PDR': pdr,
                }            

                pdr_df = pdr_df.append(pdr_entry, ignore_index=True)

    fig, ax = plt.subplots(figsize=(10, 8), dpi=100)
    graph = sns.lineplot(data=pdr_df, x='p_c', y='PDR', ci=95, err_style='bars', hue='Max retries', legend='brief')
    ax.set_ylim([0, 1])
    ax.set_xlabel('Link collision probability')
    ax.set_ylabel('PDR')

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)
    fig.tight_layout()


def plot_pdr(csv_data):
    # add a column with traffic rates as floats
    traffic_rate = [float(re.search('\$l=(\d+(\.\d+)?)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['l'] = traffic_rate

    pdr_df = pd.DataFrame(columns=['Experiment', 'Traffic Rate', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
                packets_sent = repl_group[repl_group['Name'].str.contains('Sent')]['Value'].sum()
                packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Value'].sum()
                pdr = round(packets_recv / packets_sent, 3)
             
                pdr_entry = {
                    'Experiment': e,
                    'Traffic Rate': megroup['l'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'PDR': pdr,
                }            

                pdr_df = pdr_df.append(pdr_entry, ignore_index=True)

    fig, ax = plt.subplots(figsize=(10, 8), dpi=100)
    graph = sns.lineplot(data=pdr_df, x='Traffic Rate', y='PDR', ci=95, err_style='bars', hue='Experiment', legend='brief')
    ax.set_ylim([0, 1])

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)
    fig.tight_layout()


def plot_delay(csv_data):
    # add a column with traffic rates as floats
    traffic_rate = [float(re.search('\$l=(\d+(\.\d+)?)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    lim_high = [float(re.search('\$limCellUsedHigh=(\d+(\.\d+)?)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['l'] = traffic_rate
    csv_data['lim_high'] = lim_high

    queue_type = 'DD1' if 'DD1' in csv_data['Experiment'].tolist().pop() else 'MD1'

    fig, ax = plt.subplots(figsize=(10, 8), dpi=100)

    # sns.lineplot(data=csv_data, x='l', y='Mean', ci=95, err_style='bars', hue='lim_high')

    for name, group in csv_data.groupby('lim_high'):
        l = group['l'].tolist()
        l.sort()

        lim_cell_high = group['lim_high'].tolist().pop()

        plot_expected_waiting_time(queue_type, ax, lim_cell_high, l)
        sns.lineplot(data=group, x='l', y='Mean', ax=ax, ci=95, err_style='bars', label=f'OP = {round(1/lim_cell_high, 2)} obs.')

    # remove legend subtitl
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels,loc='upper left')
    ax.set_ylabel('Delay (s)')
    ax.set_xlabel('Traffic rate (pkt/s)')
    fig.tight_layout()
    # plt.savefig(f"delay_{name}.pdf")

plot_delay(pd.read_csv("lukas_delays_dd1_upd.csv", sep="\t"))
# plot_pdr(pd.read_csv("lukas_pdr.csv", sep="\t"))
# plot_pdr_lossy(pd.read_csv("lukas_lossy_link.csv", sep="\t"))
# plot_packet_loss_adaptation(pd.read_csv("lukas_msf_adaptation.csv", sep="\t"))
# plot_expected_waiting_time()

plt.grid()
plt.show()