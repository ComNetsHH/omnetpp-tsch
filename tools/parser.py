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

plt.rcParams.update({'font.size': 16})

def plot_hpq_delays(data):
	df = pd.DataFrame(columns=['Delay', 'Traffic', 'Repetition', 'Experiment'])
	
	app_to_traffic_map = {
		0: 'Seatbelt',
		1: 'Smoke',
		2: 'Humidity'
	}

	for sim_run in data:
		# results for each application type (smoke / humidity / seatbelts)
		for vec in data[sim_run]["vectors"]:
			mean_delay = np.mean(vec["value"])
			app_id = int(re.search('sink\[\d+\].app\[(\d+)\]', vec["module"]).group(1))
			df_row = {
					'Delay': mean_delay, 
					'Traffic': app_to_traffic_map[app_id], 
					'Repetition': data[sim_run]["attributes"]["repetition"],
					'Experiment': data[sim_run]["attributes"]["experiment"]
				}

			print(data[sim_run]["attributes"]["experiment"])

			df = df.append(df_row, ignore_index=True)

	sns.barplot(data=df, x='Traffic', y='Delay', ci=95, hue='Experiment')

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
	fig2, ax2 = plt.subplots(figsize=(10, 6))
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

filename = "hpq_delays_50_hosts.json"
plot_hpq_delays(json.load(open(results_path + filename)))

# data = json.load(res_file)

# print('data - ', json.load(pkt_arrivals_data))

# jsoned = json.load(pkt_arrivals_data)
# print(jsoned)

# print(pkt_arrivals_data)

# print()

# arrivals_data_json = json.load(pkt_arrivals_data)

# plot_inter_arrival_time(arrivals_data_json[list(arrivals_data_json.keys())[0]])
# plot_inter_arrival_time(arrivals_data_json)


# arrival_data = get_arrivals_data(data)

# plot_num_arrivals(arrival_data[list(arrival_data.keys())[0]])
# get_mean_std_dev(arrival_data)



# based on the OMNeT scavetool bug exporting JSON
# all repetitions actually store all vectors, so we can just take the first one
# repetitions = len(data[list(data.keys())[0]]["vectors"])


# print(f'found {repetitions} simulation repetitions')


plt.grid()
plt.show()
# plt.savefig(filename.split('.')[0] + '.pdf')

