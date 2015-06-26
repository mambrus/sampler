import sys
import re
from sampler_insert import InsertHandler

boot_progress_re = re.compile(r'.*boot_progress_(\w+):\s(\d+).*')
sampler_ts_re = re.compile(r'^(\d+)\.(\d+)')
def ts_from_line(line):
    '''time stamp in ms'''
    m = sampler_ts_re.match(line)
    if m:
        ts = int(m.group(1))*1000 + int(m.group(2))/1000
        return ts
    else:
        print "ERROR: unable to extract time stamp from line: '%s'"%line
        sys.exit(1)

class BootProgress(InsertHandler):
    def __init__(self, args):
        if len(args) != 1:
            print "ERROR: missing argument"
            print "usage: <event-log>"
            sys.exit(1)
        self.events = []
        self.line_cnt = 0
        with open(args[0]) as infile:
            for line in infile:
                m = boot_progress_re.match(line)
                if m:
                    self.events.append((m.group(1), int(m.group(2))))

        #print self.events
    def process_line(self, line):
        if self.line_cnt == 0:
            line += ";boot_progress"
        else:
            ts = ts_from_line(line)
            l = [x for x in self.events if ts >= x[1]]
            if len(l) == 0:
                line += ";0"
            else:
                line += ";%s"%l[-1][0]
        self.line_cnt += 1
        return line

# this handler is obsolete, keept only for reference
cpu_usage_re = re.compile(r'^\d+\.\d+\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+')
class CpuUsage(InsertHandler):
    def __init__(self, args):
        if len(args) != 1:
            print "ERROR: missing argument"
            print "usage: <cpu-usage-file>"
            sys.exit(1)
        self.events = []
        self.line_cnt = 0
        self.infile = open(args[0])
        self.next = self.next_usage()
        self.ncol = 0
        self.prev_vals = ";NO_SIG"*2

    def next_usage(self):
        line = self.infile.readline()
        #print "Line:", len(line), line,
        if len(line) == 0:
            return None
        d = {}
        d['time'] = ts_from_line(line)
        d['time_str'] = line[:line.find(' ')]
        m = cpu_usage_re.match(line)
        if not m:
            print "ERROR: not valid cpu usage line line: '%s'"%line
            sys.exit(1)
        d['tot_idle'] = int(m.group(4))
        d['tot_iow'] = int(m.group(5))
        return d

    def values(self, d):
        return ";%d;%d"%(d['tot_idle'], d['tot_iow'])

    def process_line(self, line):
        if self.line_count == 0:
            line += ";cpu_idle;cpu_iow"
            return line
        if self.ncol == 0:
            self.ncol = line.count(';')
        while self.next and self.next['time'] < ts_from_line(line):
            self.prev_vals = self.values(self.next)
            print self.next['time_str']+";NO_SIG"*self.ncol + self.prev_vals
            self.next = self.next_usage()

        line += self.prev_vals
        return line

ale_re = re.compile(r'.*ALE     : \(ale\)\s+(\S+):(\S+):(\S+):(\S+)')
class ALE(InsertHandler):
    def __init__(self, args):
        if len(args) != 1:
            print "ERROR: missing argument"
            print "usage: <log-file>"
            sys.exit(1)
        self.ale_events = []
        self.ale_prev_event = None
        self.prev_ts = None
        f = open(args[0])
        for line in f:
            m = ale_re.match(line)
            if m:
                self.ale_events.append( (m.group(1),int(m.group(2)),int(m.group(3)),int(m.group(4))) )
        f.close()

    def process_line(self, line):
        if self.line_count == 0:
            line += ";ale_testcase;ale_fps"
        else:
            ts = ts_from_line(line)
            if self.prev_ts:
                l = [e for e in self.ale_events if e[1] > self.prev_ts and e[1] <= ts]
                if len(l) == 0:
                    e = self.ale_prev_event
                else:
                    e = l[-1]
                if e:
                    append_str = ";%s;%d"%(e[0],e[-1])
                else:
                    # no previous ale events
                    append_str = ";0;0"
                self.ale_prev_event = e
            else:
                # first data line, no previous time stamp
                append_str = ";0;0"
            self.prev_ts = ts
            line += append_str
        return line

marker_re = re.compile(r'^Start:\s(\d+\.\d+)')
class StartMarker(InsertHandler):
    def __init__(self, args):
        if len(args) != 1:
            print "ERROR: missing argument"
            print "usage: <marker-file>"
            sys.exit(1)
        self.marker_events = []
        self.ale_prev_event = None
        self.prev_ts = None
        f = open(args[0])
        for line in f:
            m = marker_re.match(line)
            if m:
                marker_time = float(m.group(1))
                self.marker_events.append(marker_time*1000)
        f.close()

    def process_line(self, line):
        if self.line_count == 0:
            line += ";marker"
        else:
            ts = ts_from_line(line)
            if self.prev_ts:
                l = [e for e in self.marker_events if e > self.prev_ts and e <= ts]
                if len(l) == 0:
                    e = None
                else:
                    e = l[-1]
                if e:
                    append_str = ";100"
                else:
                    # 
                    append_str = ";0"
                self.ale_prev_event = e
            else:
                # first data line, no previous time stamp
                append_str = ";0"
            self.prev_ts = ts
            line += append_str
        return line
