# VFO

vfo stands for "Video File Organiser"

[Download latest release from GitHub](https://github.com/CraigWatt/vfo/releases/latest)

VFO requires ffmpeg (and ffprobe) to be installed on your machine.

This is a great project to dive into using and/or contributing to if you have any interest in C programming and/or video/audio encoding!  

Feel free to ask any questions here: https://github.com/CraigWatt/vfo/discussions/27

## VFO runs on:
  
  macOS (Intel & Arm)
  
## VFO will, very soon, be able to run on:
  
  Linux (Ubuntu and similar)
  
  Windows (10 & 11 and similar)
  

### Project Description

VFO is like an evolution of HandBrake's queue feature.  Instead of having to 'apply preset to all' candidate videos or 'tweak each and every video 1 by 1', VFO makes it easier to encode/remux an entire portfolio of source video files WHILE STILL having the option to encode individual video files in particular ways, DEPENDING on what that source video candidate consists of.

For example, let's say you are a streaming service, or you just happen to have a catalog of videos of which you can't say with certainty how consistent they are.  Maybe you have a bunch of h264 encoded videos, some below 720p, some around 720p, some around 1080p?  But a few of them are HDR while most aren't?  In fact you know a good portion might be hevc encoded too, but some are actually only of 8 bit color depth and some are 10 bit.  Perhaps none of their extensions are all the same?  Some use mp4 video container, some use .mkv, maybe some use webm?  Darn you also have those super old videos encoded in MPEG-2!  

I could go on, but hopefully this conveys something like the considerations YOU have as an enthusiast video/audio encoder.

The goal of VFO is to save this kind of user time WHILE STILL allowing them to keep full control of their output quality.

VFO can do this by allowing the user to DEFINE ALIAS'S and SCENARIOS.  An ALIAS represents a group of video files that HAVE BEEN encoded by vfo FROM your group of SOURCE video files. You can set CRITIERIA for ALIAS'S.  A simple example could be, say you are trying generate encoded video files of the highest possible quality for an end user that is using a low-end streaming device.  We know that device can only decode something like h264 and can't touch hevc (certainly not av1).
We also set the criteria of SDR (bt709), becuase we know the device cannot support HDR OR it is highly unlikely the end user is connected to an HDR capable TV.  We also would then know the TV only supports a maximum of 1080p resolution.

We have just defined some CRITIERA that we can tie to our alias.  Let's call the alias TIGER so we can refer to it.

Now that we have set the ALIAS_NAME and have set TIGER_CRITERIA, we can now set some TIGER_SCENARIO's.  

Scenario's are used when we want to determine whether or not a candidate video MATCHES our defined scenarios.  Almost like a single if statement...if candidate file resoltion is greater than 1080p...do something!

You can then tie a specific FFMPEG_COMMAND to that SCENARIO.  The FFMPEG is the 'do something' part, it is run IF the particular candidate video file that is being looked at (at that given moment) to see if it MATCHES the SCENARIO.

Like you can define any number of ALIAS's, you can define any number of SCENARIO's within those ALIAS'S.

Just like using ffmpeg standalone, you can define each SCENARIO's FFMPEG COMMAND however you want.  Simply supstitute $vfo_input for where you would typically type your input video file, and $vfo_output for your output.

When executed, VFO will handle ALL source candidate video files, check ALL Alias's to scan for a matching scenario while retaining the same folder structure of your entire source candidate files for the Alias's folder.

This should save you time, especially when you are juggling with a large amount of 'different' video files because VFO can process everything but also make key decisions on HOW to encode based on your configuration!

### In future I'll be providing:

1. Real vfo_config.conf examples within this README to make it easier to follow along, along with REAL vfo CLI commands.

2. two companion vidoes in future that 1: explains the basics on how to use this program 2: how the program works (i.e. the programming involved to make vfo).

Again, feel free to ask any questions you may have both on HOW TO USE vfo and DEVELOPMENT/CONTRI

Thanks,

Craig
