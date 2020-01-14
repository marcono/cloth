import json
import csv
import sys
from scipy.stats import t
from math import sqrt
from collections import OrderedDict
import pandas as pd

input_args = list(sys.argv)

path = input_args[1]
pay_file_name = 'payments_output.csv'
stats_file_name = 'output.json'
pay_file_path = path + pay_file_name
stats_file_path = path + stats_file_name

n_batches = 30
# end = int(input_args[2])
# transient = int(input_args[3])
# n_batches = 30
# delta = (end-transient)/30
alfa_confidence = 0.95
percentile = t.isf(alfa_confidence/2, n_batches-1)


stats = ['Total', 'Succeeded', 'FailedNoBalance', 'FailedOffline', 'FailedNoPath', 'FailedTimeout', 'Unknown', 'AvgTime', 'MinTime', 'MaxTime', 'AvgAttempts', 'MinAttempts', 'MaxAttempts',  'AvgRoute', 'MinRoute', 'MaxRoute']

batches = [{"Total": 0, "Succeeded": 0, "FailedNoPath":0, "FailedNoBalance":0, "FailedOffline":0, "FailedTimeout":0, "Unknown":0, "AvgAttempts":0, "MinAttempts":sys.maxint,  "MaxAttempts":-1, "AvgTime":0, "MinTime":sys.maxint, "MaxTime":-1, "AvgRoute":0, "MinRoute":sys.maxint, "MaxRoute":-1 } for i in range (0, n_batches)]

means = {"Total": 0, "Succeeded": 0, "FailedNoPath":0, "FailedNoBalance":0, "FailedOffline":0, "FailedTimeout":0, "Unknown":0, "AvgAttempts":0, "MinAttempts":0,  "MaxAttempts":0, "AvgTime":0, "MinTime":0, "MaxTime":0, "AvgRoute":0, "MinRoute":0, "MaxRoute":0 }

variances = {"Total": 0, "Succeeded": 0, "FailedNoPath":0, "FailedNoBalance":0, "FailedOffline":0, "FailedTimeout":0, "Unknown":0, "AvgAttempts":0, "MinAttempts":0,  "MaxAttempts":0, "AvgTime":0, "MinTime":0, "MaxTime":0, "AvgRoute":0, "MinRoute":0, "MaxRoute":0 }

confidence_min = {"Total": 0, "Succeeded": 0, "FailedNoPath":0, "FailedNoBalance":0, "FailedOffline":0, "FailedTimeout":0, "Unknown":0, "AvgAttempts":0, "MinAttempts":0,  "MaxAttempts":0, "AvgTime":0, "MinTime":0, "MaxTime":0, "AvgRoute":0, "MinRoute":0, "MaxRoute":0 }

confidence_max = {"Total": 0, "Succeeded": 0, "FailedNoPath":0, "FailedNoBalance":0, "FailedOffline":0, "FailedTimeout":0, "Unknown":0, "AvgAttempts":0, "MinAttempts":0,  "MaxAttempts":0, "AvgTime":0, "MinTime":0, "MaxTime":0, "AvgRoute":0, "MinRoute":0, "MaxRoute":0 }

total_mean_time = 0
total_mean_route = 0
total_mean_attempts = 0
total_succeeded = 0


with open(pay_file_path, 'rb') as csv_pay:#, open(stats_file_path, 'wb') as stats_file :

     ## FIND TRANSIENT
     panda_reader = pd.read_csv(csv_pay)

     end_time = panda_reader['time'].max()
     print 'end_time: ', end_time

     for x in range(300, end_time):
          if (end_time-x)%n_batches == 0:
               transient = x
               break
     delta = (end_time - transient)/n_batches

     print 'transient: ', transient
     print 'delta_batch: ', delta


