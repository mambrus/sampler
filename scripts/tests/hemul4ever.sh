#! /bin/bash
for (( ;1; )); do
	hemul -i /tmp/kmsg.txt \
		-R '^<[0-9]>\[[[:space:]]*([[:digit:]]+\.[[:digit:]]{6})\].*' \
		-E -o /tmp/kmsg_out.txt
done
