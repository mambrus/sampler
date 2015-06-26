#!/usr/bin/python

import ast
import itertools
import operator
import optparse
import os
import pprint
import re
import sys
import traceback

csv_split_re = re.compile(r"""
(?:^|;)   # item starts at beginning of line or with ';' but ignores ';'
(         # this group contains the item value in one of three possible syntax:
    '     # 1. single quoted strin
     (?:[^'\\]*(?:\\.[^'\\]*)*)
    '
 |
    "     # 2 double quoted string
     (?:[^"\\]*(?:\\.[^"\\]*)*)
    "
 |
    [^;]* # 3. not-quoted string
)
""", re.DOTALL | re.VERBOSE)

def csv_eval_value(s):
    if len(s) == 0:
        return ''
    elif s[0] in "\"'":
        return ast.literal_eval(s)
    else:
        return s

def csv_split(line):
    return [csv_eval_value(s) for s in csv_split_re.findall(line)]

csv_split_simple_re = re.compile(r'\s*;\s*')

# this version of csv_split does not handle quoted strings
def csv_split_simple(line):
    return csv_split_simple_re.split(line)

def test_csv_split():
    #split = csv_split_simple
    split = csv_split
    tests = [r'"o\"n;e";"two"', r'"one";', r'"one";;',
             r"'o\'n;e';'two'", r"'one';", r"'one';;",
             r'one;two', r'one;;two', r'one;', r'one;;']
    # "o\"n;'e";"two"
    for test in tests:
        print "test %r -> %r" % (test, split(test))
    while True:
        # do not use iterator over "file" as it uses buffering
        test = sys.stdin.readline()
        if test == "":
            break
        print "test %r -> %r" % (test, split(test.rstrip("\n\r")))

class Env:
    def __init__(self):
        self.debug = 0
    def log(self, loglevel, tag, s):
        if self.debug >= loglevel:
            print >>sys.stderr, "[%s]: %s" % (tag, s)

class TimeConverter:
    def __init__(self, a, b):
        self.a = float(a)
        self.b = float(b)
    def convert(self, t):
        return self.a * t + self.b
    def __repr__(self):
        return "TimeConverter(%r,%r)" % (self.a, self.b)

def makeConverter(p0, p1):
    x0, y0 = map(float, p0)
    x1, y1 = map(float, p1)
    #
    # Given two points p0=(x0,y0) and p1=(x1,y1) find "a" and "b" of
    # the formula y=ax+b which iterpolates between p0 and p1
    #
    #   (y - y0) / (x - x0) = (y1 - y0) / (x1 - x0)
    # =
    #   (y - y0) * (x1 - x0) = (y1 - y0) * (x - x0)
    # =
    #   y * (x1 - x0) - y0 * (x1 - x0) = x * (y1 - y0) - x0 * (y1 - y0)
    # =
    #   y * (x1 - x0) = x * (y1 - y0) + y0 * (x1 - x0) - x0 * (y1 - y0)
    # =
    #   y  = x * (y1 - y0) / (x1 - x0) + y0 * (x1 - x0) / (x1 - x0) - x0 * (y1 - y0) / (x1 - x0)
    # =
    #   y  = x * [(y1 - y0) / (x1 - x0)] + [y0 - x0 * (y1 - y0) / (x1 - x0)]
    #
    a = (y1 - y0) / (x1 - x0)
    b = y0 - x0 * (y1 - y0) / (x1 - x0)
    return TimeConverter(a, b)

