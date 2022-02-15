import json
import math
# import sys
import numpy as np
# from scipy.stats import poisson, chisquare, chi2
# from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import re
import sys
import argparse
import os
import time

# Uncomment only for HPQ verification plots
# sns.set_palette("Paired")

plt.rcParams.update({'font.size': 20})
plt.rcParams.update({'errorbar.capsize': 3.5})

app_to_traffic_map = {
	0: 'Seat belt',
	1: 'Smoke',
	2: 'Humidity'
}


def waiting_time_be(r, rho_be, rho_np, wrr_be, wrr_np, rho_sp = 0):
	wrr_sum = wrr_be + wrr_np

	if rho_sp > 0:
		wrr_total_rho = 1 - rho_sp
		rho_np_max = wrr_total_rho * wrr_np / wrr_sum
		rho_np = rho_np if rho_np < rho_np_max else rho_np
		
		try:

			sp_limiter = 1 - rho_sp
			rho_be_modifier = 1 + rho_np * wrr_sum / wrr_be 
			rho_be_real = 1 - rho_sp - rho_be * rho_be_modifier
			denom = sp_limiter * rho_be_real

			w_be = r / denom

			return w_be if w_be > 0 else 9999999
		except:
			return 999999999

	else:
		try:
			rho_np = rho_np if rho_np < wrr_np / wrr_sum else wrr_np / wrr_sum
			w_be = r * (1 + rho_np * wrr_sum / wrr_be ) / (1 - rho_be * (1 + rho_np * wrr_sum / wrr_be) )

			# w_be = r * (1 + rho_np * wrr_np) / (1 - rho_be * (1 + rho_np * wrr_sum / wrr_be) )

			return w_be if w_be > 0 else 999999999
		except:
			return 999999999

def waiting_time_np(r, rho_be, rho_np, wrr_be, wrr_np, rho_sp = 0):
	wrr_sum = wrr_be + wrr_np

	if rho_sp > 0:
		wrr_total_rho = 1 - rho_sp
		rho_be_max = wrr_total_rho * wrr_be / wrr_sum
		rho_be = rho_be if rho_be < rho_be_max else rho_be
		
		try:
			w_np = r / ((1 - rho_sp) * (1 - rho_sp - rho_np * (1 + rho_be * wrr_sum / wrr_np)))
		except:
			return 0
		return w_np if w_np > 0 else 0

	else:
		try:
			rho_be = rho_be if rho_be < wrr_be / wrr_sum else wrr_be / wrr_sum
			w_np_wrr = r / (1 - rho_np * (1 + rho_be * wrr_sum / wrr_np) )
			return w_np_wrr if w_np_wrr > 0 else 0
		except:
			return 0

def waiting_time_single_queue(r, rho):
	return r / (1 - rho)

def plot_hpq_theoretical_delays(traffic_class, two_queues = False, wrr_be = 1, wrr_np = 4):
	rhos = np.arange(0, 0.33, 0.01)

	if traffic_class == 'be':
		waiting_times_be = [waiting_time_be(0.5, r, r, wrr_be=wrr_be, wrr_np=wrr_np, rho_sp=r) for r in rhos]
		sns.lineplot(x=rhos, y=waiting_times_be, label='BE (expected)', ls='--')
	elif traffic_class == 'sp':
		waiting_times_sp = [waiting_time_single_queue(0.5, r) for r in rhos]
		sns.lineplot(x=rhos, y=waiting_times_sp, label='SP (expected)', ls='--')
	elif traffic_class == 'np':
		waiting_times_np = [waiting_time_np(0.5, r if not two_queues else 0, r, wrr_be=wrr_be, wrr_np=wrr_np, rho_sp=r) for r in rhos]
		sns.lineplot(x=rhos, y=waiting_times_np, label='NP (expected)', ls='--')
	
