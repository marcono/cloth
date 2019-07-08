import sys
import json
import csv
import os

input_args = list(sys.argv)

with open(input_args[1], 'rb') as input, open('output.csv', 'ab') as output:
    csvwriter = csv.writer(output)
    data = json.load(input)


    row = [input_args[2],
           data["Succeeded"]["Mean"], data["Succeeded"]["Variance"], data["Succeeded"]["ConfidenceMin"], data["Succeeded"]["ConfidenceMax"],
           data["FailedNoPath"]["Mean"], data["FailedNoPath"]["Variance"], data["FailedNoPath"]["ConfidenceMin"], data["FailedNoPath"]["ConfidenceMax"],
           data["FailedNoBalance"]["Mean"], data["FailedNoBalance"]["Variance"], data["FailedNoBalance"]["ConfidenceMin"], data["FailedNoBalance"]["ConfidenceMax"],
           data["FailedOffline"]["Mean"], data["FailedOffline"]["Variance"], data["FailedOffline"]["ConfidenceMin"], data["FailedOffline"]["ConfidenceMax"],
           data["AvgTime"]["Mean"], data["AvgTime"]["Variance"], data["AvgTime"]["ConfidenceMin"], data["AvgTime"]["ConfidenceMax"],
           data["AvgRoute"]["Mean"], data["AvgRoute"]["Variance"], data["AvgRoute"]["ConfidenceMin"], data["AvgRoute"]["ConfidenceMax"],
           data["AvgAttempts"]["Mean"], data["AvgAttempts"]["Variance"], data["AvgAttempts"]["ConfidenceMin"], data["AvgAttempts"]["ConfidenceMax"],
           data["Unknown"]["Mean"], data["Unknown"]["Variance"], data["Unknown"]["ConfidenceMin"], data["Unknown"]["ConfidenceMax"]
   
    ]
    csvwriter.writerow(row)


