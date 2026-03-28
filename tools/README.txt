These tools are useful for converting files from various formats into
forms useful for X16 development.

They are written in PHP (apologies to the Python lovers, I don't know
Python very well. PHP has been my go-to scripting language for a long time
and I wanted to go ahead and share these tools prior to any effort spent
converting them into Python, which seems to be a much more common environment
in the X16 community.)

See below for instructions to use PHP in Windows

dmp2x16:
	Converts Deflemask DMP instruments (FM only) into several forms,
	including BASIC, C, ca65 assembly, and raw binary (YMP format).
	It also imports standard OPM instrument files for YM2151 patches.
	with or without the 2-byte PRG header. Use dmp2x16 -H for more
	information on the output formats.

vgm2zsm:
	Converts VGM files into ZSM music stream format. Supports YM2151,
	as well as partial support for other chips such as YM2203 (OPN),
	YM2612 (Sega Genesis), AY-3-8910, and TI SN76489.

	(Apologies in advance for the sapghetti code - this script grew
	from a simple single-purpose function and really needs to be
	rewritten from scratch)

zsmdecode:
	Interprets ZSM files and outputs human-readable text.

raw2zcm:
	Adds a Zsound PCM info header to the beginning of a raw PCM file, and
	limits output to 2MB size (minus 8K since bank 0 of X16 is reserved
	by the Kernal). Planned features for this are to support various
	formats for input (WAV, MP3, etc) and convert them into ZCM.

zsm2sfx:
	Extracts a single FM voice from a revision-1 ZSM file and converts it
	into two outputs suitable for the ZFX FM helper:
		- a 26-byte YMP patch block
		- a reg/value/delay FM triplet stream
	If the ZSM uses exactly one FM voice, the tool auto-selects it.
	Otherwise pass -v to choose the source YM voice.

vgmconvert:
	This is an in-development project to replace the vgm2zsm tool with a
	more flexible implementation in Python. It's totally unusable until
	further notice.
	
Windows PHP:

The PHP interpreter can be downloaded here:
https://windows.php.net/download/#php-7.4

The script has been tested with PHP 7.4

Unzip the distribution into any folder (e.g. c:\php-7.4.25)
To temporarily add that folder to the path:

	C:>path=%path%;c:\php-7.4.25

To permanently modify your path... Google has answers.

Once PHP is in your path, you may execute the scripts from the commandline:
	php dmp2x16 ...
	php vgm2zsm ...
