                EPICS WIN32 Extensions Build 13
                ===============================

     This package includes versions of the EPICS extensions that are
built to run on WIN32 platforms, that is, Windows 95 and Windows NT.
The extensions that use X Windows require the Exceed X Server by
Hummingbird in order to run.

     This release should be considered a beta test release.  The
programs are still under development for use on WIN32 platforms.

     If you are interested in being put on the mailing list for future
releases of the EPICS WIN32 Extensions, send email to
evans@aps.anl.gov.  Bugs and comments may be sent to the author of the
extension or to evans@aps.anl.gov.  To be useful, bugs should include
explicit steps that the responsibe person can use to reproduce the
problem.

     If you have a previous build of EPICS WIN32 Extensions installed,
it is better to uninstall it before installing the new build.  This
will insure a cleaner installation.  You can uninstall EPICS WIN32
Extensions via Add/Remove Programs in the Control Panel.  If you
previously installed the extra programs, and no longer want them, you
definitely should do an uninstall before the install.  Otherwise,
needless registry keys will be left.

     It is important to close any programs from a previous
installation that might be running.  In particular, be sure to close
CaRepeater.  CaRepeater will not appear on the Task Bar but can be
closed in the Dialog Box that comes when you type Ctrl-Alt-Delete.

     Setup installs the necessary files in the directory of your
choice, by default in:

  c:\Program Files\EPICS WIN32 Extensions

     Setup should be able to tell which version of Exceed you are
running and install the appropriate DLLs and support files.  Most of
the programs are built with Exceed 5 and will run on either version,
provided the proper DLLs are installed or already exist.  If Setup
cannot tell the Exceed version, you can pick the support files you
want by doing a Custom setup, but things may not work correctly until
Exceed is installed properly.  If you change your Exceed version
or the location of the Exceed directory, then you should do an
uninstall and a reinstall.

PATH
====

     It should not be necessary to have your Exceed directory in your
PATH to run the X programs, as was required in early editions of the
EPICS WIN32 Extensions.  The location of the EPICS WIN32 Extensions
and the Exceed path, if applicable, is stored in the registry for each
program.  (Run RegEdit and look for xxx.exe if you want to see the
key.)  This allows you to run programs from Run on the Start Menu or
by using the START command (e.g. "start medm") in a DOS window.
Moreover, it insures you will get DLLs from the installed directory
and from the Exceed directory.  A possible side effect is that the
paths in the registry are prepended to your path variable when you run
the programs.  This means you cannot get different versions if the
DLLs by putting other directories in your PATH.  (You should normally
not want to do this anyway.)

CaRepeater
=========

     Most channel access programs require CaRepeater to be running and
will try to start it if it is not running.  These applications should
be able to find CaRepeater without your doing anything special.  You
can also start CaRepeater yourself, perhaps by putting it in your
StartUp program group.

Environment Variables
=====================

     You may need to define environment variables to run the EPICS
WIN32 Extensions.  What you need to install depends on several things
as described below.  Typically, however, you will probably have to set
XAPPLRESDIR, XKEYSYMDB, and EPICS_CA_ADDR_LIST, as well as add the
Exceed directory to your path.  If you have Exceed 5, you will
typically have to add XNLSPATH.  Samples are given below that you can
copy to your AUTOEXEC.BAT and mofify.

     If you have Exceed with the XDK, you should not need to set
XAPPLRESDIR, XKEYSYMDB, nor XNLSPATH.  These are set in the Registry
for the XDK.

     You may need to define variables such as EPICS_CA_ADDR_LIST to
allow channel access to find your process variables.  A description of
these variables is in the Channel Access manual.  These can be set in
your AUTOEXEC.BAT (Windows 95) or System Properties/Environment
(Windows NT) file using lines like the following:

set EPICS_CA_ADDR_LIST=164.54.188.65

     It may be convenient to set EPICS_DISPLAY_PATH.  This is the path
list that MEDM uses to find ADL files.  The paths of directories that
contain ADL files are specified with a semi-colon as a delimiter.
(This is different than on UNIX, where the delimiter is a colon.)  See
the MEDM Reference Manual for more information.  An example of
how to set this variable is:

set EPICS_DISPLAY_LIST=c:\My Documents\adl;n:\medm\adl;n:\adlsys

     If you want to access ADL files that are in a system directory on
UNIX, you can make a link in your UNIX home directory to the system
directory:

ln -s /usr/local/iocapps/adlsys  ~/adlsys

