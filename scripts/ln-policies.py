import json
import csv
from random import randint
import sys
import numpy as np
from collections import OrderedDict

input_args = list(sys.argv)

with open(input_args[1], 'rb') as input, open('ln-stats.json', 'wb') as stats_file:
    data = json.load(input)

    edges = list(data["edges"])

    policies = ["min_htlc", "time_lock_delta", "fee_base_msat", "fee_rate_milli_msat"]

    # min_HTLC=[]
    # timelock=[]
    # fee_base=[]
    # fee_prop=[]
    # n_policies=[]
    n_fee_zero=0

    policy_values = {policy:[] for policy in policies}

    for edge in edges:
        if edge["node1_policy"] is not None:
            for policy in policies:
                policy_values[policy].append(long(edge["node1_policy"][policy]))
            if long(edge["node1_policy"]["fee_base_msat"])==0 and long(edge["node1_policy"]["fee_rate_milli_msat"])==0:
                n_fee_zero += 1
        if edge["node2_policy"] is not None:
            for policy in policies:
                policy_values[policy].append(long(edge["node2_policy"][policy]))
            if long(edge["node2_policy"]["fee_base_msat"])==0 and long(edge["node2_policy"]["fee_rate_milli_msat"])==0:
                n_fee_zero += 1

    capacities = []
    for edge in edges:
        capacities.append(long(edge["capacity"]))

    output={}

    output["NumberOfChannels"] = len(edges)
    output["NumberOfPolicies"] = len(policy_values["min_htlc"])
    output["NumberZeroFees"] = n_fee_zero

    output["ChannelCapacity(msat)"] = OrderedDict([
        ('Mean', np.mean(capacities)),
        ('Variance',np.var(capacities))
    ])

    output["MinHTLC"] = OrderedDict([
        ('Mean', np.mean(policy_values["min_htlc"])),
        ('Variance',np.var(policy_values["min_htlc"]))
        ])

    output["FeeBase(msat)"] = OrderedDict([
        ('Mean', np.mean(policy_values["fee_base_msat"])),
        ('Variance',np.var(policy_values["fee_base_msat"]))
        ])

    output["FeeProportional(msat)"] = OrderedDict([
        ('Mean', np.mean(policy_values["fee_rate_milli_msat"])),
        ('Variance',np.var(policy_values["fee_rate_milli_msat"]))
        ])

    output["Timelock"] = OrderedDict([
        ('Mean', np.mean(policy_values["time_lock_delta"])),
        ('Variance',np.var(policy_values["time_lock_delta"]))
        ])

    json.dump(output, stats_file, indent=4)



