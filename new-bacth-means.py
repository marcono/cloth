import json
import csv
import sys
from scipy.stats import t
from math import sqrt
from collections import OrderedDict
import pandas as pd
import operator
import numpy as np
import matplotlib.pyplot as plt

with open('payments_output.csv', 'r') as csv_pay:#, open('xk.csv', 'w') as output_csv:#, open(stats_file_path, 'wb') as stats_file :

     ## FIND TRANSIENT
#     panda_reader = pd.read_csv(csv_pay)
 #    end_time = panda_reader['time'].max()
  #   print(end_time)


     # reader = csv.reader(csv_pay)
     # next(reader)
     # payments = sorted(reader, key = lambda row: int(row[5]), reverse=False)

     payments = list(csv.DictReader(csv_pay))

     n = len(payments)

#     print(payments[n-1][5])

     last_payment_time = int(payments[n-1]['time'])

     remainder = last_payment_time%31

     end_time = last_payment_time - remainder

     batch_length = end_time/31

     print(batch_length)

     batches=[0 for i in range(0, 30)]
     times = [0 for i in range(0, 30)]
     attempts = [0 for i in range(0, 30)]
     n_pays = [0 for i in range(0, 30)]
     no_balance = [0 for i in range(0, 30)]
     per_pay_time = []

     for pay in payments:
          pay_end_time = int(pay['time'])
          if pay_end_time<batch_length or pay_end_time >= end_time:
               continue
          b = int(pay_end_time/batch_length) -1
          batches[b]+=1
          if(pay['route']!='-1'):
               per_pay_time.append(int(pay['end_time']) - int(pay['time']))
               times[b] += (int(pay['end_time']) - int(pay['time']))
               attempts[b] += int(pay['attempts'])
               n_pays[b] += 1
          if pay['is_timeout'] != '1' and pay['route'] != '-1' and pay['uncoop_after'] != '1' and pay['uncoop_before'] != '1' and pay['is_success'] != '1':
               no_balance[b] += 1;


     print('BATCHES')
     print(batches)
     print(np.mean(batches))
     print(np.std(batches))
     print(batch_length)

     for i in range(0, 30):
          times[i] = float(times[i])/n_pays[i]
          attempts[i] = float(attempts[i])/n_pays[i]

     print('TIMES')
     print(times)
     print(np.mean(times))
     print(np.var(times))

     print(n_pays)

     plt.plot(times)
     plt.show()

     # print('ATTEMPTS')
     # print(attempts)
     # print(np.mean(attempts))
     # print(np.var(attempts))

     # print('TIMES PER PAYMENT')
     # print(np.mean(per_pay_time))
     # print(np.var(per_pay_time))

     # print('PROB NO BALANCE')
     # print(no_balance)
     # print(np.mean(no_balance))
     # print(np.var(no_balance))

     # x=[]
     # y=[]

     # # i=0
     # # for item in sortedlist:
     # #      x.append(i)
     # #      diff = int(item[5]) - int(item[4])
     # #      y.append(diff)
     # #      i+=1

     # for k in range(0,200):
     #      mean=0
     #      for i in range(k+1, n):
     #           mean += (int(sortedlist[i][5]) - int(sortedlist[i][4]))
     #      mean = float(mean)/(n-k)
     #      x.append(k)
     #      y.append(mean)
     #      #writer.writerow([k, mean])
     #      #x_k.append(mean)

     # kneedle = KneeLocator(x, y, curve='concave', direction='increasing')
     # print(kneedle.elbow)
