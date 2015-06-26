#!/usr/bin/python
import sys
import inserthandlers

class InsertHandler(object):
    def __init__(self, args):
        pass
    def process_line(self, line):
        pass
    @classmethod
    def is_insert_handler(self):
        return True

def usage():
    print '''usage: <sampler_stat> <insert-handler> [args]

Example:
sampler.out inserthandlers.BootProgress events.txt
'''

def main():
    if len(sys.argv) < 3:
        print "ERROR: missing mandatory arguments"
        usage()
        return

    part = sys.argv[2].rpartition('.')
    mod_name = part[0]
    clazz_name = part[2]
    mod = __import__(mod_name)
    clazz = getattr(mod, clazz_name)
    if not clazz.is_insert_handler():
        print "ERROR: given inserthandler '%s' isn't of type InsertHandler"%sys.argv[2]
    mh = clazz(sys.argv[3:])
    with open(sys.argv[1]) as infile:
        mh.line_count = 0
        for line in infile:
            nl = mh.process_line(line.strip())
            print nl
            mh.line_count += 1

if __name__ == "__main__":
    main()