then mount that drive on your PC (as n: for example) and put the
directory in the EPICS_DISPLAY_PATH as above.

     You will need to set environment variables to use Exceed if you
do not have the version with the XDK.  The file StartX.bat in this
distribution will do this.  You must edit it first to fit your
situation.  There are comments in the file that may help you.
Eventually or now, it would probably be better to put the appropriate
lines in your AUTOEXEC.BAT (Windows 95) or System
Properties/Environment (Windows NT).  These are typical lines:

Set this to locate resource files:

set XAPPLRESDIR=c:\Program Files\EPICS WIN32 Extensions

Set this to locate the keysym database.  Note that the names are
different for Exceed 5 (xkeysmdb) and Exceed 6 (XKeySymDB).  You will
almost surely have to set this variable unless you have the XDK:

set XKEYSYMDB=c:\Program Files\EPICS WIN32 Extensions\xkeysmdb
set XKEYSYMDB=c:\Program Files\EPICS WIN32 Extensions\XKeySymDB
   
Set this to locate the locale files if you have Exceed 5.  You will
almost surely have to set this variable if you have Exceed 5 unless
you have the XDK:

set XNLSPATH=c:\Program Files\EPICS WIN32 Extensions

Set this if you have an Xdefaults file:

set XENVIRONMENT=c:\Program Files\EPICS WIN32 Extensions\Xdefaults

Set these to log what goes to the local console.  Use these for
debugging.  The log file will continue to grow if you always have
these set:

set LOGGING=YES
set LOGFILE=c:\Program Files\EPICS WIN32 Extensions\exceed.log

     If you run out of environment space, you need to put something
like the following line in your CONFIG.SYS:

shell=command.com /e:1024 /p

     The following are examples for Exceed 5 and 6.  This should be a
sufficent set of environment variables to make the EPICS WIN32
Extensions run.  You can copy these lines to yuour AUTOEXEC.BAT and
modify them as needed.

Exceed 5:
set EPICS_CA_ADDR_LIST=164.54.188.65
set EPICS_DISPLAY_LIST=c:\My Documents\adl;n:\medm\adl;n:\adlsys
set XAPPLRESDIR=c:\Program Files\EPICS WIN32 Extensions
set XKEYSYMDB=c:\Program Files\EPICS WIN32 Extensions\xkeysmdb
set XNLSPATH=c:\Program Files\EPICS WIN32 Extensions

Exceed 6:
set EPICS_CA_ADDR_LIST=164.54.188.65
set EPICS_DISPLAY_LIST=c:\My Documents\adl;n:\medm\adl;n:\adlsys
set XAPPLRESDIR=c:\Program Files\EPICS WIN32 Extensions
set XKEYSYMDB=c:\Program Files\EPICS WIN32 Extensions\XKeySymDB

Window Placement
================

     Exceed does not handle window positioning well.  If you want
windows to be positioned better, especially to avoid windows jumping
around in MEDM, you can put the following in your WIN.INI file:

[Exceed]
configureWindowPositionToClient=1

This appears to work for both Exceed 5 and Exceed 6.

Winsock 2
=========

     The versions of channel access included here require Winsock 2 to
be installed on your system.  If you have Windows NT, it is probably already
installed.  If you have Windows 95, it may not be installed or a bad
version may have been installed.  If you have the file:

C:\WINDOWS\SYSTEM\ws2_32.dll

on your Windows 95 system, you probably have Winsock 2 installed.  You
can look at the Properties of this file and determine the version.  It
should be 4.10.1656 or later.  There is a bug in widely distributed,
earlier versions of ws2_32.dll.  Channel access will not work well
with these earlier versions.  You can get a good version of Winsock 2
at:

http://www.microsoft.com/windows95/info/ws2.htm

     If you get an error message about not finding WS2_32.DLL, you do
not have Winsock 2 installed or it is not installed properly.

Exceed
======

    The X Windows programs require an X Server to run.  The only X
Server supported is Exceed by Hummingbird, http://www.hcl.com.  The
WIN32 versions have been built with X and Motif libraries supplied
with the Exceed XDK.  You do not need the XDK to run these programs.
You do need Exceed.

    The current version of Exceed is Exceed 6 which supports X Release
6.  The programs supplied here are mostly built with Exceed 5 so you
do not have to upgrade to Exceed 6 if you already have Exceed 5.
Exceed 6 has some improvements but it appears to be more than a factor
of two slower than Exceed 5, at least on Windows 95.  (See the Chaos
timing information below.)  Eventually, it is probably the case that
only Exceed 6 will be supported.

