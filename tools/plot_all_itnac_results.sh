itnac_visualizer $1/itnac_case_1.json -b -pdr -cd 923 -bl -s pdr_case_1.$2
itnac_visualizer $1/itnac_case_2.json -b -pdr -cd 923 -ng -bl -s pdr_case_2.$2
itnac_visualizer $1/itnac_case_1.json -dbe -bl -s delay_case_1.$2
itnac_visualizer $1/itnac_case_2.json -dbe -bl -ng -s delay_case_2.$2
