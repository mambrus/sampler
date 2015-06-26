# -*- python -*-

{'extra_feedgnuplot': '--ylabel="MHz" --y2label="Celsius" --ymin 500 --ymax 2800 --y2min 0 --y2max 110.0 --curvestyleall "with lines"',
 'host': True,
 'period': 100,
 'signals': [{'id': 'CPU-freq 0',
              'nosig': 'NO_SIG',
              'scale': 'lambda x: x / 1000'},
             {'id': 'CPU-freq 1',
              'nosig': 'NO_SIG',
              'scale': 'lambda x: x / 1000'},
             {'id': 'Temp tz0',
              'nosig': 'NO_SIG',
              'scale': 'lambda x: x / 1000',
              'y': 2}],
 'sigs_file': 'testrc/thermal_1.ini',
 'version': 2,
 'y2_shift': 2,
 'y_shift': 14}