def plot_hpq_theoretical_be_wrr(wrr_be = 1, wrr_np = 4, num_steps = 3, rho_np = 0):
	be_rhos = np.arange(0, 1, 0.01)

	if rho_np > 0:
		waiting_times_be = [waiting_time_be(0.5, rho, rho_np, wrr_be=wrr_be, wrr_np=wrr_np) for rho in be_rhos]
		sns.lineplot(be_rhos, waiting_times_be, label=f"NP utilization = {rho_np} (expected)")
		return

	rho_np_max = wrr_np / (wrr_be + wrr_np)

	np_rhos = list(np.arange(start=0, stop=rho_np_max, step=rho_np_max/num_steps))
	np_rhos.append(rho_np_max)

	for np_rho in np_rhos:
		waiting_times_be = [waiting_time_be(0.5, rho, np_rho, wrr_be=wrr_be, wrr_np=wrr_np) for rho in be_rhos]
		sns.lineplot(be_rhos, waiting_times_be, label=f"rho_np = {np_rho}")


def plot_hpq_be_delays_wrr_effect(wrr_be, wrr_np = 4, rho_np = 0.5):
	rhos = np.arange(0, 1, 0.01)
	waiting_times_be = [waiting_time_be(0.5, rho, wrr_np / (wrr_be + wrr_np), wrr_be=wrr_be, wrr_np=wrr_np) for rho in rhos]
	sns.lineplot(rhos, waiting_times_be, label=f"WT_be = {wrr_be} (expected)", ls='--')

def plot_hpq_theoretical_np_be_wrr(wrr_be = 1, wrr_np = 4):
	rhos = np.arange(0, 1, 0.01)

	slotframe_duration = 1.01 
	n_cells = 1

	y_be = [waiting_time_be(slotframe_duration/(n_cells+1), rho, rho, wrr_be=wrr_be, wrr_np=wrr_np) for rho in rhos]
	y_np = [waiting_time_np(slotframe_duration/(n_cells+1), rho, rho, wrr_be=wrr_be, wrr_np=wrr_np) for rho in rhos]

	sns.lineplot(rhos, y_be, label="BE")
	sns.lineplot(rhos, y_np, label="NP")


def plot_hpq_pdr(data, app_to_traffic_map):
	df = pd.DataFrame(columns=['PDR', 'Traffic type', 'Repetition', 'Experiment'])

	for sim_run in data:
		# results for each application type (smoke / humidity / seatbelts)
		pkt_rec = 0
		pkt_sent = 0
		app_id = -1
		repetition = data[sim_run]["attributes"]["repetition"]
		experiment = data[sim_run]["attributes"]["experiment"]

		pdr_map = {
			x: {"sent": 0, "rec": 0} for x in app_to_traffic_map.keys()
		}

		# Accumulate all sent / received packets for each app
		for sca in data[sim_run]["scalars"]:
			app_id = app_id = int(re.search('app\[(\d+)\]', sca["module"]).group(1))

			if "packetReceived" in sca["name"]:
				pdr_map[app_id]["rec"] += sca["value"]

			if "packetSent" in sca["name"]:
				pdr_map[app_id]["sent"] += sca["value"]
		
		# Append a row to dataframe with PDR for each app and repetition
		for app_id in pdr_map:
			if app_id == 1:
				print(f"{experiment}, rep #{repetition}, app[{app_id}].received = {pdr_map[app_id]['rec']}, sent = {pdr_map[app_id]['sent']}")
			df_row = {
				'PDR': (pdr_map[app_id]["rec"] / pdr_map[app_id]["sent"]) if pdr_map[app_id]["sent"] > 0 else 0,
				'Traffic type': app_to_traffic_map[app_id],
				'Repetition': repetition,
				'Experiment': experiment
				}
			df = df.append(df_row, ignore_index=True)

	df['Experiment'] = ['6TiSCH-HPQ' if x == 'HPQ' else '6TiSCH' for x in df['Experiment'].tolist()]
	df = df.sort_values(by=['Traffic type'])
	ax = sns.barplot(data=df, x='Traffic type', y='PDR', ci=95, hue='Experiment', hue_order=['6TiSCH-HPQ', '6TiSCH'])

	remove_legend_subtitle(ax)

