import csv
import sys

input_args = list(sys.argv)

stat = input_args[1]

with open(stat, 'rb') as input_file, open(stat+'.txt', 'wb') as output_file:
    reader =  csv.DictReader(input_file)

    for row in reader:
        output_file.write('%d & %.2f\%% & %.2f\%% & %.2f\%% & %.2f\%% & %.2f & %.2f & %.2f  \\\\ \n' % (
            int(row['RemovedNodes']),
            float(row['SucceededMean'])*100,
            float(row['FailedNoPathMean'])*100,
            float(row['FailedNoBalanceMean'])*100,
            float(row['FailedTimeoutMean'])*100,
            float(row['AvgTimeMean']),
            float(row['AvgRouteMean']),
            float(row['AvgAttemptsMean'])
        )

        )


#    print stat


