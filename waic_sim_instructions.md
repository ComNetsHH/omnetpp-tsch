

## Running 
Use `smoke_humidity_run.sh`, `seatbelt_run.sh` to run simulations for the small-/large-scale WAIC scenarios, respectively.

## Collecting results
Due to a bug in OMNeT++ `scavetool`, you have to manually collect the results following these steps:
1. In OMNeT++ IDE double click on any `.vec` or `.sca` result file, collected from the previous step, to create a result analysis file with `.anf` extension.
2. Open the newly created `.anf` file and navigate to `Vectors`, then click on the icon shown on the screenshot `doc/waic_sim/step_2.png` to enter a filtering query.
3. Use one of the filtering queries below to collect the desired results:
  - **PDR:** `(name(packetSent**) AND module(**host**app**)) OR (name(packetReceived**) AND module(**sink**app**))`
  - **End-to-end delay:** `name(endToEndDelay:vector)`
  - **Throughput:** `name(throughput:vector) AND module(**sink**app**)`
  - **Jitter:** `name(jitter*)`
  - **Convergence Time:** `name(daisyChainStarted:vector) OR name (daisyChainEnded:vector)`
  - **Control traffic overhead:** For this stat, first navigate to `Scalars` tab in result analysis window and then use `name(controlPacketsEnq:count) OR (name(packetReceived:count) AND module(**sink**app**))` filtering query
4. Select all filtered results and use `Copy to clipboard` option from the context menu (DO NOT use `Ctrl + C`), shown on the screenshot `doc/waic_sim/step_4`, to copy the results as tab-separated CSV table.
5. Create an empty `.csv` file and paste the results from previous step.

## Visualizing results
Use `results-visualizer.py` script on the result file (following the documentation in the script) with an appropriate flag to plot the desired statistic.


