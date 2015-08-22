import sys
import os
import time

if len(sys.argv) < 7:
    print('''usage: %s <host> <begin port> <end port> <conn count> <process count> <heartbeat interval>''' % sys.argv[0])
    raise SystemExit(1)

host, begin_port, end_port, conn_count, process_count, heartbeat =     sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6])
for i in range(process_count):
    os.system('./1kw-cli %s %d %d %d %d&' % (host, begin_port, end_port, conn_count/process_count, heartbeat))
    time.sleep(0.5)