Fonts
=====

     MEDM uses a set of font aliases that will not be installed with
Exceed by default.  See the MEDM reference Manual for several ways to
make these fonts available.  If you have Netscape or Internet Explorer
installed, you should be able to view the MEDM Reference Manual via
the Help button in MEDM.  Otherwise, the Reference Manual is with the
other EPICS documentation.  The location is given below.  There is a
sample file, fontTable.adl, in the adl directory of EPICS WIN32
Extensions.  This will show you what you are getting for the MEDM
fonts.

Programs
========

     The EPICS Extension provided in this release are:

        caRepeater.exe
        ca_test.exe
        medm.exe
        probe.exe
        namecapture.exe

     The documentation for these programs is the same as for the UNIX
versions and may be found at:

http://www.aps.anl.gov/asd/controls/epics/EpicsDocumentation/WWWPages/EpicsDoc.html

     There is a subdirectory of EPICS WIN32 Extensions named adl.  It
contains some sample ADL files for MEDM.  The MEDM icon in the EPICS
WIN32 Extensions group initially starts MEDM in this directory.  You
may change that, and it is suggested that you do not put your own ADL
files in that directory.  See the discussion under EPICS_DISPLAY_LIST
above.

Extra Programs
==============

     These are programs that may be of interest and are included for
testing purposes.  You need to do a Full or Custom install to get
them.

Chaos.exe:
Chaos5.exe:

     This is an X program that does Mandelbrodt plots.  It uses Exceed
but not EPICS.  Chaos5.exe is built with Exceed 5 and Chaos.exe is
built with Exceed 6.  The UNIX version is built with almost identical
code.  It makes a nice timing test for X Windows programs converted to
WIN32.  For the standard Mandelbrodt plot done with the screen
maximized, informal testing (on UNIX or Windows 95 unless noted) has
indicated:

Machine                  X Server        Time (sec)
-------                  --------        ----------
486DX2 66 MHz            Exceed 5        27.27
Toshiba Satellite Pro    Exceed 5         5:31
Pentium 200 MHz          Exceed 5         1:14
Pentium 300 MHz          Exceed 5         0:46

Pentium 200 MHz          Exceed 6         3:55  (Built with Exceed 5)
Pentium 200 MHz          Exceed 6         3:55  (Built with Exceed 6)

Sparc 20                 UNIX             2:42
Sparc Ultra 1            UNIX             1:46
Sparc Ultra 2            UNIX             1:35
Sparc Ultra 30           UNIX             1:09

WinProbe.exe:

     This is a native Windows version of Probe that does not use
Exceed but that does use EPICS Channel Access.  It was originally
written by Fred Vong and is not being developed at this time, but it
should work.

SM.exe:

     This is a native Windows program that does not use Exceed nor
EPICS.  It demonstrates the physics of Synchrotron Motion in an
accelerator.  It also illustrates the Standard Equation of Nonlinear
dynamics.  Further information can be obtained from its Help button.

Troubleshooting
===============

     You get dialog boxes saying the program cannot find DLLs that
have names starting with "HC" or "HCL": Your Exceed directory is
probably not in your path.  See above.

     You put your Exceed directory in the PATH, but you still get
dialog boxes saying it cannot find the Exceed DLLs: You may have run
out of environment space.  Try typing PATH or SET in a DOS window to
see if it is really set right.  If you are on Windows 95, try running
AUTOEXEC.BAT in a DOS window, and see if it says it is out of
environment space.  If so, put a line in your CONFIG.SYS as indicated
above to get more environment space.

     The X programs sort of work, but the keys, especially BS, do not
behave properly: Set XAPPLRESDIR and XKEYSYMDB as described above.

     You only get a single-sized, default font in MEDM: You probably
need to put the MEDM aliases in your font path.  See the discussion
above or the MEDM Reference Manual.
     
     You have a problem but you do not know if it is with Exceed,
EPICS, or Windows: Do a full installation and try running the extra
programs.  They require various combinations of Exceed, EPICS, and
Windows.

     The programs, especially Probe, do not appear the first time: Try
running them again.  This problem is under investigation and may be
related to CaRepeater.  It should be fixed in this version.

     There are problems with setup when doing a reinstallation or with
uninstall: Be sure all programs are closed and that CaRepeater is
killed with Ctrl-Alt-Delete.
