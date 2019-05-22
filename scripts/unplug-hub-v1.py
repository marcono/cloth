#unplug un hub and reconnect among them all nodes connected to the hub
import json
import numpy as np
import sys
from collections import Counter

np.random.seed(1992)

hub_position = 5

with open('../results/hub/ln-without-hub-4.json', 'rb') as input:
    graph = json.load(input)

    # FIND AN HUB
    pub_keys = []
    for edge in graph["edges"]:
        pub_keys.append(edge["node1_pub"])
        pub_keys.append(edge["node2_pub"])

    # map_nodes = {}
    # node_id = 0
    # for pk in set(pub_keys):
    #     map_nodes[pk] = node_id
    #     node_id += 1

    # nodes = []
    # for pk in pub_keys:
    #     nodes.append(map_nodes[pk])

    channels_per_node = Counter(pub_keys).most_common()
    hub = channels_per_node[hub_position][0]

    print 'hub pk: ' + hub

    ## COLLECT NODES CONNECTED TO HUB
    hub_nodes = []
    for edge in graph["edges"]:
        if edge["node1_pub"] == hub:
            hub_nodes.append(edge["node2_pub"])
        elif edge["node2_pub"] == hub:
            hub_nodes.append(edge["node1_pub"])

    ## CONNECT HUB NODES AMONG THEM
    for edge in graph["edges"]:
        if edge["node1_pub"] == hub:
            hub_nodes_excluded = [x for x in hub_nodes if x != edge["node2_pub"]]
            edge["node1_pub"] = np.random.choice(hub_nodes_excluded)
        elif edge["node2_pub"] == hub:
            hub_nodes_excluded = [x for x in hub_nodes if x != edge["node1_pub"]]
            edge["node2_pub"]= np.random.choice(hub_nodes_excluded)

    ## TEST
    for edge in graph["edges"]:
        if edge["node1_pub"] == edge["node2_pub"]:
            print 'ERROR: auto-channel'
            sys.exit(0)
        if  edge["node1_pub"] == hub or  edge["node2_pub"] == hub:
            print 'ERROR: hub still present'
            sys.exit(0)

with open('../results/hub/ln-without-hub-'+ str(hub_position) + '.json', 'wb') as output:
    json.dump(graph, output, indent=2)

    print 'file written without errors'

    # tot_channels = sum(channels_per_node.values())
    # print tot_channels
    # percentile = tot_channels*10/100;

    # n_channels = 0
    # hubs = []
    # for node,channels in channels_per_node.most_common():
    #     n_channels += channels
    #     if n_channels > percentile:
    #         break
    #     hubs.append(node)

    # print len(hubs)
