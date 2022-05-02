import json
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import re
import sys
import argparse
import os


def prep_dataframe(data):
    df_list = []    
    for sim_run in data:
            for vec in data[sim_run]["vectors"]:
                    mean_delay = np.mean(vec["value"])

                    df_list.append(
                            {
                                    "End-to-end delay": mean_delay, 
                                    "Repetition": data[sim_run]["attributes"]["repetition"],
                                    "Experiment": data[sim_run]["attributes"]["experiment"],
                                    "Module": vec["module"].split(".")[1]
                            }
                    )	

    return pd.DataFrame(df_list)

def plot_delay(df):
    ax = sns.barplot(data=df, x="Experiment", y="End-to-end delay", ci=95, hue="Experiment")
    plt.grid()
    plt.legend()
    plt.show()

def validate_daisy_chaining(df):
    d_normal = df[df["Experiment"] == "ReSA"]["End-to-end delay"].mean()
    d_low_lat = df[df["Experiment"] == "ReSA_Low_Latency"]["End-to-end delay"].mean()
    print(f"validating daisy-chaining, mean 6TiSCH delay: {d_normal}, daisy-chained: {d_low_lat}")
    return d_normal > d_low_lat
    

parser = argparse.ArgumentParser(description='Plot PDR / E2E delay plot for HPQ evaluation.')
parser.add_argument('result_file', metavar='f', type=str, help='path to the json file with exported simulation results')
parser.add_argument('-v', '--visual',  help='plot results', action="store_true")

args = parser.parse_args()

print(f"trying to open the file: {args.result_file} \nshow result plots? {args.visual}")
data = json.load(open(args.result_file))
print(f"successfully loaded {len(data.keys())} simulation runs")
df = prep_dataframe(data)

if args.visual:
    plot_delay(df)

exit(validate_daisy_chaining(df))



