#! /bin/bash

function sleep_forever () {
	while true; do sleep 10000; done
}

if [ "X${SAMPLER_ENV_SET}" == "X" ]; then
	if [ -f .env ]; then
		source .env
	else
		echo "Environment not set and file .env not found."\
			"Are you in the right directory?" 1>&2
		exit 1
	fi
fi

echo "Staring shabang. log-file available for: tail -f /tmp/sampler.log"

LEGEND='--legend 0 "Period_time"'

PERIOD=${1-"10000"}
(adb shell sampler -s /system/etc/sampler/thermal_1.ini -p $PERIOD | \
	awk -F";" '
		function scale(v, factor) {
			if (v == "NO_SIG")
				return "1234.5678";
			return v / factor;
		}
		BEGIN{
			first=1
		}
		NR==2{
			t0=$1;first=0
			fflush()
		}
		NR>2{
			printf("%s;%s;%s;%s\n",
				$1-t0,scale($3,1000),scale($4,1000),scale($5,1000.0))
			fflush()
		}' | tee /tmp/sampler.log | \
		feedgnuplot \
			--geometry "60x35+0+0" \
			--stream 0.1 \
			--xlen=20 \
			--domain \
			--line \
			--y2 2 \
			--y2label="Celsius" \
			--ylabel="MHz" \
			--ymin 40 \
			--ymax 1900 \
			--y2min 0 \
			--y2max 110.0 \
			--legend 0 "CPU-freq 0" \
			--legend 1 "CPU-freq 1" \
			--legend 2 "Temp tz0" \
			--dump | \
		sed 's,^.* 1234\.5678$,,' ; \
		sleep_forever ) | \
		gnuplot /dev/stdin
