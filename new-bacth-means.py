import json
import csv
import sys
from scipy.stats import t
from math import sqrt
from collections import OrderedDict
import pandas as pd
import operator
from kneed import KneeLocator

with open('payments_output.csv', 'r') as csv_pay:#, open('xk.csv', 'w') as output_csv:#, open(stats_file_path, 'wb') as stats_file :

     ## FIND TRANSIENT
#     panda_reader = pd.read_csv(csv_pay)
 #    end_time = panda_reader['time'].max()
  #   print(end_time)


     reader = csv.reader(csv_pay)
     next(reader)

     payments = sorted(reader, key = lambda row: int(row[5]), reverse=False)

     n = len(payments)

     print(payments[n-1][5])

     last_payment_time = int(payments[20000][5])

     remainder = last_payment_time%31

     end_time = last_payment_time - remainder

     batch_length = end_time/31

     print(batch_length)

     batches=[0 for i in range(0, 31)]

     for pay in payments:
          pay_end_time = int(pay[5])
          if pay_end_time<batch_length or pay_end_time >= end_time:
               continue
          b = int(pay_end_time/batch_length)
          batches[b]+=1

     print(batches)



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
