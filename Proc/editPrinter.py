#!/usr/bin/python3.4

import sys
import os
import struct
import subprocess


def print_and_change_mem(pid):
    test_string = "This is my initial string "
    # Open up maps file to know which addresses are used
    # Then use those to read in the mem file
    with open("/proc/%d/maps" % pid, 'r') as maps_file:
        with open("/proc/%d/mem" % pid, 'rb') as mem_file:
            for line in maps_file.read().split('\n'):
                try:
                    ss = line.split(' ')[0]
                    begin, end = ss.split('-')
                    begin = int(begin, 16)
                    end = int(end, 16)
                    while begin < end - 16:
                        begin += 1
                        mem_file.seek(begin, 0)
                        stuff = str(mem_file.read(7))
                        if "This is" in stuff:
                            print(hex(begin))
                            break
                    mem_write = os.open("/proc/%d/mem" % pid, os.O_WRONLY)
                    os.lseek(mem_write, begin, os.SEEK_SET)
                    os.write(mem_write, "This is a tifferent string".encode())
                    mem_file.seek(begin)
                    chunk = mem_file.read(len(test_string))
                    print(chunk)
                except:
                    continue


def get_pid():
    ps = str(subprocess.Popen(['ps', 'aux'], stdout=subprocess.PIPE).communicate()[0].decode())
    processes = ps.split('\n')
    test_string = "./printer"
    for row in processes:
        row = row.split('  ')
        try:
            if row[-1].endswith(test_string):
                row = list(filter(None, row))
                return int(row[1])
        except IndexError:
            continue


if __name__ == '__main__':
    pid = get_pid()
    print("The process' id is:", pid)
    print_and_change_mem(pid)
