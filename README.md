Chapter 0.0: SAMPLER
====================

Produces periodic samples gathered from files on target system. Typical
files would be files from sysfs, procfs, devfs but also normal files like
log-files.

Samples are either periodical, where period-time is defined in uS by -t flag
at start-up, or event-driven. Event driven samples are defined by a rule
describing the trigger (TBD).

A sample is defined as a series of signals. Each signal is defined by one
line in a file describing all signals. Output format for each sample is on
clear-text line where each signal is has it's private column representing
the same order it was mentioned in the descriptive file, prepended by 3
special signals: "sample id", "kernel timestamps (i.e. time-stamp since
start-up) and "sample time" (i.e. time it took to complete a sample). The
latter is needed to determine jitter due to system load or I/O-wait of any
data source.

Full format of the output (assuming delimiter is ";") would be:

{sample id};{kernel time};{sample time};{signal_0};..{signal_n}

Field details are described below.
Chapter 1.0: SAMPLE DEFINITION
==============================

One sample is an entity occurring at a specific moment in time. Quite
commonly periodical, or driven by some sort of event.

One sample has the following properties.

Field description:
------------------

1 Period time (uS)
------------------
  Time between two samples. 0 or NULL id event-driven

2 Event description (TBD)
-------------------------
  ***NOTE: This sub-section is not finished. Concept still needs research*** 
  
  Some form of description about the event. Could be on each signals update,
  or a specific signals update. Could be markers in log e.t.a.

3 List of signals
-----------------
  A sample has at least one signal but no upper limit (only practicalities
  determine the upper limit).

Chapter 1.1: FIELD DESCRIPTIONS
===============================

1 Signal name (symbolic)
------------------------
  Static name of the signal. To be used in output for legend if field #2
  isn't given. If neither #1 not #2 is given, signal name will be named
  Signal_<nr>, where <nr> is the line-number of the sample-description
  file.

2 Signal name from file
-----------------------
  Signal name is contained in this file. Name is expected to be the only
  content of this file. if string is longer than the length of #1, name
  will be truncated from beginning of text and length characters long.

3 Data-file name
----------------
  Signal data is fetched from this file

4 Datafile operation
---------------------
  An integer value corresponding a bit-field defining data-file mode of
  operation. Note, setting these may also alter the complete sample
  behaviour from rate-monotonic (default behaviour) to event-driven. Not
  setting this defaults to a safe but slow rate-monotonic behaviour where
  each file is constantly opened and closed. Note that some files require
  not having this set default. For example files that can block would block
  the whole sample if not marked "can block"


4.1 Bits
----------
    1: OPENCLOSE File is constantly opened and closed. This is a costly but
       safe operation (as it will always work). It's also the default file
       operation mode if #4 is empty. For some files, like log-files that
       might have been renamed or sysfs files that can dynamically exist
       and disappear, it's a necessary setting.

    2: CANBLOCK Set this indicates file can block. If file is not at the same time
       en event driver (see TRIGGER below), file will be inspected if it
       has something to deliver before read. CANBLOCK implies OPENCLOSE
       being unset.

    3: TRIGGER Updates of this file triggers a complete sample. Updates can
       come in two forms:

       A) Files that CANBLOCK. When new data releases the block for one
          signal, it will drive the event for the complete sample (i.e. all
          other signals too).
       B) Files that are TIMED, I.e. that can't block, but which support
          st_mtime in `struct stat`. In this case, rate-monotonic has to 
          poll the state, but only if it's modified since last sample, will
          it drive an event.

       Special notes:
         Consideration has to be taken into account for signals *not*
         updated since last sample, when events are driving samples, see
         field #9.

         In the physical world, two event's can't occur at exactly the
         same time. One event always occurs before another even ever so
         slightly. In the digital world, due to time-resolution and
         other practical limitations, they can. If more than one TRIGGER
         can drive an event, #9 allows output of the other, there will be
         no possibility to distinguish which is cause and which is
         effect. Even if #9 doesn't allow output for non-updated signals
         (assuming updates can be determined), two TRIGGERs can still
         both contain data, in which case it's a limitation of the kernel
         and scheduler. Ideally, If TRIGGER is set, each sample will
         contain only one field with data, the rest would be empty. This
         is a case that will rarely happen, especially as samples will
         commonly contain files from sysfs, there st_mtime isn't
         supported.

         There might be other types of events concurrently driving samples
         not specified by signal descriptions. For example strace, ftrace
         waitid (possibly filtered) events.

    4: TIMED File has support for st_mtime. File will not be read if stat
       shows it's has not been updated. If not combined with TRIGGER,
       output always falls back on #9

    5: REWIND File that has OPENCLOSE unset can stay open between events to
       optimize sample harvest. However, some files might never the less
       need to be rewind (i.e. lseek(fd, 0 SEEK_SET)) before willing to
       give updated data. This applies to most files under sysfs.

   31: ALWAYS assert file existence. I.e. framework assumes it always
       does and fails execution if it does not. This would be recommended
       if OPENCLOSE is not set for files in sysfs that can come and go
       depending on the HW plug-in framework. cpu-up/cpu-down affected
       files would be a typical example when you would like to have this
       enabled. If uncertain, always set of OPENCLOSE is unset.