class Source:
    def __init__(self, env, name, filename,
                 clock_column=None, extra_times=[], renames={}, copies=[], deletes=[], delete_all=False, keeps=[],
                 converter=TimeConverter(1.0,0.0)):
        self.env = env
        self.name = name
        self.filename = filename
        self.renames = renames
        self.copies = copies
        self.deletes = deletes
        self.keeps = keeps
        self.delete_all = delete_all
        self.extra_times = extra_times
        self.clock_column = clock_column
        self.clock_column_src_idx = 0
        self.clock_column_dst_idx = 0
        self.clock_use_custom_converter = False
        self.converter = converter
        self.converters = None
        self.time, self.line = None, None
        self.linenumber = 0
        self.nosigs = None
        self.fd = file(filename, "r")
        self.columns = None
        self.src_columns_count = 0
        self.advance()
    def get_nosigs(self):
        return ";".join(self.nosigs)
    def get_time(self):
        return self.time
    def get_line(self):
        return self.line
    def get_line_and_advance(self):
        result = self.line
        self.advance()
        return result
    def advance(self):
        self.line = None
        while self.fd and self.line == None:
            self.advance_line()
    def advance_line(self):
        self.time, self.line = None, None
        if self.fd != None:
            failed = True
            try:
                line = self.fd.readline()
                if line != "":
                    self.linenumber = self.linenumber + 1
                    line = line.rstrip("\n\r")
                    self.env.log(3, "SOURCE[%s]" % self.name, "advance() input line [%r]" % (line))
                    items = csv_split_simple(line)
                    if self.linenumber <= 1:
                        self.src_columns_count = len(items)
                        columns = [self.renames.get(n, n) for n in items]
                        # fill self.keeps with implicitly kept columns
                        if self.delete_all:
                            self.keeps.extend([self.clock_column or columns[0]])
                            self.keeps.extend([n for n, e in self.extra_times])
                            self.keeps.extend(self.renames.values())
                            self.keeps.extend(reduce(operator.add, map(list, self.copies), []))
                        # append names from "self.copies"
                        columns = reduce(operator.add, [[(i, e)]+[(i, e2) for e1, e2 in self.copies if e1 == e] for i, e in enumerate(columns)], [])
                        # handle "self.deletes" and "self.keeps"
                        columns = [(src_i, n) for src_i, n in columns
                                   if (self.delete_all and n in self.keeps)
                                   or (not self.delete_all and n not in self.deletes)]
                        # build column data
                        self.columns = [(src_i, dst_i, n) for dst_i, (src_i, n) in enumerate(columns)]
                        self.nosigs = ["NO_SIG"] * len(self.columns) # @todo should use signals "nosig" field
                        # find self.clock_column_idx
                        if self.clock_column:
                            self.clock_column_src_idx, self.clock_column_dst_idx = [(src_i, dst_i) for src_i, dst_i, n in self.columns if n == self.clock_column][0]
                            self.clock_use_custom_converter = self.clock_column in [n for n, e in self.extra_times]
                        column_dst_indices = dict([(n, dst_i) for src_i, dst_i, n in self.columns])
                        self.converters = dict([(i, (lambda x: x)) for i in xrange(len(columns))] +
                                               [(column_dst_indices[n], self.make_column_converter(10**ast.literal_eval(e))) for n, e in self.extra_times])
                        self.line = ";".join([n for src_i, dst_i, n in self.columns]) # @todo support quoted items
                    elif self.src_columns_count == len(items):
                        time = items[self.clock_column_src_idx]
                        if time and time != self.nosigs[self.clock_column_dst_idx]:
                            items = [self.converters[dst_i](items[src_i]) for src_i, dst_i, n in self.columns]
                            if self.clock_use_custom_converter:
                                self.time = self.converters[self.clock_column_dst_idx](time, format_seconds=True)
                            else:
                                self.time = self.converter.convert(ast.literal_eval(time))
                            items[self.clock_column_dst_idx] = str(self.time)
                            items[self.clock_column_dst_idx] = "%r" % self.time # @todo remove
                            self.line = ";".join(items) # @todo support quoted items
                    else:
                        print >>sys.stderr, "WARNING %s:%d: malformed line, found %d columns, expected %d columns, skipping" % (self.name, self.linenumber, len(items), self.src_columns_count)
                    failed = False
            except:
                self.env.log(1, "SOURCE[%s]" % self.name, "advance() exception, %s" % traceback.format_exc())
                traceback.print_stack()
                raise
            if failed:
                self.env.log(3, "SOURCE[%s]" % self.name, "advance() failed, closing")
                self.close()
                self.fd = None
        self.env.log(3, "SOURCE[%s]" % self.name, "advance() -> %r [%r]" % (self.time, self.line))
    def make_column_converter(self, factor):
        factor = float(factor)
        def f(x, format_seconds=False):
            if len(x) > 0 and x[0] in "0123456789-+.":
                x = ast.literal_eval(x)
                x = x / factor
                y = self.converter.convert(x)
                if not format_seconds:
                    y = y * factor
                    y = "%r" % y
            else:
                y = x
            return y
        return f
    def close(self):
        if self.fd != None:
            self.fd.close()
        self.fd, self.time, self.line = None, None, None

