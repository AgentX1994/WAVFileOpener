# Wav File Opener
_better name pending_

A simple (hopefully) c++ class to open wav file types.

All .wav loading code is in WavFile.cpp and .hpp and should be platform-agnostic.

main.cpp is osx/ios specific.

Current Features:
* 8-bit, 16-bit, and 24-bit support
* Linear PCM, or Extensible with PCM subtype
* Automatically converts to 32-bit float float internally
* Automatically frees memory when destructed

Planned Features:
* Reference counting with copy constructors and assignment operations
* Maybe support for more formats

