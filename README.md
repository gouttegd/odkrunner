ODK Runner
==========

This is a proof-of-concept for a binary “runner” tool for the [Ontology
Development Kit](https://github.com/INCATools/ontology-development-kit).
That is, a program that could replace the `run.sh` and `run.bat` scripts
used to invoke the ODK.

Very experimental, do not use in production, etc.

Rationale for a binary runner
-----------------------------
* We need a single runner that works across all the platforms supported
  by the ODK (GNU/Linux, macOS, Windows), instead of having a shell
  script (`run.sh`) for GNU/Linux and macOS, and a batch script
  (`run.bat`) for Windows. Having different runners for different
  platforms makes it likely that they are not equivalent (this is
  exactly the case currently: the batch script is way behind its shell
  counterpart).
* We need a runner that can be executed “out of the box” on the user’s
  system, without requiring the installation of a runtime. This excludes
  almost all scripting languages (certainly all the scripting languages
  that I know of).

Since the runner is a binary program, we need to provide different
versions for each system, but at least those versions are compiled from
the same source code, so there is still only one runner to maintain even
if there are three different binaries to provide.

Copying
-------
The ODK Runner is free software, published under a 3-clause BSD license
similar to the one used by the ODK itself. See the [COPYING](COPYING)
file.