class Merger:
    def __init__(self, env):
        self.env = env
        self.sources = []
        self.linenumber = 0
    def add(self, source):
        self.sources.append(source)
    def get_time(self):
        return self.time
    def get_line(self):
        return self.line
    def get_line_and_advance(self):
        result = self.line
        self.advance()
        return result
    def advance(self):
        self.time, self.line = None, None
        self.env.log(3, "MERGER", "advance()")
        failed = True
        if self.linenumber < 1:
            headers = [s.get_line_and_advance() for s in self.sources]
            if all(headers):
                self.line = ";".join(headers)
                self.env.log(3, "MERGER", "header: [%s]" % self.line)
                failed = False
        else:
            times = [(t, i) for i, t in enumerate([s.get_time() for s in self.sources]) if t != None]
            times = [s.get_time() for s in self.sources]
            proper_times = [t for t in times if t != None]
            if len(proper_times) > 0:
                self.time = min(proper_times)
                self.env.log(3, "MERGER", "time: %.02f" % self.time)
                lines = [s.get_line_and_advance() if t != None and t <= self.time else s.get_nosigs() for s, t in itertools.izip(self.sources, times)]
                self.line = ";".join(lines)
                failed = False
        if failed:
            self.close()
        else:
            self.linenumber = self.linenumber + 1
    def close(self):
        [s.close() for s in self.sources]
        self.sources = []

def test_merger():
    env = Env()
    #env.debug = 10
    merger = Merger(env)
    merger.add(Source(env, "1", "simulate1.out",
                      converter=makeConverter((60,160), (1060,1160))))
    merger.add(Source(env, "2", "simulate2.out",
                      renames = {"Time-now":    "Time2_now",
                                 "TSince-last": "TSince2_last",
                                 "Time_sync":   "Time2_sync",
                                 "P1":          "P2"},
                      converter=makeConverter((200,160.05), (1200,1160.05))))
    merger.advance()
    while True:
        line = merger.get_line_and_advance()
        if not line:
            break
        print "%s" % (line,)

def parser_sampler_data_source_ensure_outside(parser, options):
    if options.pending_sampler_data_source:
        parser.error("invalid arguments: unbalanced -[ -]")
    if (options.pending_sampler_data_source
        or options.file
        or options.name
        or options.renames
        or options.copies
        or options.deletes
        or options.keeps
        or options.delete_all
        or options.extra_times
        or options.clock_column
        or options.interpolate
        or options.linear
        ):
        parser.error("invalid arguments: data source-specific options used outside -[ -]")

def parser_sampler_data_source_begin(option, opt_str, value, parser):
    options = parser.values
    parser_sampler_data_source_ensure_outside(parser, options)
    options.pending_sampler_data_source = True

def parser_sampler_data_source_end(option, opt_str, value, parser):
    options = parser.values
    # validate
    if not options.pending_sampler_data_source:
        parser.error("invalid arguments: unbalanced -[ -]")
    if options.interpolate and options.linear:
        parser.error("invalid arguments: cannot use both --linear and --interpolate")
    # default data
    sampler_data_source = {}
    sampler_data_source['converter'] = TimeConverter(1.0, 0.0)
    # --file
    if options.file:
        sampler_data_source['file'] = options.file
        options.file = None
    else:
        parser.error("invalid arguments: --file FILE option missing between -[ and -]")
    # --name
    if options.name:
        sampler_data_source['name'] = options.name
    else:
        sampler_data_source['name'] = "%d" % len(options.sampler_data_sources)
    # --rename
    sampler_data_source['renames'] = dict(options.renames)
    # --copy
    sampler_data_source['copies'] = options.copies
    # --delete
    sampler_data_source['deletes'] = options.deletes
    # --keep
    sampler_data_source['keeps'] = options.keeps
    # --delete-all
    sampler_data_source['delete_all'] = options.delete_all
    # --times
    sampler_data_source['extra_times'] = options.extra_times
    # --clock
    sampler_data_source['clock_column'] = options.clock_column
    # --interpolate
    if options.interpolate:
        x0, y0, x1, y1 = options.interpolate
        sampler_data_source['converter'] = makeConverter((x0,y0), (x1,y1))
    # --linear
    if options.linear:
        a, b = options.linear
        sampler_data_source['converter'] = TimeConverter(a, b)
    # append new sampler data source
    options.sampler_data_sources.append(sampler_data_source)
    # reset all
    options.file = None
    options.name = None
    options.renames = []
    options.copies = []
    options.deletes = []
    options.keeps = []
    options.delete_all = False
    options.extra_times = []
    options.clock_column = None
    options.interpolate = None
    options.linear = None
    options.pending_sampler_data_source = False
    pass

