#!/bin/bash

########################################################################
#
# Parameters and command line parsing
#

selected_testcase=""
stdlog=/dev/null
testdefaultdir=`mktemp -d`
trap "{ rm -rf $testdefaultdir; exit 255; }" INT TERM EXIT
testdir="$testdefaultdir"
srcdir=$(dirname "$0")

while getopts "hvt:d:" opt; do
  case $opt in
    t)
      selected_testcase="$OPTARG"
      ;;
    v)
      stdlog=/dev/stdout
      ;;
    d)
      testdir="$OPTARG"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    h)
      echo "Usage: ./test.sh [-v] [-d TESTDIR] [-t TESTNAME]"
      echo "Options:"
      echo "    -v                verbose (useful for debugging)"
      echo "    -d TESTDIR        specifies test directory (default is temp directory)"
      echo "    -t TESTNAME       specifies testcase"
      echo "The script runs testcase(s) and reports success/failure via exit code 0/1."
      echo "If test is not specified then run all testcases."
      exit 1
      ;;
  esac
done

########################################################################
#
# Test framework implementation
#

testcase_name=
testcase_name_prev=
testcase_result=
testcases_result=0

function testcase_end() {
    testcase_result=${testcase_result:-$?}
    if [ "$testcase_name" != "" ]; then
        if [ "$stdlog" != "/dev/null" ]; then
            echo -n "TESTCASE END:   $testcase_name -> "
        fi
        if [ "$testcase_result" == "0" ]; then
            echo "success"
        else
            echo "FAILED"
	    testcases_result=1
        fi
    fi
    testcase_name_prev="$testcase_name"
    testcase_name=
    testcase_result=
}

function testcase_begin() {
    testcase_end
    if [ "$selected_testcase" == "" -o "$selected_testcase" == "$1" ]; then
	testcase_name="$1"
        if [ "$stdlog" == "/dev/null" ]; then
            echo -n "TESTCASE $testcase_name..."
        else
            if [ "$testcase_name_prev" != "" ]; then
                echo ""
            fi
            echo "TESTCASE BEGIN: $testcase_name"
        fi

    else
	false
    fi
}

function log() {
    echo "$@"  > $stdlog
}

function fail() {
    testcase_result=1
}

########################################################################
#
# Testcases
#

if testcase_begin "dummy"; then
    log This is a dummy testcase which always succeeds
    log
    log It has access to test directory \"$testdir\"
    log It has also access to source directory \"$srcdir\"
    log
    log A testcase can use two ways to report failure:
    log - by executing \"fail\" command anywhere in a testcase logic;
    log - by letting the last command to report the result via exit code.
fi

if testcase_begin "touch3-csv"; then
    log This testcase tests \"csv\" signal from \"touch3.csv\" file.
    log The \"csv\" signal contains touch breakdown data. The output
    log contains many empty lines and it can be futher refined by
    log piping it via \""| sed '1d;/;$/d;s,^.*;,,'"\".
    log
    log Input data are merged data from three asynchronous sources
    log - sampler, recording CPU frequencies, dev_input_time, deamon_input_time;
    log - tvist data;
    log - application data.
    log
    log Input data are merged without creating a \"Time_now_daemon\"
    log column.
    $srcdir/sampler-plot \
	-c $srcdir/touch3.spc \
	--rd-sampler=$srcdir/test_data/140610_tap_movies_with_clock-s5_touch3_time_now.smpl \
	--absolute --hide-all --show=csv \
	--csv=$testdir/140610_tap_movies_with_clock-s5_touch3_time_now.csv
    diff -u \
	$srcdir/test_data/140610_tap_movies_with_clock-s5_touch3_time_now.csv \
	$testdir/140610_tap_movies_with_clock-s5_touch3_time_now.csv \
	> $stdlog
fi

if testcase_begin "touch3-time-daemon-time-now-csv"; then
    log This testcase tests \"csv\" signal from \"touch3.csv\" file.
    log The \"csv\" signal contains touch breakdown data. The output
    log contains many empty lines and it can be futher refined by
    log piping it via \""| sed '1d;/;$/d;s,^.*;,,'"\".
    log
    log Input data are merged data from three asynchronous sources
    log - sampler, recording CPU frequencies, dev_input_time, deamon_input_time;
    log - tvist data;
    log - application data.
    log
    log Input data are merged with additionally creating a \"Time_now_daemon\"
    log column as copy of \"Time_now_sampler\".
    $srcdir/sampler-plot \
	-c $srcdir/touch3.spc \
	--rd-sampler=$srcdir/test_data/140610_tap_movies_with_clock-s5_touch3_time_daemon_time_now.smpl \
	--absolute --hide-all --show=csv \
	--time-daemon \
	--csv=$testdir/140610_tap_movies_with_clock-s5_touch3_time_now.csv
    diff -u \
	$srcdir/test_data/140610_tap_movies_with_clock-s5_touch3_time_now.csv \
	$testdir/140610_tap_movies_with_clock-s5_touch3_time_now.csv \
	> $stdlog
fi

if testcase_begin "startup-csv"; then
    log This testcase tests \"csv\" out for a recording of startup.spc
    $srcdir/sampler-plot \
	-c $srcdir/startup.spc \
	--rd-sampler=$srcdir/test_data/togari_user_14.1.B.1.322_no_changes_startup_sampler_events.smpl \
	--boot-progress \
	--signal iBat_internal expectable False \
	--signal vBat_internal expectable False \
	--signal iBat_internal_SI expr 'lambda x: 0' \
	--signal vBat_internal_SI expr 'lambda x: 0' \
	--csv=$testdir/togari_user_14.1.B.1.322_no_changes_startup_sampler_events.csv
    diff -u \
	$srcdir/test_data/togari_user_14.1.B.1.322_no_changes_startup_sampler_events.csv \
	$testdir/togari_user_14.1.B.1.322_no_changes_startup_sampler_events.csv \
	> $stdlog
fi

testcase_end
exit "$testcases_result"

#
# End of test.sh
#
########################################################################
