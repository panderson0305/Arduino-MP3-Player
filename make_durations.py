from pathlib import Path
from mutagen.mp3 import MP3

# Your music folder
MUSIC_FOLDER = Path(r"C:\Users\Paul Anderson\Desktop\MP3\New")

# Output file next to this Python script
OUTPUT_FILE = Path(__file__).parent / "song_durations.h"


# Sort exactly the same way as the DFPlayer
files = sorted(
    MUSIC_FOLDER.glob("*.mp3"),
    key=lambda f: f.name.lower()
)


print("Reading song durations...")
print()

durations = []

for number, file in enumerate(files, start=1):

    try:
        audio = MP3(file)

        # Duration in seconds
        duration = round(audio.info.length)

        durations.append(duration)

        print(f"{number}: {file.name} -> {duration} seconds")

    except Exception as e:

        print(f"Could not read {file.name}: {e}")

        # Use 0 if duration could not be read
        durations.append(0)


# Create Arduino header file
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:

    f.write("#ifndef SONG_DURATIONS_H\n")
    f.write("#define SONG_DURATIONS_H\n\n")

    f.write(f"const int TOTAL_SONGS = {len(durations)};\n\n")

    f.write("const unsigned long songDurations[] = {\n")

    # Put 10 durations per line
    for i in range(0, len(durations), 10):

        chunk = durations[i:i + 10]

        f.write("  ")

        f.write(", ".join(str(duration) for duration in chunk))

        if i + 10 < len(durations):
            f.write(",")

        f.write("\n")

    f.write("};\n\n")

    f.write("#endif\n")


print()
print("--------------------------------")
print(f"Found {len(files)} MP3 files.")
print("Duration database created!")
print()
print(f"Location:")
print(OUTPUT_FILE)
print("--------------------------------")