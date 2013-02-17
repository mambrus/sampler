sampler
=======

Produces periodic samples gathered from files in system (typically /sys/kernel/debug).

Samples are either periodical, where period-time is defined in mS by -t flag at
start-up, or event-driven. Event driven samples are defined by a rule describing
the trigger (TBD).

A sample is defined as a series of signals. Each signal is defined by one line
in a file describing all signals. Output format for each sample is on clear-text line where each signal is has it's private column representing the same order it
was mentioned in the descriptive file, prepended by 3 special signals: "sample id", "kernel timestamp" (i.e. time-stamp since start-up) and "sample time" (i.e. time it took to complete a sample). The latter helps determine any jitter due to system load or I/O-wait of any data source.

Full format of the output (assuming delimiter is ";") would be:


<sample id>;<kernel time>;<sample time>;<signal_0>;..<signal_n>

1.1.1 Signal name (symbolic)

1.1.2 Signal name from file

1.1.3 Data-file name

1.1.4 Datafile persistence (0=not persistent i.e. constant reopen, 1=persistent,
 3=best effort)

1.1.5 regexp identifying line to parse. NULL means complete file (usually onle line)

1.1.6 regexp for the line (NULL don't parse, use complete line)

1.1.7 sub-match index represents data



