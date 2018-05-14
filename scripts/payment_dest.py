import csv
from random import randint

with open('../payment.csv', 'rb') as  csv_payment:
     pay_reader = csv.reader(csv_payment)
     pay_lines = list(pay_reader)
     pay_iter = iter(pay_lines)
     next(pay_iter)

     n_payments = len(pay_lines)-1
     n_same_dir = int(n_payments*0.6)

     for i in range(0, n_same_dir):
         pay_id = randint(0, n_payments)
         for pay_line in pay_iter:
             if int(pay_line[0]) == pay_id:
                 if int(pay_line[1]) != 500:
                     pay_line[2]=500
                 break
         pay_iter = iter(pay_lines)
         next(pay_iter)

with open('../payment.csv', 'wb') as csv_payment:
     pay_writer = csv.writer(csv_payment)
     pay_writer.writerows(pay_lines)


with open('../payment.csv', 'rb') as  csv_payment:
     pay_reader = csv.reader(csv_payment)
     pay_lines = list(pay_reader)
     pay_iter = iter(pay_lines)
     next(pay_iter)

     count = 0
     for pay_line in pay_reader:
         if pay_line[2] == '500':
             count += 1

     print count


