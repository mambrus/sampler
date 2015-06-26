#!/system/bin/sh
INI_FILE=
PERIOD=
OUT_FILE=/data/local/tmp/out/sampler_`date +%Y%m%d_%H%M%S`
echo "Saving output to $OUT_FILE"
echo "$OUT_FILE" > /data/local/tmp/out/sampler_out_filename
PATH=$PATH:/data/local/tmp/
/data/local/tmp/sampler -n -T $PERIOD -s $INI_FILE > $OUT_FILE 2> /data/local/tmp/out/sampler_err
