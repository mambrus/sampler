import itertools
import optparse
import os
import pprint
import re
import sys
from xml.sax.saxutils import escape

tabsize=8
verbose=False

doc_orig = "../../doc/1_1_signal_definition.txt"
doc_parsed = "sampler_doc_data.py"

#
# @todo handle '%' symbol
# @todo implement intent for "append_children"
#

class Parser:
    def __init__(self, id, format, regexps, subparsers=[], append_children=False):
        self.id = id
        self.regexps = [re.compile(r) for r in regexps]
        self.format = format
        self.subparsers = subparsers
        self.append_children = append_children
    def __repr__(self):
        return 'Parser(%r)' % [r.pattern for r in self.regexps]

class ParseResult:
    def __init__(self):
        self.pos = None
        self.limit = None
        self.header_size = None
        self.dict = {}
        self.indent = None
    def parse(self, parser, data, lines=(0,None)):
        line = lines[0]
        limit = ParseResult.expand_limit(data, lines)
        matches = []
        result = [(i, ms) for i, ms in [(i, [r.match(data[i + j][1]) for j, r in enumerate(parser.regexps)]) for i in xrange(line, limit - len(parser.regexps) + 1)] if all(ms)]
        if len(result) == 0:
            print >>sys.stderr, "error: found no match for parser %r in %r" % (parser, lines)
            sys.exit(1)
        if len(result) > 1:
            print >>sys.stderr, "error: found %d matches for parser %r in %r (at %r)" % (len(result), parser, lines, [i+1 for i, _ in result])
            sys.exit(1)
        self.pos = result[0][0]
        self.header_size = len(parser.regexps)
        self.dict = dict([(item[0], escape(item[1])) for sublist in [m.groupdict().items() for m in result[0][1]] for item in sublist])
        self.indent = ([data[self.pos+i][0] + m.start(key) for i, m in enumerate(result[0][1]) for key in m.groupdict().keys() if key == 'indent']+[None])[0]
    def set_limit(self, data, limit):
        self.limit = limit
        if self.indent != None:
            f = lambda pair: self.indent <= pair[0]
            in0 = self.pos + self.header_size
            self.limit = ([in0]+[i+1 for _, i in itertools.takewhile(f, [(indent, i) for i, (indent, _) in itertools.izip(xrange(in0,self.limit), data[in0:self.limit])])])[-1]
    def set_body(self, data, exclude_range):
        in0, in1 = self.pos + self.header_size, self.limit
        ex0, ex1 = exclude_range[0], exclude_range[1]
        self.dict['body'] = escape(ParseResult.format_lines([line for i, line in enumerate(data) if in0 <= i and i < in1 and not (ex0 <= i and i < ex1)]))
    @staticmethod
    def format_lines(data):
        data = list(data)
        while len(data) > 0 and data[-1][1].strip() == '':
            del data[-1]
        if len(data) == 0:
            return ""
        indent = min([i for i, _ in data])
        return "\n".join([(" " * (i - indent)) + s for i, s in data])
    @staticmethod
    def expand_limit(data, lines=(0,None)):
        return lines[1] if lines[1] != None else len(data)
    @staticmethod
    def parse_group(parsers, data, lines=(0,None), target_dict=None):
        d = dict([(p, ParseResult()) for p in parsers])
        for parser, result in d.items():
            result.parse(parser, data, lines=lines)
        positions = [(r.pos, r) for r in d.values()] + [(ParseResult.expand_limit(data, lines), None)]
        positions.sort()
        for (_, r1), (p2, _) in itertools.izip(positions, positions[1:]):
            r1.set_limit(data, p2)
            #print "result: %r-%r %r" % (r1.pos, r1.limit, r1.dict)
        for parser, result in d.items():
            dd = ParseResult.parse_group(parser.subparsers, data, lines=(result.pos, result.limit), target_dict=target_dict)
            exclude_ranges = [(r.pos, r.limit) for r in dd.values()]
            exclude_range = (0,0)
            if len(exclude_ranges) > 0:
                exclude_range = (min([i for i, _ in exclude_ranges]), max([i for _, i in exclude_ranges]))
            result.set_body(data, exclude_range)
            #result.dict['body'] = "<hidden>"
            value = parser.format % result.dict
            if parser.append_children and target_dict != None:
                value = '\n'.join([value]+[target_dict[p.id] for p in parser.subparsers if target_dict[p.id]])
            if target_dict != None:
                target_dict[parser.id] = value
            if verbose:
                print "=== %r-%r %s\n=== exclude_range: %r\n%s" % (result.pos, result.limit, parser.id, exclude_range, value)
        return d

