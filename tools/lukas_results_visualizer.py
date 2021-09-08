import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import re
import math

plt.rcParams['axes.ymargin'] = 0.2
plt.rcParams.update({'errorbar.capsize': 3.5})
plt.rcParams.update({'font.size': 24})

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
    csv_data['l'] = traffic_rate

    fig, ax = plt.subplots(figsize=(10, 8), dpi=100)
    
    graph = sns.lineplot(data=csv_data, x='l', y='Mean', ci=95, err_style='bars', hue='Experiment', legend='brief')

    # remove legend subtitle
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels,loc='upper left')
    fig.tight_layout()
    # plt.savefig(f"delay_{name}.pdf")

# plot_delay(pd.read_csv("lukas_delays.csv", sep="\t"))
# plot_pdr(pd.read_csv("lukas_pdr.csv", sep="\t"))
plot_pdr_lossy(pd.read_csv("lukas_lossy_link.csv", sep="\t"))
# plot_packet_loss_adaptation(pd.read_csv("lukas_msf_adaptation.csv", sep="\t"))

plt.grid()
plt.show()