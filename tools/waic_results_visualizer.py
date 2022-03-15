import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib
import re
import math
# matplotlib.use('Agg')

###############################################################
### Usage: execute this file from a directory containing  
### required .csv file(s) from the repository 'results' folder
###############################################################

# plt.rcParams.update({'lines.markeredgewidth': 2})
plt.rcParams.update({'errorbar.capsize': 3.5})
plt.rcParams.update({'font.size': 30}) # was 36
## For combined throughput and jitter graph 
# plt.rcParams.update({'font.size': 20})
plt.rcParams.update({'legend.fontsize': 28})
plt.rcParams.update({'legend.markerscale': 1.3})
plt.rcParams['axes.ymargin'] = 0.2

def pmf_ad_simpl(max_hops):
    pmf = {x: 0 for x in range(1, max_hops)}
    for h in pmf.keys():
        pmf[h] = math.comb(max_hops-1, h) * pow(0.5, max_hops-1)

    return pmf

def expec_delay_simpl(max_hops):
    pmf = pmf_ad_simpl(max_hops)
    return sum([key * val for [key, val] in pmf.items()])


def camel_case_split(identifier):
    matches = re.finditer('.+?(?:(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])|$)', identifier)
    return " ".join([m.group(0) for m in matches])


def plot_stat_delay_high_density(group, disp_yaxis_values = False):
    if group.empty:
        return
    sf_type = ['CLSF' if re.search('clEnabled=(\w+)', x).group(1) == 'true' else 'MSF' for x in group['Measurement'].tolist()]
    num_nodes = [int(re.search('numHosts=(\d+)', x).group(1)) for x in group['Measurement'].tolist()]

    group['Number of nodes'] = num_nodes
    group['SF'] = sf_type
    group = group.sort_values(by=['SF'])

    fig, ax = plt.subplots(figsize=(8, 6), dpi=150)
    graph = sns.pointplot(data=group, x='Number of nodes', y='Mean', dodge=True, join=False, hue='SF', capsize=.05, legend='brief')
    
    if disp_yaxis_values:
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
    
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)
    ax.set_ylabel('End-to-end delay (s)')
    ax.set_ylim([0, 30])
##    ax.set_title('End-to-end delay, high density (seatbelt) scenario', fontsize=18)
    # plt.grid()
    fig.tight_layout()
    plt.savefig(f"delay_high_density.pdf")

def plot_stat_pdr_high_density(group, disp_yaxis_values = False):
    if group.empty:
        return
    sf_type = ['CLSF' if re.search('clEnabled=(\w+)', x).group(1) == 'true' else 'MSF' for x in group['Measurement'].tolist()]
    num_nodes = [int(re.search('numHosts=(\d+)', x).group(1)) for x in group['Measurement'].tolist()]

    group['Number of nodes'] = num_nodes
    group['SF'] = sf_type
    group = group.sort_values(by=['SF'])

    pdr_df = pd.DataFrame(columns=['Number of nodes', 'SF', 'Replication', 'PDR'])

    for me, megroup in group.groupby('Measurement'):
        for repl, repl_group in megroup.groupby('Replication'):
            packets_sent = repl_group[repl_group['Name'].str.contains('Sent')]['Value'].sum()
            packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Value'].tolist().pop()
            pdr_val = round(packets_recv / packets_sent, 3)            
            pdr_entry = {
                'Number of nodes': megroup['Number of nodes'].tolist().pop(),
                'SF': megroup['SF'].tolist().pop(),
                'Replication': int(re.search('#(\d+)', repl).group(1)),
                'PDR': pdr_val
            }            

            pdr_df = pdr_df.append(pdr_entry, ignore_index=True)

    pdr_df = pdr_df.sort_values(by=['SF'])

    fig, ax = plt.subplots(figsize=(8, 6), dpi=100)
    graph = sns.pointplot(data=pdr_df, x='Number of nodes', y='PDR', dodge=True, join=False, capsize=.05,  hue='SF', legend='brief')

    if disp_yaxis_values:
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
        
    ax.set_ylim([0.1, 1])  
