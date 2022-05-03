# TODO: add exception handlers
source run_config.sh $1
source export_result.sh 'test_data.json'
python3 validate.py test_data.json