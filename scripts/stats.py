import csv
with open('../channelInfoLN.csv', 'rb') as csv_channel:
     channel_reader = csv.reader(csv_channel)
     channel_lines = list(channel_reader)
     channel_iter = iter(channel_lines)
     next(channel_iter)

     channels = []

     count=0
     for channel_line in channel_iter:
         if channel_line[3] == '445' or channel_line[4] == '445':
             channels.append(channel_line[1])
             channels.append(channel_line[2])
             count += 1

     print count


with open('../payment_output.csv', 'rb') as csv_payment:
     pay_reader = csv.reader(csv_payment)
     pay_lines = list(pay_reader)
     pay_iter = iter(pay_lines)
     next(pay_iter)

     count = 0
     for pay_line in pay_iter:
        if pay_line[7] == '-1':
            continue
        route = pay_line[7].split('-')
        for hop in route:
            if hop in channels:
                count += 1
                break

     print count
