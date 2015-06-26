#! /bin/bash


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

PERIOD=${1-"100"}
HOST=${2-"build1"}
adb shell "sampler -s /etc/sampler/thermal_kalix.ini -p $PERIOD" | \
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
			TSTART=$1;first=0
			fflush()
		}
		NR>2{
			printf("%0.6f;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\n",
				$1 - TSTART,
				scale($3,1000),
				scale($4,1000),
				scale($5,1000),
				scale($6,1000),
				scale($7,1000),
				scale($8,1000),
				scale($9,1000),
				scale($10,1000),
				scale($11,1000),
				scale($12,1000),
				scale($13,1000000),
				scale($14,20),
				scale($15,1000000),
				scale($16,20));
			fflush()
		}' | tee /tmp/sampler.log | \
		feedgnuplot \
			--discontinuity 1234.5678 \
			--stream 0.1 \
			--xlen=20 \
			--domain \
			--line \
			--y2 0,1,2,3,10,12  \
			--ylabel="Celsius" \
			--y2label="MHz" \
			--y2min 0 \
			--y2max 2050 \
			--ymin 0 \
			--ymax 110.0 \
			--legend 0 "CPU-freq 0" \
			--legend 1 "CPU-freq 1" \
			--legend 2 "CPU-freq 2" \
			--legend 3 "CPU-freq 3" \
			--legend 4 "Temp tz0" \
			--legend 5 "Temp tz1" \
			--legend 6 "Temp tz2" \
			--legend 7 "Temp tz3" \
			--legend 8 "Temp tz4" \
			--legend 9 "Temp tz5" \
			--legend 10 "Ext-mem freq" \
			--legend 11 "Ext-mem mV (div 20)" \
			--legend 12 "GPU freq" \
			--legend 13 "GPU mV (div 20)"

