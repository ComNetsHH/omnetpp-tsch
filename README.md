# TSCH

OMNeT++ simulation model for IEEE 802.15.4e Time Slotted Channel Hopping (TSCH) with 6P protocol and scheduling functions as part of the 6TiSCH stack (combined with [RPL](https://github.com/ComNetsHH/omnetpp-rpl))

## Compatibility

This model is developed and tested with the following library versions

*  OMNeT++ [5.6.X](https://omnetpp.org/download/)
*  INET [4.2.X](https://github.com/inet-framework/inet/releases/download/v4.2.5/inet-4.2.5-src.tgz)

## Installation
1. To use with 6TiSCH, import RPL project from https://github.com/ComNetsHH/omnetpp-rpl first and follow its installation instructions.
2. Replace _respective_ INET source files with the ones included in this repo under `inet_replacement_files` folder and rebuild INET.
3. Add INET to project references by navigating TSCH project 'Properties' -> 'Project References'
4. Add RPL project source directory to compile options of TSCH project by navigating `Properties` -> `OMNeT++` -> `Makemake` -> (select `src` folder) -> `Build Makemake Options...` -> `Compile` tab -> add absolute path containing RPL 'src' folder, e.g. '/home/yevhenii/omnetpp-5.6.2/samples/omnetpp-rpl/src'. Also make sure `Add include paths exported from referenced projects` is enabled
5. In RPL `Properties`->`OMNeT++`-> `Makemake`-> (select `src` folder) -> `Build Makemake Options...`: 
   - Set `Target` to `Shared library` and enable `Export this shared library...`

## 6TiSCH-HPQ Simulations
0. Make sure OMNeT++ folder is on $PATH by `./setenv` in its root directory.
1. Simulating and plotting avionic scenarios:
    - Navigate to `tools/hpq_sim`
    - Use `hpq_run.sh` to run simulation campagins. Additional arguments are documented in `tools/parser.py`. Some usage examples: `./hpq_run.sh 200 1 --on` will run simulations for 200 nodes and a single sink with HPQ enabled, while e.g. `./hpq_run.sh 200 3 --off` would disable the HPQ mechanisms (default 6TiSCH) and use 3 sinks.
    - Plot the results using corresponding shell scripts. Here definitely refer to the `parser.py` command-line arguments explanation. Some examples: `./plot_hpq_delay.sh 200 1` would export result vector data into a JSON and plot end-to-end delay for all applications in a scenario with 200 nodes and 1 sink. Using `./plot_hpq_delay.sh 200 1 -o` would do the same but with forced re-export of the vector data, in case you changed simulation parameters and want to re-run everything.
2. Simulating and plotting HPQ model validation scenrios:
    - Navigate to `tools/hpq_verification`
    - Use `hpq_run...` scripts to run simulation campagins, no extra arguments needed
    - For plotting you can use either shell script or `parser.py` directly, following the example in shell scripts. However you need to explicitly provide CSV result file as an argument.
    - To create a CSV result file, open the desired results using OMNeT++ result file analysis tool in GUI:
        1. Locate the folder with the results, e.g. `hpq_verif`
        2. Double-click on one of the `.vec` or `.sca` files inside to create an `.anf` file
        3. Open the created `.anf` file and navigate to Vectors tab.
        4. Apply statistic name filter "endToEndDelay:vector" and module filter "HighDensity.sink[0].app[1]".
        5. Select all of the rows and copy them by clicking on "Copy to clipboard" as shown in the screenshot.
        6. Create a `.csv` file and paste there.
<img width="1832" alt="Screenshot 2022-02-11 at 18 02 03" src="https://user-images.githubusercontent.com/31932610/153635819-3232c5fa-0cc6-46d5-a3e6-971ff41daef2.png">

 