def plot_hpq_wrr_verification(data):
	service_utils = [float(re.search('rateBe=([+-]?([0-9]*[.])?[0-9]+)', x).group(1)) for x in data['Measurement'].tolist()]
	data['Service Utilization'] = service_utils
	be_weights = [int(re.search('wrrBe=(\d+)', x).group(1)) for x in data['Measurement'].tolist()]
	data['BE weight'] = be_weights

	be_weights_unique = data['BE weight'].unique().tolist()
	be_weights_unique.sort()

	# skip palette colors
	sns.lineplot(x=[0, 0], y=[0, 0], legend=None)
	sns.lineplot(x=[0, 0], y=[0, 0], legend=None)
	sns.lineplot(x=[0, 0], y=[0, 0], legend=None)
	sns.lineplot(x=[0, 0], y=[0, 0], legend=None)

	for wt_be in be_weights_unique:
		ax = sns.lineplot(data=data[data['BE weight'] == wt_be], x='Service Utilization', y='Mean', 
			ci=95, err_style='bars', label=f'WT_be = {wt_be} (measured)')
		plot_hpq_be_delays_wrr_effect(wt_be)

	ax.set_ylabel('Sojourn time (s)')
	ax.set_xlabel('Arrival rate (pkt/s)')
	remove_legend_subtitle(ax)
	ax.set_ylim([0, 10])

def plot_hpq_wrr_be_verif(data):
	np_utils = [float(re.search('rateNp=([+-]?([0-9]*[.])?[0-9]+)', x).group(1)) for x in data['Measurement'].tolist()]
	data['NP utilization'] = np_utils
	data['Service Utilization'] = [float(re.search('rateBe=([+-]?([0-9]*[.])?[0-9]+)', x).group(1)) for x in data['Measurement'].tolist()]

	np_utils_uniq = list(dict.fromkeys(np_utils))

	for np_util in np_utils_uniq:
		ax = sns.lineplot(data=data[data['NP utilization'] == np_util], x='Service Utilization', y='Mean', 
			ci=95, err_style='bars', label=f'NP utilization = {np_util} (measured)')	
		plot_hpq_theoretical_be_wrr(rho_np=np_util)

	ax.set_ylabel('End-to-end delay (s)')
	remove_legend_subtitle(ax)
	ax.set_ylim([0, 10])


def plot_hpq_delays_verification(data, app_to_traffic_map):
	data['Service Utilization'] = [float( (x.split(',')[0]).split("=")[1]) for x in data['Measurement'].tolist()]
	app_ids = [int(re.search('sink\[\d+\].app\[(\d+)\]', x).group(1)) for x in data['Module'].tolist()]
	data['Traffic Class'] = [app_to_traffic_map[x] for x in app_ids]
	
	traffic_classes = list(app_to_traffic_map.values())
	for tc in traffic_classes:
		ax = sns.lineplot(data=data[data['Traffic Class'] == tc], x='Service Utilization', y='Mean', 
			ci=95, err_style='bars', label=f'{tc} (measured)')	
		plot_hpq_theoretical_delays(traffic_class=tc.lower(), two_queues=len(traffic_classes) == 2)

	ax.set_ylabel('Sojourn time (s)')
	ax.set_xlabel('Arrival rate (pkt/s)')
	remove_legend_subtitle(ax)
	if len(traffic_classes) > 2: 
		ax.set_ylim([0, 10])

def plot_hpq_delays(data, app_to_traffic_map, compare_num_sinks = False):
	# df = pd.DataFrame(columns=['Delay', 'Traffic type', 'Repetition', 'Experiment', 'Number of sinks'])

	df_list = []
	for sim_run in data:
		# results for each application type (smoke / humidity / seatbelts)
		for vec in data[sim_run]["vectors"]:
			mean_delay = np.mean(vec["value"])
			app_id = int(re.search('sink\[\d+\].app\[(\d+)\]', vec["module"]).group(1))
			if app_id not in app_to_traffic_map:
				continue

			df_list.append(
				{
					'Delay': mean_delay, 
					'Traffic type': app_to_traffic_map[app_id], 
					'Repetition': data[sim_run]["attributes"]["repetition"],
					'Experiment': data[sim_run]["attributes"]["experiment"],
					'Number of sinks': data[sim_run]["itervars"]["numSinks"]
				}
			)	

			# df_row = {
			# 		'Delay': mean_delay, 
			# 		'Traffic type': app_to_traffic_map[app_id], 
			# 		'Repetition': data[sim_run]["attributes"]["repetition"],
			# 		'Experiment': data[sim_run]["attributes"]["experiment"],
			# 		'Number of sinks': data[sim_run]["itervars"]["numSinks"]
			# 	}

			# print(data[sim_run]["attributes"]["experiment"])

			# df = df.append(df_row, ignore_index=True)

	df = pd.DataFrame(df_list)

	df['Experiment'] = ['6TiSCH-HPQ' if x == 'HPQ' else '6TiSCH' for x in df['Experiment'].tolist()]

	# df = df[df['Delay'] < 12]	

	df = df.sort_values(by=['Number of sinks' if compare_num_sinks else 'Traffic type'])
	ax = sns.barplot(data=df, x='Number of sinks' if compare_num_sinks else 'Traffic type', y='Delay', ci=95, hue='Experiment', 
		hue_order=['6TiSCH-HPQ', '6TiSCH'])

	i = -1
	for p in ax.patches:
		i += 1
		if compare_num_sinks:
			ax.annotate(format(p.get_height(), '.1f'), 
				(p.get_x() + p.get_width() / 2., p.get_height()), 
				ha = 'right', va = 'center', 
				xytext = (0, 9), 
				textcoords = 'offset points')
		elif i == 2 or i == 5:
			ax.annotate(format(p.get_height(), '.1f'), 
				(p.get_x() + p.get_width() / 2., p.get_height()), 
				ha = 'right', va = 'center', 
				xytext = (0, 9), 
				textcoords = 'offset points')


	ax.set_ylabel('End-to-end delay (s)')
	remove_legend_subtitle(ax)
	

