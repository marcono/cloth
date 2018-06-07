import json
import csv
from random import randint

with open('../networkgraphv2-25-05-18.json', 'rb') as input, open('../channelLN.csv', 'wb') as csv_channel, open('../channelInfoLN.csv', 'wb') as csv_info, open('../peerLN.csv', 'wb') as csv_peer:
    data = json.load(input)
    peer_writer = csv.writer(csv_peer)

    peer_writer.writerow(['ID', 'WithholdsR'])

    edges = list(data["edges"])
    connected_nodes = []

    for edge in edges:
         connected_nodes.append(edge["node1_pub"])
         connected_nodes.append(edge["node2_pub"])

    nodes = list(data["nodes"])

    map_nodes = {}
    peer_id=0
    for node in nodes:
        if node["pub_key"] in connected_nodes:
             map_nodes[node["pub_key"]] = peer_id
             peer_writer.writerow([peer_id, 0])
             peer_id+=1

    info_writer = csv.writer(csv_info)
    info_writer.writerow(['ID', 'Direction1', 'Direction2', 'Peer1', 'Peer2', 'Capacity', 'Latency'])

    channel_writer = csv.writer(csv_channel)
    channel_writer.writerow(['ID', 'ChannelInfo', 'OtherDirection', 'Counterparty', 'Balance', 'FeeBase', 'FeeProportional', 'Timelock'])


    info_id = 0
    channel_id = 0

    for edge in edges:
        peer1 = map_nodes[edge["node1_pub"]]
        peer2 = map_nodes[edge["node2_pub"]]
        capacity = int(edge["capacity"])
        latency = randint(10,100)

        info_writer.writerow([info_id, channel_id, channel_id+1, peer1, peer2, capacity, latency])

        if edge["node1_policy"] is None:
            timelock = 14
            fee_base = 1000
            fee_prop = 10
        else:
            timelock = edge["node1_policy"]["time_lock_delta"]
            fee_base = edge["node1_policy"]["fee_base_msat"]
            fee_prop = edge["node1_policy"]["fee_rate_milli_msat"]

        channel_writer.writerow([channel_id, info_id, channel_id+1, peer2, capacity/2, fee_base, fee_prop, timelock])

        if edge["node2_policy"] is None:
            timelock = 14
            fee_base = 1000
            fee_prop = 10
        else:
            timelock = edge["node2_policy"]["time_lock_delta"]
            fee_base = edge["node2_policy"]["fee_base_msat"]
            fee_prop = edge["node2_policy"]["fee_rate_milli_msat"]

        channel_writer.writerow([channel_id+1, info_id, channel_id, peer1, capacity/2, fee_base, fee_prop, timelock])

        info_id+=1
        channel_id+=2

