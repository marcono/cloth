import csv

with open('../results/sigma_0/payment_output.csv', 'rb') as  csv_payment:
     pay_reader = csv.reader(csv_payment)
     pay_lines = list(pay_reader)
     pay_iter = iter(pay_lines)
     next(pay_iter)

     attempts = len(pay_lines) -1
     for pay_line in pay_iter:
         attempts = attempts + int(pay_line[7])

     print attempts

     print attempts/(len(pay_lines)-1)