def remove_legend_subtitle(ax):
	handles, labels = ax.get_legend_handles_labels()
	ax.legend(handles=handles, labels=labels)

def plot_hpq_delay_cdf(data, app_to_traffic_map):
	# df = pd.DataFrame(columns=['Delay', 'Traffic type', 'Experiment'], index=range(data.keys()))

	delays_per_app = {}

	for sim_run in data:

		experiment = data[sim_run]["attributes"]["experiment"]

		# for each config / experiment initialize an entry with app ids as keys and empty list as value
		# the list is then used to collect all the observed delay samples for this app
		if experiment not in delays_per_app:
			delays_per_app[experiment] = {x: [] for x in app_to_traffic_map.keys()}

		for vec in data[sim_run]["vectors"]:
			app_id = int(re.search('sink\[\d+\].app\[(\d+)\]', vec["module"]).group(1))
			if app_id not in app_to_traffic_map.keys():
				continue

			delays_per_app[experiment][app_id].extend(vec["value"])


	df_list = []
	for exp in delays_per_app: 
		for app_id in delays_per_app[exp]:
			for val in delays_per_app[exp][app_id]:
				df_list.append(
					{
					'Delay': val, 
					'Traffic type': app_to_traffic_map[app_id], 
					'Experiment': exp
					}
				)
				
				# row = {
				# 	'Delay': val, 
				# 	'Traffic type': app_to_traffic_map[app_id], 
				# 	'Experiment': exp
				# }
				# df = df.append(row, ignore_index=True)

	df = pd.DataFrame(df_list)
	
	df = df.sort_values(by=['Experiment'])
	ax = sns.ecdfplot(data=df, x="Delay", hue='Experiment', hue_order=['HPQ', 'HPQ_Off'])
	ax.get_legend().set_title(None)
	ax.legend(labels=['6TiSCH', '6TiSCH-HPQ'])

	ax.set(xlim=(0, 2)) # was 5
	# ax.set_axis_labels("End-to-end delay (s)", "")
	ax.set_ylabel("")
	ax.set_xlabel("End-to-end delay (s)")
	plot_percentile(ax)
	
def plot_percentile(ax, percentile = 0.95):
	percentile_x_values = []

	for line in ax.lines:
		xs = line.get_xdata()
		ys = line.get_ydata()

		percentile_index = [idx for idx, el in enumerate(ys) if el >= percentile][0]
		print(f"found x value for {percentile} percentile: {xs[percentile_index]}")

		x_value = xs[percentile_index]
		percentile_x_values.append(x_value)

		height = np.interp(x_value, xs, ys)
		ax.vlines(x_value, 0, height, color='crimson', ls='--', linewidth=1.5)

		# ax.text(x_value, -.02, f'{round(x_value, 2)}', color='crimson', ha='left', va='top') # ha='right' if xs[percentile_index] < 2 else 'left' 

	ax.hlines(percentile, xmin=0, xmax=max(percentile_x_values), color='crimson', ls='--', linewidth=1.5)
	ax.text(-.20, percentile, f'{percentile}', color='crimson', ha='left', va='top')
	# ax.fill_between(xs, 0, ys, facecolor='crimson', alpha=0.2)

