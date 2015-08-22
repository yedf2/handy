import sys
import os

if len(sys.argv) < 4:
    print('''usage: %s <begin port> <port count> <process count>''' % sys.argv[0])
    raise SystemExit(1)

begin_port, port_count, process_count = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
port_per_process = port_count / process_count
for i in range(process_count):
    os.system('./1kw-svr %d %d&' % (begin_port + i * port_per_process, begin_port + (i+1) * port_per_process))