with open(pay_file_path, 'rb') as csv_pay, open(stats_file_path, 'wb') as stats_file:


     ##GET STATS FROM FILE


     pay_reader = csv.DictReader(csv_pay)


     for pay in pay_reader:

         pay_end_time = int(pay['end_time'])

         if pay_end_time < transient or pay_end_time >= end_time:
             continue

         b = int((pay_end_time-transient)/delta)


         batches[b]['Total'] += 1

         if pay['is_success']=='1':
             batches[b]['Succeeded'] += 1
             total_succeeded += 1

             attempts = int(pay['attempts'])
             total_mean_attempts += attempts
             batches[b]['AvgAttempts'] += attempts
             if attempts > batches[b]['MaxAttempts']:
                  batches[b]['MaxAttempts'] = attempts
             if attempts < batches[b]['MinAttempts']:
                  batches[b]['MinAttempts'] = attempts

             duration = pay_end_time - int(pay['time']);
             total_mean_time += duration
             batches[b]['AvgTime'] += duration
             if duration > batches[b]['MaxTime']:
                  batches[b]['MaxTime'] = duration
             if duration < batches[b]['MinTime']:
                  batches[b]['MinTime'] = duration

             routelen = len(pay['route'].split('-'))
             total_mean_route += routelen
             batches[b]['AvgRoute'] += routelen
             if routelen > batches[b]['MaxRoute']:
                  batches[b]['MaxRoute'] = routelen
             if routelen < batches[b]['MinRoute']:
                  batches[b]['MinRoute'] = routelen

         else:
             if pay['is_timeout'] == '1':
                  batches[b]['FailedTimeout'] += 1
             elif pay['route'] == '-1':
                 batches[b]['FailedNoPath'] +=1
             elif pay['uncoop_after'] == '1':
                 batches[b]['Unknown'] += 1
             elif pay['uncoop_before'] == '1':
                 batches[b]['FailedOffline'] += 1
             else:
                 batches[b]['FailedNoBalance'] += 1

     total_mean_time = float(total_mean_time)/total_succeeded
     total_mean_route = float(total_mean_route)/total_succeeded
     total_mean_attempts = float(total_mean_attempts)/total_succeeded
     print total_mean_time, total_mean_route

     ## COMPUTE PER-BATCH STATS

     for i in range (0, n_batches):
          print batches[i]['Total']


          if batches[i]['Succeeded'] == 0:
               batches[i]['AvgTime'] = total_mean_time
               batches[i]['AvgRoute'] = total_mean_route
               batches[i]['AvgAttempts'] = total_mean_attempts
          else:
               batches[i]['AvgTime'] = float(batches[i]['AvgTime'])/batches[i]['Succeeded']
               batches[i]['AvgRoute'] = float(batches[i]['AvgRoute'])/batches[i]['Succeeded']
               batches[i]['AvgAttempts'] = float(batches[i]['AvgAttempts'])/batches[i]['Succeeded']


          if batches[i]['Total'] == 0:
               print i

          batches[i]['Succeeded'] = float(batches[i]['Succeeded'])/batches[i]['Total']
          batches[i]['FailedNoPath'] = float(batches[i]['FailedNoPath'])/batches[i]['Total']
          batches[i]['FailedNoBalance'] = float(batches[i]['FailedNoBalance'])/batches[i]['Total']
          batches[i]['FailedOffline'] = float(batches[i]['FailedOffline'])/batches[i]['Total']
          batches[i]['FailedTimeout'] = float(batches[i]['FailedTimeout'])/batches[i]['Total']
          batches[i]['Unknown'] = float(batches[i]['Unknown'])/batches[i]['Total']
          #print batches[i]



     ## COMPUTE BATCH MEANS

     for i in range (0, n_batches):
          means['Total'] += batches[i]['Total']
          means['FailedNoBalance'] += batches[i]['FailedNoBalance']
          means['FailedOffline'] += batches[i]['FailedOffline']
          means['FailedNoPath'] += batches[i]['FailedNoPath']
          means['FailedTimeout'] += batches[i]['FailedTimeout']
          means['Unknown'] += batches[i]['Unknown']
          means['AvgAttempts'] += batches[i]['AvgAttempts']
          means['MinAttempts'] += batches[i]['MinAttempts']
          means['MaxAttempts'] += batches[i]['MaxAttempts']
          means['AvgTime'] += batches[i]['AvgTime']
          means['MinTime'] += batches[i]['MinTime']
          means['MaxTime'] += batches[i]['MaxTime']
          means['AvgRoute'] += batches[i]['AvgRoute']
          means['MinRoute'] += batches[i]['MinRoute']
          means['MaxRoute'] += batches[i]['MaxRoute']
          means['Succeeded'] += batches[i]['Succeeded']


     means['Total'] = float(means['Total'])/n_batches
     means['FailedNoBalance'] = float(means['FailedNoBalance'])/n_batches
     means['FailedOffline'] = float(means['FailedOffline'])/n_batches
     means['FailedNoPath'] = float(means['FailedNoPath'])/n_batches
     means['FailedTimeout'] = float(means['FailedTimeout'])/n_batches
     means['Unknown'] = float(means['Unknown'])/n_batches
     means['AvgAttempts'] = float(means['AvgAttempts'])/n_batches
     means['MinAttempts'] = float(means['MinAttempts'])/n_batches
     means['MaxAttempts'] = float(means['MaxAttempts'])/n_batches
     means['AvgTime'] = float(means['AvgTime'])/n_batches
     means['MinTime'] = float(means['MinTime'])/n_batches
     means['MaxTime'] = float(means['MaxTime'])/n_batches
     means['AvgRoute'] = float(means['AvgRoute'])/n_batches
     means['MinRoute'] = float(means['MinRoute'])/n_batches
     means['MaxRoute'] = float(means['MaxRoute'])/n_batches
     means['Succeeded'] = float(means['Succeeded'])/n_batches



     ## COMPUTE BATCH VARIANCES

     for i in range (0, n_batches):
          variances['Total'] += (batches[i]['Total'] - means['Total'])**2
          variances['FailedNoBalance'] += (batches[i]['FailedNoBalance']-means['FailedNoBalance'])**2
          variances['FailedOffline'] += (batches[i]['FailedOffline'] - means['FailedOffline'])**2
          variances['FailedNoPath'] += (batches[i]['FailedNoPath'] - means['FailedNoPath'])**2
          variances['FailedTimeout'] += (batches[i]['FailedTimeout'] - means['FailedTimeout'])**2
          variances['Unknown'] += (batches[i]['Unknown'] - means['Unknown'])**2
          variances['AvgAttempts'] += (batches[i]['AvgAttempts'] - means['AvgAttempts'])**2
          variances['MinAttempts'] += (batches[i]['MinAttempts'] - means['MinAttempts'])**2
          variances['MaxAttempts'] += (batches[i]['MaxAttempts'] - means['MaxAttempts'])**2
          variances['AvgTime'] += (batches[i]['AvgTime'] - means['AvgTime'])**2
          variances['MinTime'] += (batches[i]['MinTime'] - means['MinTime'])**2
          variances['MaxTime'] += (batches[i]['MaxTime'] - means['MaxTime'])**2
          variances['AvgRoute'] += (batches[i]['AvgRoute'] - means['AvgRoute'])**2
          variances['MinRoute'] += (batches[i]['MinRoute'] - means['MinRoute'])**2
          variances['MaxRoute'] += (batches[i]['MaxRoute'] - means['MaxRoute'])**2
          variances['Succeeded'] += (batches[i]['Succeeded'] - means['Succeeded'])**2

     variances['Total'] = float(variances['Total'])/(n_batches-1)
     variances['FailedNoBalance'] = float(variances['FailedNoBalance'])/(n_batches-1)
     variances['FailedOffline'] = float(variances['FailedOffline'])/(n_batches-1)
     variances['FailedNoPath'] = float(variances['FailedNoPath'])/(n_batches-1)
     variances['FailedTimeout'] = float(variances['FailedTimeout'])/(n_batches-1)
     variances['Unknown'] = float(variances['Unknown'])/(n_batches-1)
     variances['AvgAttempts'] = float(variances['AvgAttempts'])/(n_batches-1)
     variances['MinAttempts'] = float(variances['MinAttempts'])/(n_batches-1)
     variances['MaxAttempts'] = float(variances['MaxAttempts'])/(n_batches-1)
     variances['AvgTime'] = float(variances['AvgTime'])/(n_batches-1)
     variances['MinTime'] = float(variances['MinTime'])/(n_batches-1)
     variances['MaxTime'] = float(variances['MaxTime'])/(n_batches-1)
     variances['AvgRoute'] = float(variances['AvgRoute'])/(n_batches-1)
     variances['MinRoute'] = float(variances['MinRoute'])/(n_batches-1)
     variances['MaxRoute'] = float(variances['MaxRoute'])/(n_batches-1)
     variances['Succeeded'] = float(variances['Succeeded'])/(n_batches-1)


    ## COMPUTE BATCH CONFidENCE MIN

     confidence_min['Total'] = means['Total'] - percentile*sqrt( (variances['Total']**2)/n_batches ) 
     confidence_min['FailedNoBalance'] = means['FailedNoBalance'] - percentile*sqrt( (variances['FailedNoBalance']**2)/n_batches )
     confidence_min['FailedOffline'] = means['FailedOffline'] - percentile*sqrt( (variances['FailedOffline']**2)/n_batches )
     confidence_min['FailedNoPath'] = means['FailedNoPath'] - percentile*sqrt( (variances['FailedNoPath']**2)/n_batches )
     confidence_min['FailedTimeout'] = means['FailedTimeout'] - percentile*sqrt( (variances['FailedTimeout']**2)/n_batches )
     confidence_min['Unknown'] = means['Unknown'] - percentile*sqrt( (variances['Unknown']**2)/n_batches )
     confidence_min['AvgAttempts'] = means['AvgAttempts'] - percentile*sqrt( (variances['AvgAttempts']**2)/n_batches )
     confidence_min['MinAttempts'] = means['MinAttempts'] - percentile*sqrt( (variances['MinAttempts']**2)/n_batches )
     confidence_min['MaxAttempts'] = means['MaxAttempts'] - percentile*sqrt( (variances['MaxAttempts']**2)/n_batches )
     confidence_min['AvgTime'] = means['AvgTime'] - percentile*sqrt( (variances['AvgTime']**2)/n_batches )
     confidence_min['MinTime'] = means['MinTime'] - percentile*sqrt( (variances['MinTime']**2)/n_batches )
     confidence_min['MaxTime'] = means['MaxTime'] - percentile*sqrt( (variances['MaxTime']**2)/n_batches )
     confidence_min['AvgRoute'] = means['AvgRoute'] - percentile*sqrt( (variances['AvgRoute']**2)/n_batches )
     confidence_min['MinRoute'] = means['MinRoute'] - percentile*sqrt( (variances['MinRoute']**2)/n_batches )
     confidence_min['MaxRoute'] = means['MaxRoute'] - percentile*sqrt( (variances['MaxRoute']**2)/n_batches )
     confidence_min['Succeeded'] = means['Succeeded'] - percentile*sqrt( (variances['Succeeded']**2)/n_batches )


     ## COMPUTE BATCH CONFidENCE MAX

     confidence_max['Total'] = means['Total'] + percentile*sqrt( (variances['Total']**2)/n_batches )
     confidence_max['FailedNoBalance'] = means['FailedNoBalance'] + percentile*sqrt( (variances['FailedNoBalance']**2)/n_batches )
     confidence_max['FailedOffline'] = means['FailedOffline'] + percentile*sqrt( (variances['FailedOffline']**2)/n_batches )
     confidence_max['FailedNoPath'] = means['FailedNoPath'] + percentile*sqrt( (variances['FailedNoPath']**2)/n_batches )
     confidence_max['FailedTimeout'] = means['FailedTimeout'] + percentile*sqrt( (variances['FailedTimeout']**2)/n_batches )
     confidence_max['Unknown'] = means['Unknown'] + percentile*sqrt( (variances['Unknown']**2)/n_batches )
     confidence_max['AvgAttempts'] = means['AvgAttempts'] + percentile*sqrt( (variances['AvgAttempts']**2)/n_batches )
     confidence_max['MinAttempts'] = means['MinAttempts'] + percentile*sqrt( (variances['MinAttempts']**2)/n_batches )
     confidence_max['MaxAttempts'] = means['MaxAttempts'] + percentile*sqrt( (variances['MaxAttempts']**2)/n_batches )
     confidence_max['AvgTime'] = means['AvgTime'] + percentile*sqrt( (variances['AvgTime']**2)/n_batches )
     confidence_max['MinTime'] = means['MinTime'] + percentile*sqrt( (variances['MinTime']**2)/n_batches )
     confidence_max['MaxTime'] = means['MaxTime'] + percentile*sqrt( (variances['MaxTime']**2)/n_batches )
     confidence_max['AvgRoute'] = means['AvgRoute'] + percentile*sqrt( (variances['AvgRoute']**2)/n_batches )
     confidence_max['MinRoute'] = means['MinRoute'] + percentile*sqrt( (variances['MinRoute']**2)/n_batches )
     confidence_max['MaxRoute'] = means['MaxRoute'] + percentile*sqrt( (variances['MaxRoute']**2)/n_batches )
     confidence_max['Succeeded'] = means['Succeeded'] + percentile*sqrt( (variances['Succeeded']**2)/n_batches )


     ## WRITE STATS ON

     #output = {stat:{"Mean": means[stat], "Variance": variances[stat], "ConfidenceMin": confidence_min[stat], "ConfidenceMax": confidence_max[stat]} for stat in stats}

     dict_stats = {}

     for stat in stats:
          dict_stats[stat] = OrderedDict([
               ('Mean', means[stat]),
               ('Variance', variances[stat]),
               ('ConfidenceMin', confidence_min[stat]),
               ('ConfidenceMax', confidence_max[stat])
          ])

     temp_output = []
     for stat in stats:
          temp_output.append((stat, dict_stats[stat]))

     output = OrderedDict(temp_output)

     # output = OrderedDict([
     #      (stats[0], dict_stats[stats[0]]),
     #      (stats[1], dict_stats[stats[1]]),
     #      (stats[2], dict_stats[stats[2]]),
     #      (stats[3], dict_stats[stats[3]]),
     #      (stats[4], dict_stats[stats[4]]),
     #      (stats[5], dict_stats[stats[5]]),
     #      (stats[6], dict_stats[stats[6]]),
     #      (stats[7], dict_stats[stats[7]]),
     #      (stats[8], dict_stats[stats[8]]),
     #      (stats[9], dict_stats[stats[9]]),
     #      (stats[10], dict_stats[stats[10]]),
     #      (stats[11], dict_stats[stats[11]]),
     #      (stats[12], dict_stats[stats[12]]),
     #      (stats[13], dict_stats[stats[13]]),
     #      (stats[14], dict_stats[stats[14]]),

     # ])

     json.dump(output, stats_file, indent=4)
