import json
import csv

with open('channels-per-node.json', 'rb') as input,  open('channels-per-node.csv', 'wb') as output:
    data = json.load(input)
    csvwriter = csv.writer(output)

    for node in data:
        row = [node[1]]
        csvwriter.writerow(row)

