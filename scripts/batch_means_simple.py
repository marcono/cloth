import csv

#path = '/home/marco/DGX1-remote-results/simulations/'
path = '../../simulations/'
sim_number = '20-LN'
pay_file_name = '/payment_output.csv'
stats_file_name = '/stats.csv'
pay_file_path = path   + sim_number + pay_file_name
stats_file_path = path + sim_number + stats_file_name

with open(pay_file_path, 'rb') as csv_pay, open(stats_file_path, 'wb') as csv_stats :
     pay_reader = csv.reader(csv_pay)
     pay_lines = list(pay_reader)
     pay_iter = iter(pay_lines)
     next(pay_iter)

     csvwriter = csv.writer(csv_stats);

     header = ['SucceededPayments', 'FailedPaymentsNoPath', 'FailedPaymentsUncoop', 'FailedPaymentsNoBalance', 'UnknownPayments', 'AvgTime', 'AvgAttempts']

     csvwriter.writerow(header);

     succeeded=0
     no_path=0
     uncoop=0
     no_balance=0
     unknown=0
     avg_time = 0
     avg_attempts = 0

     count = 0

     for pay in pay_iter:
         if int(pay[3]) >= 1e6 and int(pay[6]) == 1:
              count += 1
         if pay[10] != -1:
              avg_time += int(pay[5]) - int(pay[4])
              avg_attempts += int(pay[9])
         if pay[6]=='1':
             succeeded += 1
         else:
             if pay[10] == '-1':
                 no_path +=1
             elif pay[7] == '1':
                 unknown += 1
             elif pay[8] == '1':
                 uncoop += 1
             else:
                 no_balance += 1

     avg_time = avg_time/(len(pay_lines) - no_path)
     avg_attempts = avg_attempts/(len(pay_lines) - no_path)

     stats = [succeeded, no_path, uncoop, no_balance, unknown, avg_time, avg_attempts]
     csvwriter.writerow(stats)

     print sim_number




