from pydub import AudioSegment
import numpy as np

INPUT_FILE = "input.mp3"
OUTPUT_FILE = "audio.h"
TARGET_RATE = 44100

audio = AudioSegment.from_file(INPUT_FILE) # load mp3 (decode via ffmpeg)
audio = audio.set_channels(1) # convert to mono
audio = audio.set_frame_rate(TARGET_RATE)  #  set sample rate
samples = np.array(audio.get_array_of_samples()) # get raw PCM data
samples = samples.astype(np.int16) #  ensure int16
print("Samples:", len(samples))
print("Sample rate:", TARGET_RATE)

with open(OUTPUT_FILE, "w") as f: # write C header

    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"#define SAMPLE_RATE {TARGET_RATE}\n")
    f.write(f"#define SAMPLE_LEN {len(samples)}\n\n")
    f.write("const int16_t audio_sample[SAMPLE_LEN] = {\n")
    for i, v in enumerate(samples):
        f.write(str(int(v)))
        if i != len(samples) - 1:
            f.write(", ")
        if i % 16 == 0:
            f.write("\n")
    f.write("\n};\n")
print("Done -> audio.h generated")