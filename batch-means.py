import json
import csv
import sys
from scipy.stats import t
from math import sqrt
from collections import OrderedDict
import operator
import numpy as np
import scipy.stats
import sys
import os.path
from os import path

print('COMPUTE SIMULATION OUTPUT STATS')

input_args = list(sys.argv)
if len(input_args) < 2:
     print("batch-means.py: Please specify the output directory")
     exit()
output_dir_name = input_args[1]
if not path.exists(output_dir_name):
     print("batch-means.py: Cannot find the output directory. The output will be stored in the current directory" )
     output_dir_name = './'

with open(output_dir_name + 'payments_output.csv', 'r') as csv_pay:#, open('xk.csv', 'w') as output_csv:#, open(stats_file_path, 'wb') as stats_file :

     ## INITIALIZATION
     n_batches = 30
     alfa_confidence = 0.95
     stats = ['Total', 'Success', 'FailNoPath', 'FailNoBalance', 'FailOfflineNode', 'FailTimeoutExpired', 'Time', 'Attempts', 'RouteLength']
     batches = {stat: [0]*n_batches for stat in stats}
     means = {"Total": 0, "Success": 0, "FailNoPath":0, "FailNoBalance":0, "FailOfflineNode":0, "FailTimeoutExpired":0, "Time":0, "Attempts":0,  "RouteLength":0 }
     h = {"Total": 0, "Success": 0, "FailNoPath":0, "FailNoBalance":0, "FailOfflineNode":0, "FailTimeoutExpired":0, "Time":0, "Attempts":0,  "RouteLength":0 }
     variances = {"Total": 0, "Success": 0, "FailNoPath":0, "FailNoBalance":0, "FailOfflineNode":0, "FailTimeoutExpired":0, "Time":0, "Attempts":0,  "RouteLength":0 }
     confidence_min = {"Total": 0, "Success": 0, "FailNoPath":0, "FailNoBalance":0, "FailOfflineNode":0, "FailTimeoutExpired":0, "Time":0, "Attempts":0,  "RouteLength":0 }
     confidence_max = {"Total": 0, "Success": 0, "FailNoPath":0, "FailNoBalance":0, "FailOfflineNode":0, "FailTimeoutExpired":0, "Time":0, "Attempts":0,  "RouteLength":0 }
     total_mean_time = 0
     total_mean_route = 0
     total_mean_attempts = 0
     total_succeeded = 0


     ## FIND BATCH LENGTH AND EXCLUDE TRANSIENT

     payments = list(csv.DictReader(csv_pay))
     n = len(payments)
     last_payment_time = int(payments[n-1]['start_time'])
     remainder = last_payment_time%31
     end_time = last_payment_time - remainder
     batch_length = end_time/31
     print("Batch length: " + str(batch_length) + " ms")
     print("Total simulated time: " + str(end_time-batch_length) + " ms")

     ## COMPUTE PAYMENT STATS

     for pay in payments:
          pay_start_time = int(pay['start_time'])
          pay_end_time = int(pay['end_time'])
          if pay_start_time<batch_length or pay_start_time >= end_time: continue
          b = int(pay_start_time/batch_length) -1
          batches['Total'][b] += 1
          if pay['is_success']=='1':
               batches['Success'][b] += 1
               total_succeeded += 1
               attempts = int(pay['attempts'])
               total_mean_attempts += attempts
               batches['Attempts'][b] += attempts
               time = pay_end_time - pay_start_time
               total_mean_time += time
               batches['Time'][b] += time
               routelen = len(pay['route'].split('-'))
               total_mean_route += routelen
               batches['RouteLength'][b] += routelen
          else:
               if pay['timeout_exp'] == '1':
                  batches['FailTimeoutExpired'][b] += 1
               elif pay['route'] == '-1':
                  batches['FailNoPath'][b] +=1
               else:
                    if int(pay['offline_node_count']) > int(pay['no_balance_count']):
                         batches['FailOfflineNode'][b] += 1
                    else:
                         batches['FailNoBalance'][b] += 1

     total_mean_time = float(total_mean_time)/total_succeeded
     total_mean_route = float(total_mean_route)/total_succeeded
     total_mean_attempts = float(total_mean_attempts)/total_succeeded


# COMPUTE PER-BATCH STATS

for i in range (0, n_batches):
     if batches['Success'][i] == 0:
          batches['Time'][i] = total_mean_time
          batches['RouteLength'][i] = total_mean_route
          batches['Attempts'][i] = total_mean_attempts
     else:
          batches['Time'][i] = float(batches['Time'][i])/batches['Success'][i]
          batches['RouteLength'][i] = float(batches['RouteLength'][i])/batches['Success'][i]
          batches['Attempts'][i] = float(batches['Attempts'][i])/batches['Success'][i]

     batches['Success'][i] = float(batches['Success'][i])/batches['Total'][i]
     batches['FailNoPath'][i] = float(batches['FailNoPath'][i])/batches['Total'][i]
     batches['FailNoBalance'][i] = float(batches['FailNoBalance'][i])/batches['Total'][i]
     batches['FailOfflineNode'][i] = float(batches['FailOfflineNode'][i])/batches['Total'][i]
     batches['FailTimeoutExpired'][i] = float(batches['FailTimeoutExpired'][i])/batches['Total'][i]

# COMPUTE BATCH MEANS

for stat in stats:
     means[stat] = np.mean(batches[stat])
     h[stat] = scipy.stats.sem(batches[stat]) * scipy.stats.t.isf((alfa_confidence) / 2., n_batches-1)
     confidence_min[stat] = means[stat] - h[stat]
     confidence_max[stat] = means[stat] + h[stat]
     variances[stat] = np.var(batches[stat])

# WRITE OUTPUT

with open(output_dir_name + 'cloth_output.json', 'w') as stats_file:
     dict_stats = {}
     for stat in stats:
          dict_stats[stat] = OrderedDict([
               ('Mean', '{0:.10f}'.format(means[stat])),
               ('Variance', '{0:.10f}'.format(variances[stat])),
               ('ConfidenceMin', '{0:.10f}'.format(confidence_min[stat])),
               ('ConfidenceMax', '{0:.10f}'.format(confidence_max[stat]))
          ])
     temp_output = []
     for stat in stats:
          if stat == 'Total': continue
          temp_output.append((stat, dict_stats[stat]))
     output = OrderedDict(temp_output)
     json.dump(output, stats_file, indent=4)


print('SIMULATION OUTPUT STATS SAVED IN <' + output_dir_name + 'cloth_output.json>')
