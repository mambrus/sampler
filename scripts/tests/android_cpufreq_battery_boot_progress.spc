# -*- python -*-

{'extra_feedgnuplot': '--ylabel="CPU freq" --y2label="Battery Charge" --ymin 0 --ymax 3000000 --y2min 0 --y2max 110 --curvestyleall "with lines"',
 'host': True,
 'period': 100000,
 'signals': [{'fdata': '/sys/devices/system/cpu/cpu1/cpufreq/cpuinfo_cur_freq',
              'fops_mask': 1,
              'id': 'CPU_frq_1',
              'nosig': 'NO_SIG',
              'nuce': 3},
             {'fdata': '/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq',
              'fops_mask': 1,
              'id': 'CPU_frq_0',
              'nosig': 'NO_SIG',
              'nuce': 3},
             {'fdata': '/tmp/logcat_e.txt',
              'fops_mask': 4,
              'id': 'battery_level',
              'nosig': 'NO_SIG',
              'nuce': 1,
              'rgx_line': ' I battery_level: ',
              'rgx_sig': '(^.*\\[)(.*),(.*),(.*)(\\].*)',
              'rgx_sig_index': 2,
              'style': 'with linespoints pt 6 ps 0.5',
              'y': 2},
             {'fdata': '/tmp/logcat_e.txt',
              'fops_mask': 4,
              'id': 'boot_progress',
              'nosig': 'NO_SIG',
              'nuce': 1,
              'rgx_line': ' I boot_progress_',
              'rgx_sig': '(^.* I boot_progress_)(.*)(:[[:space:]])(.*$)',
              'rgx_sig_index': 2,
              'style': 'with linespoints pt 6 ps 0.5',
              'vars': {'dict': {'ams_ready': 90,
                                'enable_screen': 100,
                                'pms_data_scan_start': 60,
                                'pms_ready': 80,
                                'pms_scan_end': 70,
                                'pms_start': 40,
                                'pms_system_scan_start': 50,
                                'preload_end': 20,
                                'preload_start': 10,
                                'start': 5,
                                'system_run': 30}},
              'y': 2}],
 'sigs_file': '/tmp/smpl_edit_test1.ini',
 'version': 2}
