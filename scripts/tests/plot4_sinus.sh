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
./sampler testrc/thermal_1.ini $PERIOD | \
	awk -F";" '
		BEGIN{
			first=1
		}
		NR==1{
			t0=$1;first=0
			fflush()
		}
		NR>1{
			printf("%s;%s;%s:%s;%s;%s\n",
				$1-t0,$2,$3,$4,$5,$6)
			fflush()
		}' | tee /tmp/sampler.log | \
		feedgnuplot \
			--geometry "60x35+0+0" \
			--stream \
			--xlen=100 \
			--domain \
			--line \
			--y2 0 \
			--y2label="Sample accuracy" \
			--ylabel="Thread values" \
			--ymin -1 \
			--ymax 1.35 \
			--y2min 0.01 \
			--y2max 0.011434 \
			--legend 0 "Period time" \
			--legend 1 "ThreadVal 1" \
			--legend 2 "ThreadVal 2" \
			--legend 3 "ThreadVal 3" \
			--legend 4 "ThreadVal 4"

