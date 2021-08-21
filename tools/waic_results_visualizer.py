import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import re
import math
import sys
import argparse

###############################################################
### Usage: execute this file from a directory containing  
### required .csv file(s) from the repository 'results' folder
###############################################################

plt.rcParams.update({'lines.markeredgewidth': 2})
plt.rcParams.update({'errorbar.capsize': 3.5})
##plt.rcParams.update({'font.size': 32})
##plt.rcParams.update({'legend.fontsize': 24})
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
    plt.grid()
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
    ax.set_title('PDR, high density (seatbelt) scenario', fontsize=18)
    
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    plt.grid()
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
                packets_recv = repl_group[repl_group['Name'].str.contains('Received')]['Count'].sum()

                num_hops = megroup['Hops'].tolist().pop()
                sf_type = megroup['SF'].tolist().pop()
                
                pdr_val = round(packets_recv / packets_sent, 3)

                print(f'#{repl}: hops = {num_hops}, sf = {sf_type}, sent = {packets_sent}, \
                      recv = {packets_recv}, pdr = {pdr_val}')
             
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
        ax.set_title(f'PDR, {camel_case_split(name)}', fontsize=18)
        
        # remove legend title
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles=handles, labels=labels)

        fig.tight_layout()
        plt.grid()
        plt.savefig(f"pdr_{name}.pdf")