5 Regex identifying line to parse.
-----------------------------------
  Can be NULL, which means don't scan, use complete file as data (usually
  one liner data-files which is quite common in sysfs)

  Regex follow extended regex format. If escape characters are needed, no
  need to escape the escape (reg-exp text is read as is). Regex can contain
  sub-expressions, but no special care is taken about these. Match applies
  to the complete line or not at all.

  Scanning file continues until first match our until the match number
  given in #6.. If regex would match more than one line, only the first
  line or the one specified is considered for #7.

6 Line count match
------------------
  When parsing more complex files, #5 can give multiple hits. Consider for
  example /proc/cpuinfo. This field, if given, must contain an integer
  specifying which hit counting from top for which further data extraction
  by #7 applies to. If left empty it defaults to the first hit, unless both
  #5 and #6 are left empty, in  which has it has the special meaning that
  data is not considered for extraction line by line, but instead the whole
  file is considered. In which case regex anchors '^' and '$' loose their
  meaning, but is a quite common case in sysfs were a most files contain
  only value entry (i.e. #5-#8 are empty). There could however exist cases
  when parsing complex files where everything has to be done by one regex
  only. One such case would be parsing accumulative log-files.

7 Signal regex
---------------
  NULL means don't parse, use complete line as is.

  Regex follow extended regex format. If escape characters are needed, no
  need to escape the escape (reg-exp text is read as is). Regex can
  contain sub-expressions, in which case #7 is needed.

8 Sub-match index
-----------------
  Sub-match index, or list of index, represents data. If 0 or NULL,
  complete match is used, in which case #6 is pointless as it means the
  same as using the whole line and #5 alone defines which.

  Having any value here without any signal regex defined, or if any index
  is larger than the largest sub-expression number in #6 would be an
  error.

  NOTE: Errors are not detected until runtime! It's a good idea to test
  your regex strings against known patterns before running them on target.

  If #6 contain sub-expressions this field contain which sub-expression
  contain data. Field is either an integer number, or a list of integer
  numbers.

  If field contain more than one sub-expression which with data, output
  will contain all subexpressions mentioned here, in the same order as
  mentioned an separated by the normal delimiter used as if it would be
  several separate signals. Signal-name outputted in the legend will be
  suffixed with an index corresponding.

  For a more specif signal name, two separate signals need be defined
  parsing the same file and same line need be defined.

9 On no update
--------------
  What to output if during an event no update has occurred for this signal

  0: Nothing. Unfortunately this will confuse feedgnuplot and drivegnuplot,
     therefor please avoid.

  1: Last value (default)

  2: Output value zero. Oscilloscope analogy for a dead or unconnected
     signal.

  As no updates can be an indication of error in either the framework or
  the signal definition, two additional values can be set.

  3: Output pre-set signal value. This is a value passed on command line
     for each signal. If not given on command line it defaults to 0xCACAD0D0.

  4: Output pre-set sample value. This is a hard-coded value defined during
     build time using -DSMPL_FALLBACK_VAL=<val>. If not set, it defaults to
     0xC0DEDEAD.