def plot_inter_arrival_time(data):
	df = pd.DataFrame(columns=['Interarrival Bin', 'Occurrences', 'Repetition'])
	i = 0

	fig1, ax1 = plt.subplots()
	fig2, ax2 = plt.subplots()

	# key = repetition run
	for key in data:
		timestamps = data[key]["vectors"][0]["time"]
		interarrivals = np.array(timestamps[1:]) - np.array(timestamps[:-1])
		obs, bin_edges, patches = ax1.hist(interarrivals)
		bin_centers = 0.5 * (bin_edges[:-1] + bin_edges[1:])

		for bin_center, num_occurences in zip(bin_centers, obs):	
			row = {'Interarrival Bin': bin_center, 'Occurrences': num_occurences, 'Repetition': i}
			df = df.append(row, ignore_index=True)

		i += 1

	sns.set_color_codes("muted")
	ax = sns.barplot(x="Interarrival Bin", y="Occurrences", data=df, color='b')
	ax.set_xlabel("Packets per second")
	ax.set_ylabel("Number of occurrences")

	plt.grid()
	plt.show()
	plt.show()

	# print('ias: ', interarrivals)


def get_num_arrivals(single_run_data):
	num_arrivals = []
	second = single_run_data["vectors"][0]["time"][0]

	for time in single_run_data["vectors"][0]["time"]:
		while math.floor(time) > second:
			num_arrivals.append(0)
			second += 1

		try:
			num_arrivals[-1] += 1
		except:
			print('')

	return num_arrivals
	
def get_mean_std_dev(arrivals_data):
	# i - number of nodes / hop distance to sink
	std_devs = {i: [] for i in arrival_data}
	for run in arrivals_data:
		for repetition in arrivals_data[run]: 
			std_devs[run].append( np.std(repetition) )
		# std_devs[run] = np.mean(std_devs[run])

	print('all std_devs per hops: ', std_devs)	
	
	return std_devs

def get_arrivals_data(data):
	arrival_data = {}

	for run in data:
		num_nodes = data[run]['itervars']['N']
		if num_nodes not in arrival_data:
			arrival_data[num_nodes] = []
		arrival_data[num_nodes].append(get_num_arrivals(data[run]))

	# # convert nested list-of-lists into a dictionary
	# for num_nodes in arrival_data:
	# 	converted = {}
	# 	for repetition, rep_data in enumerate(arrival_data[num_nodes]):
	# 		converted[repetition] = rep_data

	# 	arrival_data[num_nodes] = converted	

	print('arrival data (no conversion) - ', arrival_data)

	return arrival_data

def plot_num_arrivals(samples):
	fig1, ax1 = plt.subplots() # used only to contain all histograms before merging them in seaborn
	fig2, ax2 = plt.subplots(figsize=(8, 6), dpi=600)
	max_y = 0

	df = pd.DataFrame(columns=['Arrived', 'Occurrences', 'Repetition'])
	i = 0
	means = []

	for s in samples:
		means.append(np.mean(s))
		# Create and plot histogram of observed sample data
		bin_edges = np.arange(max(s) + 2) - 0.5
		bin_centers = 0.5 * (bin_edges[:-1] + bin_edges[1:])
		obs, bin_edges, patches = ax1.hist(s, bins=bin_edges)

		for bin_center, num_occurences in zip(bin_centers, obs):	
			row = {'Arrived': bin_center, 'Occurrences': num_occurences, 'Repetition': i}
			df = df.append(row, ignore_index=True)

		i += 1

	sns.set_color_codes("muted")
	ax = sns.barplot(x="Arrived", y="Occurrences", data=df, color='b')
	ax.set_xlabel("Packets per second")
	ax.set_ylabel("Number of occurrences")

	# TODO: calc y_max
	ax.axvline(x=np.mean(means), ymax=500, color='red', label='Mean')
	ax.legend()

	# plt.savefig('arrivals_distribution.pdf')


results_path = "/Users/yevhenii/omnetpp-5.6.2/samples/tsch-master/simulations/wireless/waic/"
# results_filename = "arrival_vector_1_hop_fixed.json"
# packet_arrivals_filename = "arrivals_data_5_hop.json"

