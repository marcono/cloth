import csv

npeers_hub = 60
hub_id = 65

with open('../channelInfo.csv', 'rb') as csv_info, open('../channel.csv', 'rb') as csv_channel:
     info_reader = csv.reader(csv_info)
     info_lines = list(info_reader)
     iter_info = iter(info_lines)
     next(iter_info)
     #del(info_lines[0])
     channel_reader = csv.reader(csv_channel)
     channel_lines = list(channel_reader)
     iter_channel = iter(channel_lines)
     next(iter_channel)
     #del(channel_lines[0])

     for peer_id in range(0, npeers_hub):
          found = 0
          iter_info = iter(info_lines)
          next(iter_info)
          iter_channel = iter(channel_lines)
          next(iter_channel)
          for info_line in iter_info:
               if int(info_line[3]) == peer_id and int(info_line[4]) != hub_id:
                    found = 1
                    channel = int(info_line[1])
                    old_counterparty = int(info_line[4])
                    info_line[4] = hub_id
                    break
               if int(info_line[4]) == peer_id and int(info_line[3]) != hub_id:
                    found = 1
                    channel = int(info_line[2])
                    old_counterparty = int(info_line[3])
                    info_line[3] = hub_id
                    break
          if found == 1:
               for channel_line in iter_channel:
                    if int(channel_line[0]) == channel and int(channel_line[3]) == old_counterparty:
                         channel_line[3] = hub_id
                         break


with open('../channelInfo.csv', 'wb') as csv_info, open('../channel.csv', 'wb') as csv_channel:
     info_writer = csv.writer(csv_info)
     info_writer.writerows(info_lines)
     channel_writer = csv.writer(csv_channel)
     channel_writer.writerows(channel_lines)

# test: the two counts should be equal!
with open('../channelInfo.csv', 'rb') as csv_info, open('../channel.csv', 'rb') as csv_channel:
     info_reader = csv.reader(csv_info)
     info_lines = list(info_reader)
     iter_info = iter(info_lines)
     next(iter_info)
     #del(info_lines[0])
     channel_reader = csv.reader(csv_channel)
     channel_lines = list(channel_reader)
     iter_channel = iter(channel_lines)
     next(iter_channel)
     #del(channel_lines[0])


     count = 0
     for info_line in iter_info:
          if int(info_line[3]) == hub_id  or int(info_line[4]) == hub_id:
               count += 1
     print "channelInfo.csv", count


     count = 0
     for channel_line in iter_channel:
          if int(channel_line[3]) == hub_id:
               count += 1
     print "channel.csv ", count




# #test
# #csv_info.close()
# #csv_channel.close()
# channel_reader = csv.reader(csv_channel)
# channel_lines = list(channel_reader)
# iter_channel = iter(channel_lines)
# next(iter_channel)



# #csv_channel.close()