##    ax.set_title('PDR, high density (seatbelt) scenario', fontsize=18)
    
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    # plt.grid()
    fig.tight_layout()
    plt.savefig(f"pdr_high_density.pdf")   
    

def plot_stat_pdr(csv_data, disp_yaxis_values = False):
    if csv_data.empty:
        return
    for name, group in csv_data.groupby('Experiment'):  
        hop_count = [int(re.search('hops=(\d+)', x).group(1)) for x in group['Measurement'].tolist()]
        sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in group['Measurement'].tolist()]

        group['Hops'] = hop_count
        group['SF'] = sf_type
        
        pdr_df = pd.DataFrame(columns=['Hops', 'SF', 'Replication', 'PDR'])

        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
                packets_sent = repl_group[repl_group['Name'].str.contains('Sent')]['Count'].sum()
                packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Count'].tolist().pop()
                pdr_val = round(packets_recv / packets_sent, 3)
             
                pdr_entry = {
                    'Hops': megroup['Hops'].tolist().pop(),
                    'SF': megroup['SF'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'PDR': pdr_val
                }            

                pdr_df = pdr_df.append(pdr_entry, ignore_index=True)

        pdr_df = pdr_df.sort_values(by=['SF'])

        fig, ax = plt.subplots(figsize=(8, 6), dpi=150)
        graph = sns.lineplot(data=pdr_df, x='Hops', y='PDR', ci=95, err_style='bars', hue='SF', legend='brief')

        if disp_yaxis_values:
            print(f'{name}:\n')
            for line in ax.lines:
                print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
            
        ax.set_ylim([0.3, 1.05])
##        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
        
        # remove legend title
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles=handles, labels=labels)

        fig.tight_layout()
        # plt.grid()
        plt.savefig(f"pdr_{name}.pdf")


