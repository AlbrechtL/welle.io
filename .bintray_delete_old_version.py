import requests
import json
import sys

bintray_api = 'https://api.bintray.com'
#bintray_user = 'albrechtl'
#bintray_pass = '<secret>'
#bintray_package = '/packages/albrechtl/welle.io/welle.io_next_nightly'
#keep_last_version = 20

if len(sys.argv) != 4 + 1:
    print('Error: The number of parameters is != 4')
    exit(-1)

bintray_user = sys.argv[1]
bintray_pass = sys.argv[2]
bintray_package = sys.argv[3]
keep_last_version = sys.argv[4]

# Get all versions
r = requests.get(bintray_api + bintray_package, auth=(bintray_user, bintray_pass))

# Decode data
data = json.loads(r.text)

# Extract versions
versions = data['versions']

# Extract versions to delete
delete_versions = versions[keep_last_version:len(versions)]

# Iterate over all versions to delete
for version in delete_versions:
    print('Deleting version ' + version)

    # Delete version
    r = requests.delete(bintray_api + bintray_package + '/versions/' + version, auth=(bintray_user, bintray_pass))

    # Check status
    if r.status_code != 200:
        print('Error while deleting. Status {}'.format(r.status_code))
    else:
        delete_status = json.loads(r.text)
        print('Status: ' + delete_status['message'])
	
