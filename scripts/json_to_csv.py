import sys
import json
import csv
import os

input_args = list(sys.argv)

with open(input_args[1], 'rb') as input, open(input_args[2], 'ab') as output:
    csvwriter = csv.writer(output)
    data = json.load(input)

    if os.stat(input_args[2]).st_size == 0:
        row = [input_args[3], 'TotalPayments', 'SucceededPayments', 'FailedPaymentsUncoop', 'FailedPaymentsNoBalance', 'FailedPaymentsNoPath', 'AvgTimeCoop', 'AvgTimeUncoop', 'AvgRouteLen', 'LockedFundCost']
        csvwriter.writerow(row)

    row = [input_args[4], data["TotalPayments"], data["SucceededPayments"]["Mean"], data["FailedPaymentsUncoop"]["Mean"], data["FailedPaymentsNoBalance"]["Mean"], data["FailedPaymentsNoPath"]["Mean"], data["AvgTimeCoop"]["Mean"], data["AvgTimeUncoop"]["Mean"], data["AvgRouteLen"]["Mean"], data["LockedFundCost"]["Mean"] ]
    csvwriter.writerow(row)


