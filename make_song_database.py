from pathlib import Path
from mutagen.easyid3 import EasyID3


# Your music folder
MUSIC_FOLDER = Path(
    r"C:\Users\Paul Anderson\Desktop\MP3\New"
)

# songs.txt will be created next to this Python script
OUTPUT_FILE = Path(__file__).parent / "songs.txt"


def get_filename_info(filename):
    """
    Try to get title and artist from a filename such as:

        ABBA - Chiquitita.mp3

    Returns:
        artist, title
    """

    name = Path(filename).stem

    if " - " in name:
        artist, title = name.split(" - ", 1)
        return artist.strip(), title.strip()

    return "Unknown Artist", name.strip()


def fat32_sort_key(file):
    """
    Sort filenames using these rules:
        1. Symbols first
        2. Numbers second
        3. Letters A-Z third
        4. Ignore dashes when comparing filenames
        5. Ignore capitalization
    """

    name = file.name.lower()

    # Ignore dashes when comparing filenames
    name = name.replace("-", "")

    # Determine the sorting category
    if not name:
        category = 0
    elif name[0].isalpha():
        category = 2
    elif name[0].isdigit():
        category = 1
    else:
        category = 0

    return (category, name)


# Get all MP3 files and sort by filename
files = sorted(
    MUSIC_FOLDER.glob("*.mp3"),
    key=fat32_sort_key
)


songs = []

print("Scanning music library...")
print()

for file in files:

    filename_artist, filename_title = get_filename_info(file.name)

    try:
        tags = EasyID3(file)

        title = tags.get("title", [None])[0]
        artist = tags.get("artist", [None])[0]
        album = tags.get("album", ["Unknown Album"])[0]

        # Use metadata when available
        if not title:
            title = filename_title

        if not artist:
            artist = filename_artist

        if not album:
            album = "Unknown Album"

    except Exception:
        # No readable ID3 metadata
        title = filename_title
        artist = filename_artist
        album = "Unknown Album"

    songs.append({
        "filename": file.name,
        "title": title,
        "artist": artist,
        "album": album
    })


# Create the database
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:

    for number, song in enumerate(songs, start=1):

        f.write(
            f"{number}|"
            f"{song['filename']}|"
            f"{song['title']}|"
            f"{song['artist']}|"
            f"{song['album']}\n"
        )


print("--------------------------------")
print(f"Found {len(songs)} MP3 files.")
print("Database created successfully!")
print()
print("Location:")
print(OUTPUT_FILE)
print("--------------------------------")