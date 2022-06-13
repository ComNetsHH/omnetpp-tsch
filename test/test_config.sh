# TODO: add exception handlers
if [[ $2 == '--skip-run' ]]; then
    echo "skipping simulation runs"
else
    source run_config.sh $1
fi
source export_result.sh 'test_data.json'
python3 validate.py test_data.json
