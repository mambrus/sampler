# -*- python -*-

{'extra_feedgnuplot': '--ylabel="MHz" --y2label="Celsius" --ymin 40 --ymax 1900 --y2min -100000.0 --y2max 400000.0 --curvestyleall "with lines"',
 'period': 10000,
 'signals': [{'fdata': '/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq',
              'fops_mask': 1,
              'id': 'CPU_frq_0',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: x / 1000',
              'style': 'with linespoints pt 6 ps 0.5'},
             {'fdata': '/sys/devices/system/cpu/cpu1/cpufreq/cpuinfo_cur_freq',
              'fops_mask': 1,
              'id': 'CPU_frq_1',
              'nosig': 'NO_SIG',
              'nuce': 3,
              'scale': 'lambda x: x / 1000'},
             {'fdata': '/sys/class/thermal/thermal_zone0/temp',
              'fname': '/sys/class/thermal/thermal_zone0/type',
              'fops_mask': 1,
              'id': 'T_ntc_ext_tz',
              'nosig': 'NO_SIG',
              'nuce': 1,
              'style': 'with linespoints pt 6 ps 0.5',
              'y': 2}],
 'sigs_file': '/data/local/tmp/thermal_1.ini',
 'version': 2,
 'y2_shift': 2,
 'y_shift': 7}
