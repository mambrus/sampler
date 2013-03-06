Chapter 0.0: SAMPLER
====================

Produces periodic samples gathered from files in system (typically /sys/kernel/debug).

Samples are either periodical, where period-time is defined in mS by -t flag at
start-up, or event-driven. Event driven samples are defined by a rule describing
the trigger (TBD).

A sample is defined as a series of signals. Each signal is defined by one line
in a file describing all signals. Output format for each sample is on clear-text line where each signal is has it's private column representing the same order it
was mentioned in the descriptive file, prepended by 3 special signals: "sample id", "kernel timestamp" (i.e. time-stamp since start-up) and "sample time" (i.e. time it took to complete a sample). The latter helps determine any jitter due to system load or I/O-wait of any data source.

Full format of the output (assuming delimiter is ";") would be:


"<sample id>";"<kernel time>";"<sample time>";"<signal_0>";.."<signal_n>"

1.1.1 Signal name (symbolic)

1.1.2 Signal name from file

1.1.3 Data-file name

1.1.4 Datafile persistence (0=not persistent i.e. constant reopen, 1=persistent,
 3=best effort)

1.1.5 regexp identifying line to parse. NULL means complete file (usually onle line)

1.1.6 regexp for the line (NULL don't parse, use complete line)

1.1.7 sub-match index represents data



Chapter 1.0: SAMPLE DEFINITION
==============================

One sample is an entity occuring at a specific moment in time. Quite
commonly periodical, or driven by some sort of event.

One sampe has the followng properties.

Field description:
------------------

1 Period time (uS)
------------------
  Time between two samples. 0 or NULL id event-driven

2 Event description (TBD)
-------------------------
  Some form of description about the event. Could be on each signals
  update, or a specific signals update. Could be markes in log e.t.a.

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
  will be truncated from begining of text and and length characters long.

3 Data-file name
----------------
  Signal data is fetched from this file

4 Datafile persistence
----------------------
  An integer value defining the source files persistency, i.e. if file
  needs to be opened constantly.

4.1 Values
----------
    0=not persistent i.e. constant reopen
    1=persistent
    2=best effort

5 Regexp identifying line to parse.
-----------------------------------
  NULL means dont't scan, use complete file (usually onle line)

  Regexp follow extended regexp format. If escape charaters are needed, no
  need to escape the escape (reg-exp text is read as is). Regexp can
  contain sub-expressions, but no special care is taken about these. Match
  applies to the complete line or not at all.

  Scanning file continues until first match. I.e if regexp would mactch
  more than one line, only the first line is considered for #6.

6 Signal regex
---------------
  NULL means don't parse, use complete line as is.

  Regexp follow extended regexp format. If escape charaters are needed, no
  need to escape the escape (reg-exp text is read as is). Regexp can
  contain sub-expressions, in which case #7 is needed.

7 Sub-match index
-----------------
  Sub-match index, or list of index, represents data. If 0 or NULL,
  complete match is used, in which case #6 is pointless as it means the
  same as using the whole line and #5 alone defines which.

  Having any value here without any signal regex defined, or if any index
  is larger than the largest sub-expression number in #6 would be an
  error.

  NOTE: Errors ar not detected until runtime! It's a good idea to test
  your regex strings against known patterns before running them on target.

  If #6 contain sub-expressions this field contain which sub-expression
  contain data. Field is either an integer number, or a list of integer
  numbers.

  If field contain more than one sub-expression which with data, output
  will contain all subexpressions mentioned here, in the same order as
  mentioned an separated by the normal delimiter used as if it would be
  several separate signals. Signal-name outputetd in the legend will be
  suffixed with an index corresponding.

  For a more specic signal name, two separate signals need be defined
  parsig the same file and same line need be defined.
