# wav-utils-cpp

### Project Goals:
 - To provide users an easily understandable and simple to use library for .wav files
 - To provide an explaination of how .wav files are formatted

### Deliverables:
 - A header-only library of classes that handles .wav files and their metadata
 - A suite of unit tests to ensure quality is met
 - A basic implementation to serve as a tutorial through the heavy use of comments

### Building and Usage

This is a header-only library. To use it in your project, simply include the headers:

```cpp
#include <wav/Reader.hpp>

wav::Reader reader("audio.wav");
if (reader.open()) {
    std::cout << "Channels: " << reader.getNumChannels() << std::endl;
}
```

To build the examples:

```bash
cmake -B build -S .
cmake --build build
./build/examples/basic_usage <your_wav_file.wav>
```

#### Resources:

- riff-specs.pdf (see pages 56-65)
- WAVE File Format.html