def plot_stat_delay(csv_data, disp_yaxis_values=False, disp_title=False, plot_expectation=True, save_jpg=False):
    if csv_data.empty:
        return

    hop_count = [int(re.search('hops=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]
    
    csv_data['Hops'] = hop_count
    csv_data['SF'] = sf_type
    csv_data['End-to-end delay (s)'] = csv_data['Mean'].apply(lambda x: float(x))

    marker = ['o', 'x']
    markers = [marker[i] for i in range(len(csv_data["SF"].unique()))]
    
    for name, group in csv_data.groupby('Experiment'):
        title = f'{camel_case_split(name)} (' + {'SmokeAlarm': '1s to 2s transmission interval)', 'HumidityMonitoring': '10s to 20s trans. interval)'}[name]
        fig, ax = plt.subplots(figsize=(10, 9), dpi=600 if save_jpg else 150)
        group = group.sort_values(by=['SF'])

        graph = sns.lineplot(data=group, x='Hops', y='End-to-end delay (s)', ci=95, err_style='bars',
            hue='SF', legend='brief', hue_order=['6TiSCH-MSF', '6TiSCH-CLX'], linewidth=3, markers=markers)

        # plot analytical expectation
        if plot_expectation:

            if 'Smoke' not in name:
                max_hops = group.sort_values(by=['Hops'])['Hops'].tolist()[-1]
                pmf = pmf_ad_simpl(max_hops + 2)
                
                eds = [expec_delay_simpl(x) for x in range(2, len(pmf.keys()))]

                for i in range(len(eds)):
                    eds[i] += 0.5 # to account for average service time on each hop

                plt.scatter([x for x in range(2, len(pmf.keys()))], eds, color='violet', s=96, linewidth=3)
                plt.plot([x for x in range(2, len(pmf.keys()))], eds, label='E[delay] MSF (analytical)', color='violet', ls='dashed', linewidth=3)
            else:
                graph.axhline(y=1, color='g', linestyle='--', label='App. requirement', linewidth=3)
        
        if disp_yaxis_values:
            print(f'{name}:\n')
            for line in ax.lines:
                print([round(x, 2) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
        
        ax.set_ylim([0, 6])
##        ax.set_title(f'{"End-to-end delay"}, {camel_case_split(name)}', fontsize=18)
        
        # remove legend title
        handles, labels = ax.get_legend_handles_labels()
        leg = ax.legend(handles=handles, labels=labels,loc='upper left')

        for line in leg.get_lines():
            line.set_linewidth(3)

        ax.grid()
        fig.tight_layout()

        plt.savefig(f"delay_{name}." + ("jpg" if save_jpg else "eps"))

    
##########################
## Multi-gateway scenarios
##########################

def plot_average_rank(csv_data, disp_yaxis_values=False, boxplot=False, ax=None, save_jpg=False):
    if csv_data.empty:
        return

    is_subplot = ax is not None

    csv_data = csv_data.loc[csv_data['Value'] < 100]

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    
    df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'Avg. Rank'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
               
                entry = {
                    'Experiment': e,
                    'Gateways': megroup['Gateways'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'Avg. Rank': repl_group['Value'].mean(),
                    'SF': megroup['SF'].tolist().pop()
                }            

                df = df.append(entry, ignore_index=True)

    df = df.sort_values(by=['Gateways'])

    if not ax:
        fig, ax = plt.subplots(figsize=(10, 9), dpi=600 if save_jpg else 100)

    if boxplot:
        graph = sns.boxplot(ax=ax, data=df, x='Gateways', y='Avg. Rank', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(ax=ax, data=df, x='Gateways', y='Avg. Rank', dodge=True,
                              join=False, capsize=.05,  hue='SF', legend='brief')
        
    ax.set(ylabel='Average rank')
    ax.set(xlabel=('Sinks'))

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')


##        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
##    ax.set_ylim([0, 13])
    ax.set_ylim(bottom=1)
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)
    ax.grid()

    if not is_subplot:
        fig.tight_layout()
        # plt.grid()
        plt.savefig(f"waic_avg_rank_200." + ("jpg" if save_jpg else "pdf"))

def plot_convergence_time(csv_data, disp_yaxis_values=False, boxplot=False):
    if csv_data.empty:
        return

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    
    df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'Convergence Time'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):

                start_time = repl_group[repl_group['Name'].str.contains('Started')]['Mean'].min()
                end_time = repl_group[repl_group['Name'].str.contains('Ended')]['Mean'].max()

                print(f'convergence time replication #{repl}: start = {start_time}, end = {end_time}')

                entry = {
                    'Experiment': e,
                    'Gateways': megroup['Gateways'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'Convergence Time': end_time - start_time,
                    'SF': megroup['SF'].tolist().pop()
                }            

                df = df.append(entry, ignore_index=True)

    df = df.sort_values(by=['Gateways'])

    fig, ax = plt.subplots(figsize=(10, 9), dpi=100)

    if boxplot:
        graph = sns.boxplot(data=df, x='Gateways', y='Convergence Time', hue='SF', width=0.3, palette=['#FFFFF'])
    else:
        graph = sns.pointplot(data=df, x='Gateways', y='Convergence Time', dodge=True,
                              join=False, capsize=.05,  hue='SF', legend='brief', palette=['#fc7d2d'])
        
    ax.set(ylabel="Convergence Time (s)")
    ax.set(xlabel="Sinks")

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')


##        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
##    ax.set_ylim([200, 400])
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    fig.tight_layout()
    # plt.grid()
    plt.savefig(f"waic_convergence_times_200.pdf")

    
def plot_delay_multigw(csv_data, disp_yaxis_values = False, boxplot = False, ax=None, save_jpg=False):
    if csv_data.empty:
        return

    is_subplot = ax is not None

    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    csv_data['Gateways'] = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['End-to-end delay (s)'] = csv_data['Mean'].apply(lambda x: float(x))
    
    #title = f'{camel_case_split(name)} (' + {'SmokeAlarm': '1s to 2s transmission interval)', 'HumidityMonitoring': '10s to 20s trans. interval)'}[name]
    if not ax:
        fig, ax = plt.subplots(figsize=(10, 9), dpi=600 if save_jpg else 150)
    csv_data = csv_data.sort_values(by=['SF'])
##    graph = sns.pointplot(data=csv_data, x='Number of nodes', y='End-to-end delay (s)', dodge=True, join=False, capsize=.05,  hue='Experiment', legend='brief')

    if boxplot:
        graph = sns.boxplot(ax=ax, data=csv_data, x='Gateways', y='End-to-end delay (s)', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(ax=ax, data=csv_data, x='Gateways', y='End-to-end delay (s)',
                              dodge=True, join=False, capsize=.05,  hue='SF', hue_order=['6TiSCH-MSF', '6TiSCH-CLX'])  
    
    graph.axhline(y=1, color='g', linestyle='--', label='App. requirement', linewidth=3)

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 2) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
    
    ax.set_ylim([0, 6])
    ax.set(xlabel="Sinks")
##        ax.set_title(f'End-to-end delay, {camel_case_split(name)}', fontsize=18)

    # ax.set_ylim(bottom=0)
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    leg = ax.legend(handles=handles, labels=labels,loc='upper right')

    # lines = leg.get_lines()
    # lines[1].set_linewidth(3)

    ax.grid()

    if not is_subplot:
        fig.tight_layout()
        plt.savefig(f"waic_delay_200." + ("jpg" if save_jpg else "eps"))

def plot_overhead(csv_data, disp_yaxis_values=False, boxplot=False, ax=None, save_jpg=False):
    is_subplot = ax is not None
    
    if csv_data.empty:
            return

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    
    df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
            
                app_packets = repl_group[repl_group['Name'].str.contains('Received')]['Value'].sum()
                ctrl_packets = repl_group[repl_group['Name'].str.contains('control')]['Value'].sum()

##                print(f'total app packets: {app_packets}, control: {ctrl_packets}')                
                
                entry = {
                    'Experiment': e,
                    'Gateways': megroup['Gateways'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'Overhead': ctrl_packets / app_packets,
                    'SF': megroup['SF'].tolist().pop()
                }            

                df = df.append(entry, ignore_index=True)

    df = df.sort_values(by=['Gateways'])

    if not ax:
        fig, ax = plt.subplots(figsize=(10, 9), dpi=600 if save_jpg else 100)

    if boxplot:
        graph = sns.boxplot(ax=ax, data=df, x='Gateways', y='Overhead', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(ax=ax, data=df, x='Gateways', y='Overhead', dodge=True,
                              join=False, capsize=.05,  hue='SF', legend='brief')
        
    ax.set(ylabel="Control to app. traffic ratio")
    ax.set(xlabel="Sinks")

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')


##        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
##    ax.set_ylim([200, 400])
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    ax.grid()

    if not is_subplot:
        fig.tight_layout()
        # plt.grid()
        plt.savefig(f"waic_overhead_200." + ("jpg" if save_jpg else "pdf"))

def plot_throughput(csv_data, disp_yaxis_values=False, boxplot=False, ax=None, save_jpg=False):
    is_subplot = ax is not None
    
    if csv_data.empty:
        return

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    
    throughput_df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
                total_throughput = repl_group['Mean'].sum()
                
                t_entry = {
                    'Experiment': e,
                    'Gateways': megroup['Gateways'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'Throughput': total_throughput / 8,
                    'SF': megroup['SF'].tolist().pop()
                }            

                throughput_df = throughput_df.append(t_entry, ignore_index=True)

    throughput_df = throughput_df.sort_values(by=['Gateways'])

    if ax is None:
        fig, ax = plt.subplots(figsize=(10, 9), dpi=600 if save_jpg else 100)

    if boxplot:
        graph = sns.boxplot(ax=ax, data=throughput_df, x='Gateways', y='Throughput', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(ax=ax, data=throughput_df, x='Gateways', y='Throughput', dodge=True,
                              join=False, capsize=.05,  hue='SF', hue_order=['6TiSCH-MSF', '6TiSCH-CLX'])
        
    ax.set(ylabel='Throughput (B/s)')
    ax.set(xlabel='Sinks')

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')


##        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
##    ax.set_ylim([200, 400])
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)
    ax.grid()

    if not is_subplot:
        fig.tight_layout()
        # plt.grid()
        plt.savefig(f"waic_throughput_200." + ("jpg" if save_jpg else "pdf"))

    
def plot_pdr_multigw(csv_data, disp_yaxis_values=False, boxplot=False, ax=None, save_jpg=False):
    if csv_data.empty:
        return

    is_subplot = ax is not None

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    
    pdr_df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
                packets_sent = repl_group[repl_group['Name'].str.contains('Sent')]['Count'].sum()
                packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Count'].sum()
                pdr = round(packets_recv / packets_sent, 3)
             
                pdr_entry = {
                    'Experiment': e,
                    'Gateways': megroup['Gateways'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'PDR': pdr,
                    'SF': megroup['SF'].tolist().pop()
                }            

                pdr_df = pdr_df.append(pdr_entry, ignore_index=True)

    pdr_df = pdr_df.sort_values(by=['SF'])

    if not ax:
        fig, ax = plt.subplots(figsize=(10, 9), dpi=600 if save_jpg else 100)

    if boxplot:
        graph = sns.boxplot(ax=ax, data=pdr_df, x='Gateways', y='PDR', hue='SF', hue_order=['6TiSCH-MSF', '6TiSCH-CLX'], width=0.3)
    else:
        graph = sns.pointplot(ax=ax, data=pdr_df, x='Gateways', y='PDR', dodge=True, join=False,
                              capsize=.05,  hue='SF', hue_order=['6TiSCH-MSF', '6TiSCH-CLX'], legend='brief')

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
        
    ax.set_ylim([0, 1])
    ax.set(xlabel="Sinks")
##        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
    
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)
    ax.grid()

    if not is_subplot:
        fig.tight_layout()
        plt.savefig(f"waic_pdr_200." + ("jpg" if save_jpg else "pdf"))

        
##########################
## WAIC vs ISM
##########################

def plot_stat_pdr_waic(csv_data, disp_yaxis_values = False):
    if csv_data.empty:
        return

    num_hosts = [int(re.search('numHosts=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Number of nodes'] = num_hosts

    print(csv_data)
    
    pdr_df = pd.DataFrame(columns=['Experiment', 'Number of nodes', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            for repl, repl_group in megroup.groupby('Replication'):
                packets_sent = repl_group[repl_group['Name'].str.contains('Sent')]['Count'].sum()
                packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Count'].tolist().pop()
                pdr = round(packets_recv / packets_sent, 3)

                pdr_entry = {
                    'Experiment': e,
                    'Number of nodes': megroup['Number of nodes'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'PDR': pdr
                }            

                pdr_df = pdr_df.append(pdr_entry, ignore_index=True)

    pdr_df = pdr_df.sort_values(by=['Number of nodes'])

    fig, ax = plt.subplots(figsize=(8, 6), dpi=150)
##    graph = sns.pointplot(data=pdr_df, x='Number of nodes', y='PDR', dodge=True, join=False, capsize=.05,  hue='Experiment', legend='brief')
    graph = sns.boxplot(data=pdr_df, x='Number of nodes', y='PDR', hue='Experiment', width=0.3)

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
        
    ax.set_ylim([0, 1])
##        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
    
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    fig.tight_layout()
    # plt.grid()
    plt.savefig(f"waic_pdr.pdf")


def plot_stat_delay_waic(csv_data, disp_yaxis_values=False, disp_title=False):
    if csv_data.empty:
        return
    
    num_hosts = [int(re.search('numHosts=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Number of nodes'] = num_hosts
    
    csv_data['End-to-end delay (s)'] = csv_data['Mean'].apply(lambda x: float(x))

    #title = f'{camel_case_split(name)} (' + {'SmokeAlarm': '1s to 2s transmission interval)', 'HumidityMonitoring': '10s to 20s trans. interval)'}[name]
    fig, ax = plt.subplots(figsize=(12, 8), dpi=100)
    csv_data = csv_data.sort_values(by=['Number of nodes'])
##    graph = sns.pointplot(data=csv_data, x='Number of nodes', y='End-to-end delay (s)', dodge=True, join=False, capsize=.05,  hue='Experiment', legend='brief')

    graph = sns.boxplot(data=csv_data, x='Number of nodes', y='End-to-end delay (s)', hue='Experiment', width=0.3)
    
    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 2) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
    
##        ax.set_ylim([0, 5])
##        ax.set_title(f'End-to-end delay, {camel_case_split(name)}', fontsize=18)

    ax.set_ylim([0, 5])
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels,loc='upper left')
    fig.tight_layout()
    plt.grid()
##    plt.savefig(f"waic_delay_200.pdf")
        
    
def plot_jitter(csv_data, disp_yaxis_values = False, boxplot = False, filter_outliers = True, ax=None, save_jpg=False):
    if csv_data.empty:
        return

    is_subplot = ax is not None
    # outliers = csv_data.loc[csv_data['Mean'] >= 15]
    # print(f'jitter outliers - ', outliers)

    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    csv_data['Gateways'] = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Jitter'] = csv_data['Mean'].apply(lambda x: float(x))

##    if filter_outliers:
##        csv_data = csv_data.loc[csv_data['Jitter'] < 20]

    #title = f'{camel_case_split(name)} (' + {'SmokeAlarm': '1s to 2s transmission interval)', 'HumidityMonitoring': '10s to 20s trans. interval)'}[name]
    if not ax:
        fig, ax = plt.subplots(figsize=(10, 9), dpi=600 if save_jpg else 100)
##    csv_data = csv_data.sort_values(by=['Gateways'])
##    graph = sns.pointplot(data=csv_data, x='Number of nodes', y='End-to-end delay (s)', dodge=True, join=False, capsize=.05,  hue='Experiment', legend='brief')

    if boxplot:
        graph = sns.boxplot(ax=ax, data=csv_data, x='Gateways', y='Jitter', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(ax=ax, data=csv_data, x='Gateways', y='Jitter',
                              dodge=True, join=False, capsize=.05,  hue='SF', hue_order=['6TiSCH-CLX', '6TiSCH-MSF'])  
    
    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 2) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
    
##        ax.set_ylim([0, 5])
##        ax.set_title(f'End-to-end delay, {camel_case_split(name)}', fontsize=18)

    ax.set_ylim(bottom=0)

    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels,loc='upper right')

    ax.set(ylabel='Jitter (s)')
    ax.set(xlabel='Sinks')
    ax.grid()
    
    if not is_subplot:
        fig.tight_layout()
        # plt.grid()
        plt.savefig(f"waic_jitter_200." + ("jpg" if save_jpg else "pdf"))


def read_csvs():
    plot_data = {
            'delays': pd.DataFrame(),
            'pdr': pd.DataFrame(),
            'high_density_delays': pd.DataFrame(),
            'high_density_pdr': pd.DataFrame()
        }
    try:
        plot_data['delays'] = pd.read_csv("simulation_results_delays.csv", sep="\t")
    except:
        print("Couldn't read small-scale scenario delays data")
        
    try:
        plot_data['high_density_delays'] = pd.read_csv("high_density_delays.csv", sep="\t")
    except:
        print("Couldn't read high density scenario delays data")
        
    try:
        plot_data['pdr'] = pd.read_csv("simulation_results_pdr.csv", sep="\t")
    except:
        print("Couldn't read small-scale scenario PDR data")

    try:
        plot_data['high_density_pdr'] = pd.read_csv("pdr_high_density.csv", sep="\t")
    except:
        print("Couldn't read high density PDR data")

   
    plot_data['waic_ism_pdr'] = pd.read_csv("waic_pdr_beautified.csv", sep="\t")
    plot_data['waic_ism_delay'] = pd.read_csv("waic_delays_beautified.csv", sep="\t")

    return plot_data    

# data = read_csvs()    

def plot_throughput_and_jitter(throughput_data, jitter_data):
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    
    plot_throughput(throughput_data, ax=axes[0])
    plot_jitter(jitter_data, ax=axes[1])

    plt.tight_layout()
    plt.subplots_adjust(wspace=0.4)
    plt.savefig("waic_200_jitter_throughput.pdf")

def plot_avg_rank_ctrl_overhead(avg_rank_data, ctrl_overhead_data):
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    
    plot_average_rank(avg_rank_data, ax=axes[0])
    plot_overhead(ctrl_overhead_data, ax=axes[1])

    plt.tight_layout()
    plt.subplots_adjust(wspace=0.4)
    plt.savefig("waic_200_avg_rank_ctrl_overhead.pdf")

def plot_delay_pdr_multigw(delay_data, pdr_data):
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    
    plot_delay_multigw(delay_data, ax=axes[0])
    plot_pdr_multigw(pdr_data, ax=axes[1])

    plt.tight_layout()
    plt.subplots_adjust(wspace=0.4)
    plt.savefig("waic_200_delay_pdr.pdf")

##plot_stat_pdr_waic(data['waic_ism_pdr'])
##plot_stat_delay_waic(data['waic_ism_delay'])

#plot_convergence_time(pd.read_csv("waic_200_convergence_time_final_5.csv", sep="\t"), False)

save_as_jpg = False

# plot_overhead(pd.read_csv("waic_200_overhead.csv", sep="\t"), save_jpg=save_as_jpg)
plot_delay_multigw(pd.read_csv("waic_200_delays.csv", sep="\t"), save_jpg=save_as_jpg)
# plot_pdr_multigw(pd.read_csv("waic_200_pdr.csv", sep="\t"), save_jpg=save_as_jpg)
# plot_average_rank(pd.read_csv("waic_200_avg_rank.csv", sep="\t"), save_jpg=save_as_jpg)
# plot_jitter(pd.read_csv("waic_200_jitter.csv", sep="\t"), save_jpg=save_as_jpg)
# plot_throughput(pd.read_csv("waic_200_throughput.csv", sep="\t"), save_jpg=save_as_jpg)
# plot_stat_pdr(data['pdr'])
# plot_stat_delay(pd.read_csv("delays.csv", sep="\t"), save_jpg=save_as_jpg)
##plot_stat_delay_high_density(data['high_density_delays'])
##plot_stat_pdr_high_density(data['high_density_pdr'])
# plot_throughput_and_jitter(pd.read_csv("waic_200_throughput.csv", sep="\t"), pd.read_csv("waic_200_jitter.csv", sep="\t"))
# plot_avg_rank_ctrl_overhead(pd.read_csv("waic_200_avg_rank.csv", sep="\t"), pd.read_csv("waic_200_overhead.csv", sep="\t")) 
# plot_delay_pdr_multigw(pd.read_csv("waic_200_delays.csv", sep="\t"), pd.read_csv("waic_200_pdr.csv", sep="\t"))

plt.show()        

    