# pkt_arrivals_data = open(results_path + packet_arrivals_filename)
# res_file = open(results_path + results_filename)

# filename = "hpq_delays_50_hosts.json"

# fig, ax = plt.subplots(figsize=(8, 6), dpi=100)
# ax.set_ylim([0, 10])
# plot_hpq_theoretical_np_be_wrr()
# plot_hpq_theoretical_be_wrr()
# plot_hpq_theoretical_delays()
# plt.grid()
# plt.show()


parser = argparse.ArgumentParser(description='Plot PDR / E2E delay plot for HPQ evaluation.')
parser.add_argument('result_file', metavar='f', type=str, help='path to the json file with exported simulation results')
parser.add_argument('-d', '--delay', help='plot end-to-end delay', action='store_true')
parser.add_argument('-ds', '--delay-smoke', help='plot end-to-end delay of smoke app for different number of sinks', action='store_true')
parser.add_argument('-p', '--pdr', help='plot PDR', action='store_true')
parser.add_argument('-c', '--cdf', help='plot CDF of delay', action='store_true')
parser.add_argument('-s', '--save', help='save the plot in PDF', action='store_true')
parser.add_argument('-t', '--verification', help='verify expected HPQ delays over 1 hop communication', action='store_true')
parser.add_argument('-t2', '--verification-two-queues', dest='t2', help='verify expected HPQ delays in 1 hop case with two traffic classes', action='store_true')
parser.add_argument('-w', '--wrr', help='verify expected effect of WRR weights in HPQ model', action='store_true')

args = parser.parse_args()
print("result file: ", args.result_file)

fig, ax = plt.subplots(figsize=(9, 6), dpi=600)
if args.verification:
	data = pd.read_csv(args.result_file, sep="\t")

	app_to_traff_map = {
		0: 'SP',
		1: 'NP',
		2: 'BE'
	}

	plot_hpq_delays_verification(data, app_to_traff_map)
	
elif args.t2:
	data = pd.read_csv(args.result_file, sep="\t")
	plot_hpq_delays_verification(data, {0: 'SP', 1: 'NP'})
elif args.wrr:
	data = pd.read_csv(args.result_file, sep="\t")
	# plot_hpq_wrr_be_verif(data)
	plot_hpq_wrr_verification(data)
	
else:
	data = json.load(open(args.result_file))
	if args.delay:
		print('plotting delay')
		plot_hpq_delays(data, app_to_traffic_map)
	elif args.pdr:
		print('plotting PDR')
		plot_hpq_pdr(data, app_to_traffic_map)
	elif args.cdf:
		print('plotting delay CDF')
		start = time.time()
		plot_hpq_delay_cdf(data, {1: 'Smoke'})
		print(f'elapsed: {time.time() - start}s')
	elif args.delay_smoke:
		print("plotting delay of smoke app for different number of sinks")
		plot_hpq_delays(data, app_to_traffic_map={1: 'Smoke alarm'}, compare_num_sinks=True)	
	

# # data = json.load(res_file)

# # print('data - ', json.load(pkt_arrivals_data))

# # jsoned = json.load(pkt_arrivals_data)
# # print(jsoned)

# # print(pkt_arrivals_data)

# # print()

# # arrivals_data_json = json.load(pkt_arrivals_data)

# # plot_inter_arrival_time(arrivals_data_json[list(arrivals_data_json.keys())[0]])
# # plot_inter_arrival_time(arrivals_data_json)


# # arrival_data = get_arrivals_data(data)

# # plot_num_arrivals(arrival_data[list(arrival_data.keys())[0]])
# # get_mean_std_dev(arrival_data)



# # based on the OMNeT scavetool bug exporting JSON
# # all repetitions actually store all vectors, so we can just take the first one
# # repetitions = len(data[list(data.keys())[0]]["vectors"])


# # print(f'found {repetitions} simulation repetitions')

# plt.tight_layout()
plt.grid()

if args.save:
	out_name = args.result_file.split('/')[-1].split('.')[0]
	if args.cdf:
		out_name += '_cdf'
	out_name += '.jpg'
	print("saving plot as " + out_name)
	plt.savefig(out_name)
else:
	plt.show()

