import csv

hub_id = 613

with open('../channelInfo.csv', 'rb') as csv_info, open('../peer.csv', 'rb') as csv_peer :
     info_reader = csv.reader(csv_info)
     info_lines = list(info_reader)
     info_iter = iter(info_lines)
     next(info_iter)

     peer_reader = csv.reader(csv_peer)
     peer_lines = list(peer_reader)
     peer_iter = iter(peer_lines)
     next(peer_iter)

     dic_peer_chann = {}

     for peer in peer_iter:
         info_iter = iter(info_lines)
         next(info_iter)
         nchannels=0
         for info in info_iter:
             if info[3] == peer[0] or info[4] == peer[0]:
                 nchannels+=1
         dic_peer_chann[peer[0]] = nchannels


     nchannels = 0
     nhubs=0
     for k,v in dic_peer_chann.iteritems():
         nchannels += int(v)
         if v >= 100:
              nhubs+=1

     print nhubs

#      print float(count)/(len(peer_lines)-1)

     info_iter = iter(info_lines)
     next(info_iter)

     infos = []

     count=0
     for info_line in info_iter:
         if int(info_line[3]) == hub_id  or int(info_line[4]) == hub_id:
             infos.append(info_line[1])
             infos.append(info_line[2])
             count += 1

     print count


with open('../payment_output.csv', 'rb') as csv_payment:
     pay_reader = csv.reader(csv_payment)
     pay_lines = list(pay_reader)
     pay_iter = iter(pay_lines)
     next(pay_iter)

     count = 0
     for pay_line in pay_iter:
        if pay_line[8] == '-1':
            continue
        route = pay_line[8].split('-')
        for hop in route:
            if hop in infos:
                count += 1
                break

     print count