def main(argv):
    usage = "usage: %prog OPTIONS"
    parser = optparse.OptionParser(usage)
    parser.formatter._long_opt_fmt="%s %s"
    parser.add_option("-d", "--debug",
                      type="int",
                      dest="debug",
                      default=0,
                      metavar="LEVEL",
                      help="Set debug level between 0 (default, no debug output) and 3 (several lines per sample, very verbose)")
    parser.add_option("-f", "--file",
                      type="string",
                      dest="file",
                      metavar="FILE",
                      help="Read sampler data from FILE")
    parser.add_option("-[",
                      action="callback",
                      nargs=0,
                      default=False,
                      dest="pending_sampler_data_source",
                      callback=parser_sampler_data_source_begin,
                      help="Begins description of a sampler data source")
    parser.add_option("-]",
                      action="callback",
                      nargs=0,
                      default=[],
                      dest="sampler_data_sources",
                      callback=parser_sampler_data_source_end,
                      help="Ends description of a sampler data source")
    group = optparse.OptionGroup(parser, "Sampler Data Source (must be used between -[ and -])")
    group.add_option("-n", "--name",
                     type="string",
                     dest="name",
                     metavar="NAME",
                     help="Assigns NAME to the sampler data source. Useful for debugging")
    group.add_option("-C", "--clock",
                     type="string",
                     default=None,
                     dest="clock_column",
                     metavar="NAME",
                     help="Use column NAME as primary clock. Default is to use the first column")
    group.add_option("-t", "--time",
                     type="string",
                     nargs=2, # NAME FACTOR
                     action="append",
                     default=[],
                     dest="extra_times",
                     metavar="NAME EXPONENT",
                     help="Treat also column NAME (after renaming) as time scaled using EXPONENT (for example EXPONENT 3 means time in milliseconds)")
    group.add_option("-r", "--rename",
                     type="string",
                     nargs=2, # N1 N2
                     action="append",
                     default=[],
                     dest="renames",
                     metavar="N1 N2",
                     help="Renames column from N1 to N2")
    group.add_option("-c", "--copy",
                     type="string",
                     nargs=2, # N1 N2
                     action="append",
                     default=[],
                     dest="copies",
                     metavar="N1 N2",
                     help="Create new signal N2 as copy of N1")
    group.add_option("-D", "--delete",
                     type="string",
                     action="append",
                     default=[],
                     dest="deletes",
                     metavar="NAME",
                     help="Deletes signal NAME")
    group.add_option("--delete-all",
                     action="store_true",
                     default=False,
                     dest="delete_all",
                     help="Switches logic from deleting individual signals using \"--delete\" to keeping indivual signals using \"--keep\". All signals marked by \"--copy\", \"--rename\", \"--time\" and \"--clock\" are implicitly marked as kept")
    group.add_option("-k", "--keep",
                     type="string",
                     action="append",
                     default=[],
                     dest="keeps",
                     metavar="NAME",
                     help="Keeps signal NAME (should be used together with \"--delete-all\")")
    group.add_option("--interpolate",
                     type="float",
                     nargs=4, # X0 Y0 X1 Y1
                     dest="interpolate",
                     metavar="X0 Y0 X1 Y1",
                     help="Linearly interpolate time given two points: (X0,Y0) and (X1,Y1)")
    group.add_option("--linear",
                     type="float",
                     nargs=2, # A B
                     dest="linear",
                     metavar="A B",
                     help="Recalculate time using formula Y = A * X + B")
    parser.add_option_group(group)
    options, args = parser.parse_args()
    parser_sampler_data_source_ensure_outside(parser, options)
    if len(args) > 0:
        parser.error("invalid arguments3")
    #pprint.PrettyPrinter().pprint(options.sampler_data_sources)
    env = Env()
    env.debug = options.debug
    merger = Merger(env)
    for sds in options.sampler_data_sources:
        merger.add(Source(env, sds['name'], sds['file'],
                          clock_column=sds['clock_column'],
                          extra_times=sds['extra_times'],
                          renames=sds['renames'],
                          copies=sds['copies'],
                          deletes=sds['deletes'],
                          delete_all=sds['delete_all'],
                          keeps=sds['keeps'],
                          converter=sds['converter']))
    merger.advance()
    while True:
        line = merger.get_line_and_advance()
        if not line:
            break
        print "%s" % (line,)

if __name__ == "__main__" and os.getenv("INSIDE_EMACS") == None:
        main(sys.argv[1:])

#test_csv_split()
#test_merger()
