SEQ2MID v1.0b3 -SATURN VERSION-
================================


Introduction
-------------
This program was designed to Scan files off a cd or hard drive, rip Saturn Sequence 2.00 files,
and convert them into a format readable on most people's computers: Midi. I'd like to point out
that the instruments chosen may or may not be correct. This isn't a bug, but rather just a
limitation of the seq format.


Usage
------
Not sure why I'm even bothering with this section as even a monkey could figure it out. Type
the following where filename.ext is the file you want to search and rip from. seq2mid will
automatically output the midi's in the same directory as seq2mid, so make sure the directory 
it's in is writeable.

seq2mid [filename.ext]


Version History
----------------
1.0rc1: -Overhauled gate queuing yet again. Delta/Gate conversion appears to be almost perfect,
         if not perfect. This also fixed a bug where some notes would be left on after a loop.
        -Didn't realize Extend Delta events weren't supposed to be counted as a referenced 
         event. Fixed a number of Lunar's Seq's(probably others as well). Updated seqformat.txt
         to reflect this.
        -Fixed a bug which caused each midi to have 4 bytes missing at the end.
        -Added a bit of a temporary hack that makes sure that all Note On's have been Note
         Off'ed by the time LoopEnd is called. 
1.0b3: -Fixed a few oddball gate queuing bugs(whoops).
       -Gate queuing went through a bit of an overhaul. This fixed midi's that wouldn't play in
        Windows Media Player and had notes that wouldn't cut off when they were supposed to.
       -Added command line switch for a second looping method I added. Run the program without
        any file specified to see a list of switches.
       -Added Seq/Midi statistics as a command switch.
       -Fixed a buffer overflow bug.
       -Added a hack to get Seq's from Langrisser DE detected.
       -Fixed a bug in detecting seq banks with multiple seq's.
1.0b2: -Fixed a bug which prevented the output midi's from being played in Windows Media Player
1.0b1: -Initial release


Release Notes
--------------
Keep in mind this beta-ish software, and may behave as such if provoked ;P But
for the most part it should work fine. 


Special Thanks
---------------
-The saturn fans that keep the system alive
-segadev mailing list and other coders enjoying what's left of the saturn dev scene. 
-Sega for writing useless outdated docs for their developers. I'm honestly amazed sometimes at
how developers even made anything for the system.
-I borrowed a snippet of code from the following page: 
 http://www.borg.com/~jglatt/tech/midifile.htm
 Not sure who exactly wrote it, but thanks anyways. 
-Marshal Gladstone for all his documents and sample code


Cyber Warrior X
cwx@softhome.net