parsers = [
    Parser('doc:name',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Signal name \(symbolic\))', r'^-*$']),
    Parser('doc:fname',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Signal name from file)', r'^-*$']),
    Parser('doc:fdata',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Data-file name)', r'^-*$']),
    Parser('doc:fops_mask',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Sampling operation)', r'^-*$'],
           append_children=True,
           subparsers =
           [Parser('doc:fops_mask_bits',
                   '',
                   [r'^[\d\.]+\s+(?P<title>Bits)', r'^-*$']),
            Parser('doc:fops_openclose',
                   '<b>%(bits)s: %(title)s:</b> %(line1)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>OPENCLOSE)\s*:\s*(?P<line1>.*)']),
            Parser('doc:fops_canblock',
                   '<b>%(bits)s: %(title)s:</b> %(line1)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>CANBLOCK)\s*:\s*(?P<line1>.*)']),
            Parser('doc:fops_trigger',
                   '<b>%(bits)s: %(title)s:</b> %(line1)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>TRIGGER)\s*:\s*(?P<line1>.*)']),
            Parser('doc:fops_timed',
                   '<b>%(bits)s: %(title)s:</b> %(line1)s\n%(body)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>TIMED)\s*:\s*(?P<line1>.*)']),
            Parser('doc:fops_no_rewind',
                   '<b>%(bits)s: %(title)s:</b> %(line1)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>NO_REWIND)\s*:\s*(?P<line1>.*)']),
            Parser('doc:fops_thread_option',
                   '<b>%(bits)s: %(title)s:</b> %(line1)s\n%(body)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>THREAD_OPTION)\s*:\s*(?P<line1>.*)']),
            Parser('doc:fops_thread_option_values',
                   '  Value is one of:',
                   [r'^$', r'^Values:$', r'^$'],
                   append_children=True,
                   subparsers=
                   [Parser('doc:thread_options_thread_all',
                           '  <b>%(bits)s: %(title)s:</b> %(line1)s',
                           [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>THREAD_ALL)\s*:\s*(?P<line1>.*)']),
                    Parser('doc:thread_options_no_sub_threads',
                           '  <b>%(bits)s: %(title)s:</b> %(line1)s',
                           [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>NO_SUB_THREADS)\s*:\s*(?P<line1>.*)']),
                    Parser('doc:thread_options_never_sub_thread',
                           '  <b>%(bits)s: %(title)s:</b> %(line1)s',
                           [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>NEVER_SUB_THREAD)\s*:\s*(?P<line1>.*)']),
                    Parser('doc:thread_options_tbd',
                           '  <b>%(bits)s: %(title)s:</b> %(line1)s',
                           [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>TBD)\s*:?\s*(?P<line1>.*)']),
                    Parser('doc:thread_options_single_threading',
                           '  <b>%(bits)s: %(title)s:</b> %(line1)s',
                           [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>SINGLE_THREADING)\s*:\s*(?P<line1>.*)']),
                     ]),
            Parser('doc:fops_priority',
                   '\n<b>%(bits)s: %(title)s:</b> %(line1)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>PRIORITY)\s*:\s*(?P<line1>.*)']),
            Parser('doc:fops_always',
                   '<b>%(bits)s: %(title)s:</b> %(line1)s',
                   [r'(?P<bits>\d+(?:-\d+)?)\s*:?\s*(?P<title>ALWAYS)\s*:\s*(?P<line1>.*)']),
            ]),
    Parser('doc:rgx_line',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Regex identifying line to parse.)', r'^-*$']),
    Parser('doc:rgx_line_index',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Line count match)', r'^-*$']),
    Parser('doc:rgx_sig',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Signal regex)', r'^-*$']),
    Parser('doc:rgx_sig_index',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>Sub-match index)', r'^-*$']),
    Parser('doc:nuce',
           '<b>%(title)s</b>\n\n%(body)s',
           [r'^[\d\.]+\s+(?P<title>On no update \(NUCE\))', r'^-*$'],
           [#[Parser('doc:nuce_nothing',
           #        '+++ [%(value)s]\n%(line1)s\n%(body)s',
           #        [r'(?P<value>\d+)\s*:?\s*(?P<line1>Nothing.*)']),
           # Parser('doc:nuce_last_value',
           #        '+++ [%(value)s]\n%(line1)s\n%(body)s',
           #        [r'(?P<value>\d+)\s*:?\s*(?P<line1>Last\s+value.*)']),
           # Parser('doc:nuce_dead_per_signal',
           #        '+++ [%(value)s]\n%(line1)s\n%(body)s',
           #        [r'(?P<value>\d+)\s*:?\s*(?P<line1>Per\s+signal\s+defined.*)']),
           # Parser('doc:nuce_dead_preset',
           #        '+++ [%(value)s]\n%(line1)s\n%(body)s',
           #        [r'(?P<value>\d+)\s*:?\s*(?P<indent>)(?P<line1>.*\bpre-set\b.*)']),
            ]),
    ]

def load_file(filename):
    prev_indent = 0
    data = []
    for line in file(filename):
        line = line.rstrip("\n\r").expandtabs(tabsize)
        line_len = len(line)
        line = line.lstrip(" ")
        indent = line_len - len(line)
        if not line:
            indent = prev_indent
        prev_indent = indent
        data.append((indent, line))
        #print "%d: %s" % (indent, line)
    return data

def parse_documentation(filename):
    data = load_file(filename)
    d = {}
    ParseResult.parse_group(parsers, data, target_dict=d)
    return d

def load_documentation(filename):
    try:
        result = eval(file(filename).read())
        return result
    except:
        print >>sys.stderr, "error: error parsing documentation file: %r" % (filename,)
        sys.exit(1)

def get_documentation():
    bin_file = sys.argv[0]
    while os.path.islink(bin_file):
        bin_file = os.path.join(os.path.dirname(bin_file), os.readlink(bin_file))
    bin_dir = os.path.dirname(bin_file)
    fullpath_doc_orig = os.path.join(bin_dir, doc_orig)
    fullpath_doc_parsed = os.path.join(bin_dir, doc_parsed)
    if os.path.exists(fullpath_doc_parsed):
        return load_documentation(fullpath_doc_parsed)
    elif os.path.exists(fullpath_doc_orig):
        return parse_documentation(fullpath_doc_orig)
    else:
        print >>sys.stderr, "warning: sampler documentation not found"
        return {}

def main(args):
    usage = "usage: %prog OPTIONS"
    parser = optparse.OptionParser(usage)
    parser.add_option("-i", "--input",
                      type="string",
                      dest="input",
                      metavar="FILE",
                      help="Input file for parsing documentation")
    parser.add_option("-o", "--output",
                      type="string",
                      dest="output",
                      metavar="FILE",
                      help="Output file to store parsed documentation data")
    parser.add_option("--verbose",
                      action="store_true",
                      default=False,
                      dest="verbose",
                      help="Print debugging information")
    options, args = parser.parse_args()
    global verbose
    verbose = options.verbose
    if len(args) == 0 and options.input and options.output:
        d = parse_documentation(options.input)
        with file(options.output, 'w') as stream:
            print >>stream, "# -*- python -*-"
            print >>stream, ""
            pp = pprint.PrettyPrinter(stream=stream)
            pp.pprint(d)
    else:
        parser.error("invalid arguments")

if __name__ == "__main__" and os.getenv("INSIDE_EMACS") == None:
        main(sys.argv[1:])
