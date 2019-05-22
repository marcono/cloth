import json
import numpy as np
from collections import Counter

with open('../ln-mainnet-16-36-12-02-2019.json', 'rb') as input, open('channels-per-node.json', 'wb') as output:
    graph = json.load(input)

    edges = list(graph["edges"])

    pub_keys = []
    for edge in graph["edges"]:
        pub_keys.append(edge["node1_pub"])
        pub_keys.append(edge["node2_pub"])


    map_nodes = {}
    node_id = 0
    for pk in set(pub_keys):
        map_nodes[pk] = node_id
        node_id += 1

    nodes = []
    for pk in pub_keys:
        nodes.append(map_nodes[pk])

    channels_per_node = Counter(pub_keys).most_common()
    hub = channels_per_node[0][0]

    tot_capacity = 0
    tot_node_hubs = 0
    for edge in edges:
        if edge["node1_pub"] == hub or edge["node2_pub"] == hub:
            tot_capacity += long(edge["capacity"])
            tot_node_hubs += 1

    print tot_capacity/tot_node_hubs


    # hub_nodes = []
    # for edge in graph["edges"]:
    #     if edge["node1_pub"] == hub:
    #         hub_nodes.append(edge["node2_pub"])
    #     elif edge["node2_pub"] == hub:
    #         hub_nodes.append(edge["node1_pub"])

    # print len(hub_nodes)
    # print len(set(hub_nodes))

    # node_channels = {}
    # for node in set(hub_nodes):
    #     n_channels = 0
    #     for edge in edges:
    #         if node == edge["node1_pub"] and edge["node2_pub"] != hub:
    #             n_channels += 1
    #         if node == edge["node2_pub"] and edge["node1_pub"] != hub:
    #             n_channels += 1
    #     node_channels[node] = n_channels

    # print node_channels


    print len(channels_per_node)

    json.dump(channels_per_node.most_common(), output, indent=2)


    tot_channels = sum(channels_per_node.values())
    print tot_channels
    percentile = tot_channels*10/100;

    n_channels = 0
    hubs = []
    for node,channels in channels_per_node.most_common():
        n_channels += channels
        if n_channels > percentile:
            break
        hubs.append(node)

    print len(hubs)

    distance1 = set()
    distance2 = set()
    for hub in hubs:
        for edge in edges:
            if edge["node1_pub"] == hub:
                counterparty1 = edge["node2_pub"]
            elif edge["node2_pub"] == hub:
                counterparty1 = edge["node1_pub"]
            else:
                continue

            if counterparty1 not in hubs:
                distance1.add(counterparty1)

            for edge in edges:
                if edge["node1_pub"] == counterparty1:
                    counterparty2 = edge["node2_pub"]
                elif edge["node2_pub"] == counterparty1:
                    counterparty2 = edge["node1_pub"]
                else:
                    continue

                if counterparty2 not in hubs:
                    distance2.add(counterparty2)


    print len(distance1)
    print len(distance2)
    distance2 = distance2 - distance1
    print len(distance2)



