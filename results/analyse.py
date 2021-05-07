#!env python
import csv
import numpy as np

class Event:
    accepted = False
    # Time the event was first accepted
    acceptTime = 0

    def __init__(self, x, y, val, initTime):
        self.x = x
        self.y = y
        self.val = val
        self.initTime = initTime

    def acceptEvent(self, time):
        assert(time >= self.initTime)
        if not self.accepted:
            self.acceptTime = time
            self.accepted = True

    def __str__(self):
        decisionString = "ACCEPTED" if self.accepted else "NOT ACCEPTED"
        return f'{decisionString} (x:{self.x}, y:{self.y}):{self.val}'


# Will hold the results from each run
g_total = []
g_accepted = []
g_rejected = []

g_wrongAccepted = []
g_rightRejected = []
g_totalWrong    = []
g_avgLatency    = []

filenames = [f'results{i}.csv' for i in range(10)]

# Populate the global variables
for name in filenames :
    # maps a node id to a list of events
    nodes = {}
    csvfile = open(name)

    reader = csv.reader(csvfile)
    # skip the header
    next(csvfile)

    for row in reader:
        time   = int(row[0])
        nodeid = int(row[1])
        x      = int(row[2])
        y      = int(row[3])
        val    = int(row[4])
        typ    = int(row[5])

        # Means the event has just arrived
        if typ == 0:
            if nodeid not in nodes:
                nodes[nodeid] = [Event(x, y, val, time)]

            # Only insert something if there is nothing there
            # otherwise ignore the arriving event
            else:
                find = [i for i in nodes[nodeid] if
                        i.x == x and i.y == y and i.val == val]
                if not find:
                    nodes[nodeid].append(Event(x, y, val, time))

        elif typ == 1:
            # Find the event that has not been accepted
            find = [i for i in nodes[nodeid] if
                    i.x == x and i.y == y and i.val == val and not i.accepted]

            if find:
                find[0].acceptEvent(time)

    total         = 0
    accepted      = 0
    rejected      = 0

    rejectedRight = 0   # false positive
    acceptedWrong = 0   # false negatives

    latency = []

    for nodeid in nodes:
        for event in nodes[nodeid]:
            total += 1
            # accepted / rejected count
            if event.accepted:
                latency.append(event.acceptTime - event.initTime)
                accepted += 1
            else:
                rejected += 1

            # false positive
            if event.accepted and event.val == 909090:
                acceptedWrong += 1

            # false negative
            if not event.accepted and event.val != 909090:
                rejectedRight += 1


    totalWrong = acceptedWrong + rejectedRight

    g_total.append(total)
    g_accepted.append(accepted)
    g_rejected.append(rejected)
    g_rightRejected.append(rejectedRight)
    g_wrongAccepted.append(acceptedWrong)
    g_totalWrong.append(totalWrong)
    g_avgLatency.append(np.average(latency))


wrongPercentage = [i/j for i,j in zip (g_totalWrong, g_total)]
wrongAcceptedPercentage = [i/j for i,j in zip(g_wrongAccepted, g_totalWrong)]
rightRejectedPercentage = [i/j for i,j in zip(g_rightRejected, g_totalWrong)]

string = f'''
total messages = {np.average(g_total)}
total accepted = {np.average(g_accepted)}
total rejected = {np.average(g_rejected)}
average latency = {np.average(g_avgLatency) : .2f} ms
average correct = {100 - np.average(wrongPercentage) * 100: .2f}%
wrong messages accepted = {np.average(wrongAcceptedPercentage) * 100 : .2f}%
right messages rejected = {np.average(rightRejectedPercentage) * 100 : .2f}%
'''

print(string)