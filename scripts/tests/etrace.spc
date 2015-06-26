# -*- python -*-

#
# The following example shows how .spraw file can be adapted to work
# with sampler and it is used to show first 1000 lines from the file:
#
# $ cat $FILE_SPRAW | head --lines=1000 | (echo 'Time-now;TSince-last;cpu_id;col4;col5;state_prev;state;tid_prev;tid;freq'; cat) | sampler-plot -c etrace.spc --rd-sampler=-
#

import optparse
import sampler_plot

class ETrace(sampler_plot.SpcModule):
    def __init__(self):
        self.cores = 8
        self.radius = 100
    def on_init(self):
        self.mark_signal_as_template(None, ['CPU_template'])
    def on_parser(self, parser):
        group = optparse.OptionGroup(parser, "SPC specific: etrace.spc")
        group.add_option("--cores",
                         type="int",
                         default=8,
                         dest="cores",
                         help="Set number of CPU cores available on the measured device. Default is 8.")
        group.add_option("--ctx-min",
                         type="float",
                         default=1,
                         dest="ctx_min",
                         metavar="TIME",
                         help="Set minimum context switch TIME in mks (default is 1).")
        group.add_option("--ctx-max",
                         type="float",
                         default=1000,
                         dest="ctx_max",
                         metavar="TIME",
                         help="Set maximum context switch TIME in mks (default is 1000).")
        group.add_option("--ctx-ref",
                         type="float",
                         nargs=3, # core, freq_kHz, mks
                         action="append",
                         default=[(0, 1000000, 30),],
                         dest="ctx_ref",
                         metavar="CORE FREQ TIME",
                         help="Set reference context switch TIME (in mks) at given FREQ (in kHz) for all cores with id starting from CORE, can be specified more than once (default is --ctx-ref 0 1000000 30)")
        group.add_option("--radius",
                         type="int",
                         default=100,
                         dest="radius",
                         metavar="EVENTS",
                         help="Set radius in number of events (default is 100)")
        parser.add_option_group(group)
    def on_options(self, options):
        # --cores
        if options.cores:
            self.cores = options.cores
        _, idx, cpu_template = self.find_signal(None, ['CPU_template'])
        cpus = [self.clone_signal(cpu_template, {'cpuid': str(i)}) for i in range(self.cores)]
        self.get_children()[idx:idx] = cpus
        list_of_cores_as_args = [(i,) for i in range(self.cores)]
        self.signal_set_expr_list('cores', "cpu_%d[0]", list_of_cores_as_args)
        self.ctx_min = options.ctx_min
        self.ctx_max = options.ctx_max
        # make array of reference frequences and time for every core
        self.ctx_ref_freq = [[f for core, f, t in options.ctx_ref if core <= n][-1] for n in range(self.cores)]
        self.ctx_ref_time = [[t for core, f, t in options.ctx_ref if core <= n][-1] for n in range(self.cores)]
        self.radius = options.radius
    def signal_set_expr(self, _id, value):
        _, _, signal = self.find_signal(None, [_id])
        signal['expr'] = "lambda x: %s" % (value,)
    def signal_set_expr_list(self, _id, template, list_of_args):
        self.signal_set_expr(_id, "[%s]" % (", ".join([template % args for args in list_of_args])))
    #
    # This function estimates context switch time (in seconds)
    # given cpu id and frequency.
    #
    def ctx_switch_time(self, cpuid, freq_kHz):
        mks = self.ctx_ref_freq[cpuid] * self.ctx_ref_time[cpuid] / freq_kHz if freq_kHz > 0 else 0
        return max(min(mks, self.ctx_max), self.ctx_min) / 1000000.0

etrace_module = ETrace()
sampler_plot.register_module(etrace_module)

{'extra_feedgnuplot': '--ylabel="CPU" --y2label=" " --ymin=-1 --ymax=10 --y2min=-10000 --y2max=110000 --extracmds "set format x \\"%.6f\\"" --extracmds "set mouse format \\"%.6f\\"" --extracmds "set xtics font \\"arial,8\\"" --extracmds "set style fill solid 1.0 border -1"',
 'period': 10000,
 'signals': [{'exportable': False,
              'id': 'cpu_id',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'exportable': False,
              'id': 'col4',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'exportable': False,
              'id': 'col5',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'exportable': False,
              'id': 'state_prev',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'exportable': False,
              'id': 'state',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'exportable': False,
              'id': 'tid_prev',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'exportable': False,
              'id': 'tid',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'exportable': False,
              'id': 'freq',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: etrace_module.ctx_switch_time(int(cpu_id[0]), freq[0])',
              'id': 'ctx_time',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'expectable': False,
              'exportable': False,
              'id': 'CPU_template',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: state[0] if cpu_id[0] == %(cpuid)s else (cpu_%(cpuid)s[-1] if t[0] > 0 else 0)',
              'id': 'cpu_%(cpuid)s',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'subsignal': 1,
              'visible': False},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: freq[0] if cpu_id[0] == %(cpuid)s else (cpu_%(cpuid)s_freq[-1] if t[0] > 0 else -1)',
              'id': 'cpu_%(cpuid)s_freq',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'subsignal': 1,
              'visible': False},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: 1',
              'id': 'cpu_points_%(cpuid)s',
              'name': ' ',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: (None, None) if cpu_id[0] != %(cpuid)s else (cpu_%(cpuid)s[0] / 2 + %(cpuid)s, ctx_time[0])',
              'style': 'using ($1):($2):($3) with circles lc rgb "#8080ff"',
              'subsignal': 1},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: 1',
              'id': 'cpu_steps_%(cpuid)s',
              'name': ' ',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: interpolate if cpu_id[0] != %(cpuid)s else cpu_%(cpuid)s[0] / 2 + %(cpuid)s',
              'style': 'with steps lc rgb "#0000ff"',
              'subsignal': 1},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: 1',
              'id': 'cores',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: sum(cores[0])',
              'id': 'sum_cores',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'visible': False},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: sum_cores[0]',
              'id': 'sum_cores_points',
              'name': ' ',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: (None, None) if cpu_id[0] == None else (sum_cores_points[0], ctx_time[0])',
              'style': 'using ($1):($2):($3) with circles lc rgb "#ff8080"'},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: sum_cores[0]',
              'id': 'sum_cores_steps',
              'name': 'sum',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: interpolate if cpu_id[0] == None else sum_cores_steps[0]',
              'style': 'with steps lc rgb "#ff0000"'},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: sum_cores[0] > 0',
              'id': 'any_cores_points',
              'name': ' ',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: (None, None) if cpu_id[0] == None else ((etrace_module.cores + 0.5 * any_cores_points[0]), ctx_time[0])',
              'style': 'using ($1):($2):($3) with circles lc rgb "#ff8080"'},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: any_cores_points[0]',
              'id': 'any_cores_steps',
              'name': 'any',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: interpolate if cpu_id[0] == None else (etrace_module.cores + 0.5 * any_cores_steps[0])',
              'style': 'with steps lc rgb "#ff0000"'},
             {'expectable': False,
              'exportable': False,
              'expr': 'lambda x: sum(state[-etrace_module.radius:etrace_module.radius]) / (t[etrace_module.radius] - t[-etrace_module.radius])',
              'id': 'ctx_per_sec',
              'name': 'ctx/sec, Right',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'style': 'with lines linewidth 3 linecolor rgb "black"',
              'visible': True,
              'y': 2}],
 'sigs_file': '/system/etc/sampler/etrace.ini',
 'version': 2}
