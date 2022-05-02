source run_daisy_chain.sh
source export_daisy_chain.sh 'daisy_chain_test_data.json'
if [ -z "$1" ]; then
    python3 daisy_chain_test.py 'daisy_chain_test_data.json'
else
    python3 daisy_chain_test.py 'daisy_chain_test_data.json' "$1"
fi