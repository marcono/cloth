import csv
from random import randint


with open('../channelLN.csv', 'rb') as csv_channel, open('../channelInfoLN.csv', 'rb') as csv_info:
     info_reader = csv.reader(csv_info)
     info_lines = list(info_reader)
     iter_info = iter(info_lines)
     next(iter_info)
     channel_reader = csv.reader(csv_channel)
     channel_lines = list(channel_reader)
     iter_channel = iter(channel_lines)
     next(iter_channel)

     for info in iter_info:
        info[5] = int(3e11)

     for channel in iter_channel:
        channel[4] = int(1.5e11)

with open('../channelLN.csv', 'wb') as csv_channel, open('../channelInfoLN.csv', 'wb') as csv_info:
    info_writer = csv.writer(csv_info)
    info_writer.writerows(info_lines)
    channel_writer = csv.writer(csv_channel)
    channel_writer.writerows(channel_lines)


