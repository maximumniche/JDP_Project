from pydub import AudioSegment
import numpy as np
import sys

OUTPUT_FILE = "samples.h"
TARGET_RATE = 30000

samplesList = []

for i in range(1, len(sys.argv)):

    audio = AudioSegment.from_file(sys.argv[i])
    audio = audio.set_channels(1)
    audio = audio.set_frame_rate(TARGET_RATE)

    samples = np.array(audio.get_array_of_samples()).astype(np.int16)

    samplesList.append(samples)

NUM_SAMPLES = len(samplesList)
MAX_LEN = max(len(s) for s in samplesList)

with open(OUTPUT_FILE, "w") as f:

    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n\n")

    f.write(f"#define SAMPLE_RATE {TARGET_RATE}\n")
    f.write(f"#define NUM_SAMPLES {NUM_SAMPLES}\n")
    f.write(f"#define MAX_SAMPLE_LEN {MAX_LEN}\n\n")

    # Array with sample lengths
    f.write("const uint32_t sampleLengths[NUM_SAMPLES] = {\n")

    for i, s in enumerate(samplesList):

        f.write(f"    {len(s)}")

        if i != NUM_SAMPLES - 1:
            f.write(",")

        f.write("\n")

    f.write("};\n\n")

    # 2D Array w/ every sample array
    f.write("const int16_t audioSamples[NUM_SAMPLES][MAX_SAMPLE_LEN] = {\n")

    for sampleIdx, samples in enumerate(samplesList):

        f.write("    {\n")

        for i in range(MAX_LEN):

            if i < len(samples):
                v = int(samples[i])
            else:
                v = 0 # pad shorter samples

            f.write(str(v))

            if i != MAX_LEN - 1:
                f.write(", ")

            if i % 16 == 15:
                f.write("\n")

        f.write("\n    }")

        if sampleIdx != NUM_SAMPLES - 1:
            f.write(",")

        f.write("\n")

    f.write("};\n")

print("Done")