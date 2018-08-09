# Copyright (C) 2018
# Albrecht Lohofener (albrechtloh@gmx.de)
#
# This file is part of the welle.io.
# Many of the ideas as implemented in welle.io are derived from
# other work, made available through the GNU general Public License.
# All copyrights of the original authors are recognized.
#
# welle.io is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# welle.io is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with welle.io; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
keep_last_version = int(sys.argv[4])

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
	