def plot_stat_delay(csv_data, disp_yaxis_values=False, disp_title=False, plot_expectation=True):
    if csv_data.empty:
        return
    for name, group in csv_data.groupby('Experiment'):
        hop_count = [int(re.search('hops=(\d+)', x).group(1)) for x in group['Measurement'].tolist()]
        sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in group['Measurement'].tolist()]

        group['Hops'] = hop_count
        group['SF'] = sf_type
        
        group['End-to-end delay (s)'] = group['Mean'].apply(lambda x: float(x))

        title = f'{camel_case_split(name)} (' + {'SmokeAlarm': '1s to 2s transmission interval)', 'HumidityMonitoring': '10s to 20s trans. interval)'}[name]
        fig, ax = plt.subplots(figsize=(12, 8), dpi=100)
        group = group.sort_values(by=['SF'])
        graph = sns.lineplot(data=group, x='Hops', y='End-to-end delay (s)', ci=95, err_style='bars', hue='SF', legend='brief')

        # plot analytical expectation
        if plot_expectation:
            max_hops = group.sort_values(by=['Hops'])['Hops'].tolist()[-1]
            pmf = pmf_ad_simpl(max_hops + 2)
            
            eds = [expec_delay_simpl(x) for x in range(2, len(pmf.keys()))]

            for i in range(len(eds)):
                eds[i] += 0.5 # service rate of a node with a single TX cell, applicaple per each hop!
            
            plt.scatter([x for x in range(2, len(pmf.keys()))], eds, color='violet', s=96)
            plt.plot([x for x in range(2, len(pmf.keys()))], eds, label='E[delay] MSF (analytical)', color='violet')
            
            graph.axhline(y=1, color='g', linestyle='--', label='CLX upper bound (best case)')
        
        if disp_yaxis_values:
            print(f'{name}:\n')
            for line in ax.lines:
                print([round(x, 2) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')
        
        ax.set_title(f'{"End-to-end delay"}, {camel_case_split(name)}', fontsize=18)
        
        # remove legend title
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles=handles, labels=labels,loc='upper left')
        fig.tight_layout()
        plt.grid()
        plt.savefig(f"delay_{name}.pdf")

    
##########################
## Multi-gateway scenarios
##########################

def plot_average_rank(csv_data, disp_yaxis_values=False, boxplot=False):
    if csv_data.empty:
        return

    csv_data = csv_data.loc[csv_data['Value'] < 100]

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    
    df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'Avg. Rank'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
##            print(f'for {me} calculating')
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

    fig, ax = plt.subplots(figsize=(8, 8), dpi=100)

    if boxplot:
        graph = sns.boxplot(data=df, x='Gateways', y='Avg. Rank', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(data=df, x='Gateways', y='Avg. Rank', dodge=True,
                              join=False, capsize=.05,  hue='SF', legend='brief')
        
    ax.set(ylabel='Average Rank')

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')

    ax.set_ylim(bottom=1)
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    fig.tight_layout()
    plt.grid()
    plt.savefig(f"waic_avg_rank_200.pdf")

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
##            print(f'for {me} calculating')
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
        
    ax.set(ylabel='Convergence Time (s)')

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')

    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    fig.tight_layout()
    plt.grid()
    plt.savefig(f"waic_convergence_times_200.pdf")

    
def plot_delay_multigw(csv_data, disp_yaxis_values = False, boxplot = False, title = ""):
    if csv_data.empty:
        return
    
    sf_type = ['6TiSCH-CLX' if re.search('\$cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]   
    
    csv_data['SF'] = sf_type
    csv_data['Gateways'] = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['End-to-end delay (s)'] = csv_data['Mean'].apply(lambda x: float(x))
 
    #title = f'{camel_case_split(name)} (' + {'SmokeAlarm': '1s to 2s transmission interval)', 'HumidityMonitoring': '10s to 20s trans. interval)'}[name]
    fig, ax = plt.subplots(figsize=(10, 9), dpi=100)
    csv_data = csv_data.sort_values(by=['SF'])
##    graph = sns.pointplot(data=csv_data, x='Number of nodes', y='End-to-end delay (s)', dodge=True, join=False, capsize=.05,  hue='Experiment', legend='brief')

    if boxplot:
        graph = sns.boxplot(data=csv_data, x='Gateways', y='End-to-end delay (s)', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(data=csv_data, x='Gateways', y='End-to-end delay (s)',
                              dodge=True, join=False, capsize=.05,  hue='SF', hue_order=["6TiSCH-MSF", "6TiSCH-CLX"], legend='brief')  
    
    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 2) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')

    ax.set_ylim(bottom=0)
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels,loc='upper right')
    fig.tight_layout()
    plt.grid()

    fig_title = title if title else "waic_delay_200" 
    
    plt.savefig(f"{fig_title}.pdf")

def plot_overhead(csv_data, disp_yaxis_values=False, boxplot=False):
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
                
                entry = {
                    'Experiment': e,
                    'Gateways': megroup['Gateways'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'Overhead': ctrl_packets / app_packets,
                    'SF': megroup['SF'].tolist().pop()
                }            

                df = df.append(entry, ignore_index=True)

    df = df.sort_values(by=['Gateways'])

    fig, ax = plt.subplots(figsize=(10, 9), dpi=100)

    if boxplot:
        graph = sns.boxplot(data=df, x='Gateways', y='Overhead', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(data=df, x='Gateways', y='Overhead', dodge=True,
                              join=False, capsize=.05,  hue='SF', legend='brief')
        
    ax.set(ylabel='Control to application traffic ratio')

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')

    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    fig.tight_layout()
    plt.grid()
    plt.savefig(f"waic_overhead_200.pdf")

def plot_throughput(csv_data, disp_yaxis_values=False, boxplot=False):
    if csv_data.empty:
        return

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
##    sf_type = ['6TiSCH-CLX' if re.search('\$0=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]
    sf_type = ['6TiSCH-CLX' if re.search('\$cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()] 
    csv_data['SF'] = sf_type
    
    throughput_df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
            print(f'for {me} calculating')
            for repl, repl_group in megroup.groupby('Replication'):
                total_throughput = repl_group['Mean'].sum()

                print(f'total throughput: {total_throughput/8}')                
                
                t_entry = {
                    'Experiment': e,
                    'Gateways': megroup['Gateways'].tolist().pop(),
                    'Replication': int(re.search('#(\d+)', repl).group(1)),
                    'Throughput': total_throughput / 8,
                    'SF': megroup['SF'].tolist().pop()
                }            

                throughput_df = throughput_df.append(t_entry, ignore_index=True)

    throughput_df = throughput_df.sort_values(by=['Gateways'])

    fig, ax = plt.subplots(figsize=(10, 9), dpi=100)

    if boxplot:
        graph = sns.boxplot(data=throughput_df, x='Gateways', y='Throughput', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(data=throughput_df, x='Gateways', y='Throughput', dodge=True,
                              join=False, capsize=.05,  hue='SF', hue_order=['6TiSCH-CLX', '6TiSCH-MSF'], legend='brief')
        
    ax.set(ylabel='Throughput (B/s)')

    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 3) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')

    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels)

    fig.tight_layout()
    plt.grid()
    plt.savefig(f"waic_throughput_200.pdf")

    
def plot_pdr_multigw(csv_data, disp_yaxis_values=False, boxplot=False):
    if csv_data.empty:
        return

    num_gws = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Gateways'] = num_gws
    sf_type = ['6TiSCH-CLX' if re.search('\$0=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]    
    csv_data['SF'] = sf_type
    
    pdr_df = pd.DataFrame(columns=['Experiment', 'Gateways', 'Replication', 'PDR'])

    for e, group in csv_data.groupby('Experiment'):
        for me, megroup in group.groupby('Measurement'):
##            print(f'for {me} calculating')
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

    fig, ax = plt.subplots(figsize=(10, 9), dpi=100)

    if boxplot:
        graph = sns.boxplot(data=pdr_df, x='Gateways', y='PDR', hue='SF', width=0.3)
    else:
        graph = sns.pointplot(data=pdr_df, x='Gateways', y='PDR', dodge=True, join=False,
                              capsize=.05,  hue='SF', legend='brief')

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
    plt.grid()
    plt.savefig(f"waic_pdr_200.pdf")

        
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
            print(f'for {me} calculating')
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
    plt.grid()
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

    ax.set_ylim([0, 5])
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels,loc='upper left')
    fig.tight_layout()
    plt.grid()
    plt.savefig(f"waic_delay_200.pdf")
        
    
def plot_jitter(csv_data, disp_yaxis_values = False, boxplot = False):
    if csv_data.empty:
        return

    sf_type = ['6TiSCH-CLX' if re.search('cl=(\w+)', x).group(1) == 'true' else '6TiSCH-MSF' for x in csv_data['Measurement'].tolist()]  
    csv_data['SF'] = sf_type
    csv_data['Gateways'] = [int(re.search('sinks=(\d+)', x).group(1)) for x in csv_data['Measurement'].tolist()]
    csv_data['Jitter'] = csv_data['Mean'].apply(lambda x: float(x))

    #title = f'{camel_case_split(name)} (' + {'SmokeAlarm': '1s to 2s transmission interval)', 'HumidityMonitoring': '10s to 20s trans. interval)'}[name]
    fig, ax = plt.subplots(figsize=(10, 9), dpi=100)
##    csv_data = csv_data.sort_values(by=['Gateways'])
##    graph = sns.pointplot(data=csv_data, x='Number of nodes', y='End-to-end delay (s)', dodge=True, join=False, capsize=.05,  hue='Experiment', legend='brief')

    if boxplot:
        graph = sns.boxplot(data=csv_data, x='Gateways', y='Jitter', hue='SF', hue_order=['6TiSCH-CLX', '6TiSCH-MSF'], width=0.3)
    else:
        graph = sns.pointplot(data=csv_data, x='Gateways', y='Jitter',
                              dodge=True, join=False, capsize=.05,  hue='SF', hue_order=['6TiSCH-MSF', '6TiSCH-CLX'], legend='brief')  
    
    if disp_yaxis_values:
        print(f'{name}:\n')
        for line in ax.lines:
            print([round(x, 2) for x in line.get_ydata()], end='\n' + '-'*40 + '\n')

    ax.set_ylim(bottom=0)
    # remove legend title
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles, labels=labels,loc='upper right')

    ax.set(ylabel='Jitter (s)')
    
    fig.tight_layout()
    plt.grid()
    plt.savefig(f"waic_jitter_200.pdf")


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

parser = argparse.ArgumentParser(description='Plotting results for smoke alarm/humidity and seatbelt WAIC scenarios')
parser.add_argument('result_file', metavar='f', type=str, help='path to the CSV file with exported simulation results')
parser.add_argument('-dsmoke', '--delay-smoke', help='plot end-to-end delay of smoke/humidity sensors', action='store_true')
parser.add_argument('-psmoke', '--pdr-smoke', help='plot PDR of of smoke/humidity sensors', action='store_true')
parser.add_argument('-dseat', '--delay-seatbelt', help='plot end-to-end delay of seatbelt sensors', action='store_true')
parser.add_argument('-pseat', '--pdr-seatbelt', help='plot PDR of seatbelt sensors', action='store_true')
parser.add_argument('-t', '--throughput', help='plot throughput of seatbelt sensors', action='store_true')
parser.add_argument('-j', '--jitter', help='plot jitter of seatbelt sensors', action='store_true')
parser.add_argument('-r', '--rank', help='plot average rank of seatbelt sensors', action='store_true')
parser.add_argument('-o', '--overhead', help='plot control traffic overhead of seatbelt sensors', action='store_true')
parser.add_argument('-ct', '--convergence-time', help='plot daisy-chain convergence time of seatbelt sensors', action='store_true')

args = parser.parse_args()

data = pd.read_csv(args.result_file, sep="\t")

if args.delay_smoke:
    plot_stat_delay(data)
elif args.pdr_smoke:
    plot_stat_pdr(data)
elif args.delay_seatbelt:
    plot_delay_multigw(data)
elif args.pdr_seatbelt:
    plot_pdr_multigw(data)
elif args.throughput:
    plot_throughput(data)
elif args.jitter:
    plot_jitter(data)
elif args.rank:
    plot_average_rank(data)
elif args.overhead:
    plot_overhead(data)
elif args.convergence_time:
    plot_convergence_time(data)
    

##plot_stat_pdr_waic(data['waic_ism_pdr'])
##plot_stat_delay_waic(data['waic_ism_delay'])

##plot_convergence_time(pd.read_csv("waic_200_convergence_time_final_5.csv", sep="\t"), False)
##plot_overhead(pd.read_csv("waic_200_overhead_final_5.csv", sep="\t"))
##plot_delay_multigw(pd.read_csv("waic_200_delays_very_old.csv", sep="\t"), False)
##plot_delay_multigw(pd.read_csv("delays_200_no_warmup.csv", sep="\t"), False, False, "delays_200_no_warmup")
##plot_pdr_multigw(pd.read_csv("waic_200_pdr_upd_4.csv", sep="\t"))
##plot_average_rank(pd.read_csv("waic_200_avg_rank_final_5.csv", sep="\t"))
##plot_jitter(pd.read_csv("waic_200_jitter_very_old.csv", sep="\t"))
##plot_throughput(pd.read_csv("waic_200_throughput_old.csv", sep="\t"))
##
##plot_stat_pdr(data['pdr'])
##plot_stat_delay(data['delays'])
##plot_stat_delay(data)
##plot_stat_pdr(data)

##plot_stat_delay_high_density(data['high_density_delays'])
##plot_stat_pdr_high_density(data['high_density_pdr'])

plt.show()
    


