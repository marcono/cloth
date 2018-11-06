import csv
from sortedcontainers import SortedDict

times = SortedDict()

path = '../../DGX1/simulations/'
sim_number = '58'
pay_file_name = '/payment_output.csv'
stats_file_name = '/times.csv'
pay_file_path = path   + sim_number + pay_file_name
stats_file_path = path + sim_number + stats_file_name


with open(pay_file_path, 'rb') as csv_pay, open(stats_file_path, 'wb') as csv_output :
     pay_reader = csv.reader(csv_pay)
     pay_lines = list(pay_reader)
     pay_iter = iter(pay_lines)
     next(pay_iter)

     writer = csv.writer(csv_output)


     for pay in pay_iter:
         if int(pay[6]) == 1:
             diff = int(pay[5]) - int(pay[4])
             if diff < 0:
                  continue
             times[int(pay[5])] = diff

     for key, value in times.items():
          writer.writerow([key, value])

     print sim_number

