#include <DFRobotDFPlayerMini.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "song_durations.h"
#include "song_info.h"


// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// DFPlayer
DFRobotDFPlayerMini player;


// Player settings
int volume = 20;
bool playing = false;
int currentTrack = 1;


// Buttons
const int playPauseButton = 2;
const int nextButton = 3;
const int previousButton = 4;
const int shuffleButton = 5;

const int volumePot = A0;


// Track timing
unsigned long trackStartTime = 0;
unsigned long pausedTime = 0;


// Ignore stale DFPlayer finish messages briefly
unsigned long lastTrackChangeTime = 0;
const unsigned long finishIgnoreTime = 1500;


// ---------------- SHUFFLE ----------------

// Stores the shuffled order of the songs
uint16_t shuffleOrder[TOTAL_SONGS];

// Position inside the shuffled order
int shuffleIndex = 0;

// Shuffle state
bool shuffleEnabled = false;


// ---------------- DISPLAY ----------------

void updateDisplay() {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Header
  display.setCursor(0, 0);
  display.print("Altoid Tunes");
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // Look up the title for the current DFPlayer track.
  // This compact title-only table keeps the sketch within flash limits.
  char songTitle[SONG_TITLE_BUFFER_SIZE];

  bool foundSongInfo = getSongTitle(
    currentTrack,
    songTitle,
    sizeof(songTitle)
  );

if (!foundSongInfo) {
  strcpy(songTitle, "Unknown title");
}

  // Use two rows for long titles (about 21 characters per row).
  display.setCursor(0, 12);
  if (strlen(songTitle) > 21) {
    char secondLine[22];
    strncpy(secondLine, songTitle + 21, sizeof(secondLine) - 1);
    secondLine[sizeof(secondLine) - 1] = '\0';
    songTitle[21] = '\0';
    display.print(songTitle);
    display.setCursor(0, 21);
    display.print(secondLine);
  } else {
    display.print(songTitle);
  }

  // Compact status row: track number, playback state, and shuffle state.
  display.setCursor(0, 30);
  display.print("#");
  display.print(currentTrack);
  display.print(" ");
  display.print(playing ? "PLAY" : "PAUSE");
  if (shuffleEnabled) {
    display.print(" SHUF");
  }

  // Get song duration
  unsigned long totalSeconds = 0;
  if (currentTrack >= 1 && currentTrack <= TOTAL_SONGS) {
    totalSeconds = songDurations[currentTrack - 1];
  }

  // Calculate elapsed time
  unsigned long elapsedSeconds = pausedTime;
  if (playing) {
    elapsedSeconds = (millis() - trackStartTime) / 1000;
  }
  if (totalSeconds > 0 && elapsedSeconds > totalSeconds) {
    elapsedSeconds = totalSeconds;
  }

  // Progress bar
  display.drawRect(0, 39, 128, 9, SSD1306_WHITE);
  if (totalSeconds > 0) {
    int progressWidth = (elapsedSeconds * 124UL) / totalSeconds;
    if (progressWidth > 0) {
      display.fillRect(2, 41, progressWidth, 5, SSD1306_WHITE);
    }
  }

  // Elapsed / total time
  display.setCursor(0, 50);
  int currentMinutes = elapsedSeconds / 60;
  int currentSeconds = elapsedSeconds % 60;
  if (currentMinutes < 10) display.print("0");
  display.print(currentMinutes);
  display.print(":");
  if (currentSeconds < 10) display.print("0");
  display.print(currentSeconds);
  display.print(" / ");

  int totalMinutes = totalSeconds / 60;
  int totalRemainingSeconds = totalSeconds % 60;
  if (totalMinutes < 10) display.print("0");
  display.print(totalMinutes);
  display.print(":");
  if (totalRemainingSeconds < 10) display.print("0");
  display.print(totalRemainingSeconds);

  // Volume indicator stays at the right side of the bottom row.
  display.setCursor(98, 57);
  display.print("V:");
  display.print(volume);

  display.display();
}

// ---------------- START TRACK ----------------

void startTrack(int trackNumber) {

  currentTrack = trackNumber;

  player.play(currentTrack);

  playing = true;

  trackStartTime = millis();
  pausedTime = 0;

  // Remember when we changed tracks
  lastTrackChangeTime = millis();

  Serial.print("Playing track ");
  Serial.println(currentTrack);

  updateDisplay();
}


// ---------------- BUILD SHUFFLE ORDER ----------------

void buildShuffleOrder(int avoidTrack) {

  // Put every track into the array
  for (int i = 0; i < TOTAL_SONGS; i++) {

    shuffleOrder[i] = i + 1;
  }


  // Fisher-Yates shuffle
  for (int i = TOTAL_SONGS - 1; i > 0; i--) {

    int j = random(i + 1);

    uint16_t temp = shuffleOrder[i];

    shuffleOrder[i] = shuffleOrder[j];

    shuffleOrder[j] = temp;
  }


  // If we need to avoid immediately
  // repeating a track, make sure the
  // first shuffled track isn't that track.
  if (avoidTrack > 0 &&
      TOTAL_SONGS > 1 &&
      shuffleOrder[0] == avoidTrack) {

    int swapIndex = 1;

    while (swapIndex < TOTAL_SONGS &&
           shuffleOrder[swapIndex] == avoidTrack) {

      swapIndex++;
    }

    if (swapIndex < TOTAL_SONGS) {

      uint16_t temp = shuffleOrder[0];

      shuffleOrder[0] = shuffleOrder[swapIndex];

      shuffleOrder[swapIndex] = temp;
    }
  }
}


// ---------------- TURN SHUFFLE ON ----------------

void enableShuffle() {

  // Create a randomized order
  // while avoiding the current song
  // as the next song.
  buildShuffleOrder(currentTrack);


  // Find the current track in the
  // shuffled order.
  shuffleIndex = 0;

  for (int i = 0; i < TOTAL_SONGS; i++) {

    if (shuffleOrder[i] == currentTrack) {

      shuffleIndex = i;
      break;
    }
  }


  shuffleEnabled = true;

  Serial.println("Shuffle ON");

  updateDisplay();
}


// ---------------- TURN SHUFFLE OFF ----------------

void disableShuffle() {

  shuffleEnabled = false;

  Serial.println("Shuffle OFF");

  updateDisplay();
}


// ---------------- SHUFFLE TOGGLE ----------------

void toggleShuffle() {

  if (shuffleEnabled) {

    disableShuffle();

  }
  else {

    enableShuffle();
  }
}


// ---------------- NEXT TRACK ----------------

void nextTrack() {

  if (shuffleEnabled) {

    // Move forward in shuffled order
    shuffleIndex++;

    // If we've reached the end,
    // create a fresh shuffled order.
    if (shuffleIndex >= TOTAL_SONGS) {

      int oldTrack = currentTrack;

      buildShuffleOrder(oldTrack);

      shuffleIndex = 0;
    }

    startTrack(shuffleOrder[shuffleIndex]);

  }
  else {

    // Normal sequential playback
    currentTrack++;

    if (currentTrack > TOTAL_SONGS) {
      currentTrack = 1;
    }

    startTrack(currentTrack);
  }
}


// ---------------- PREVIOUS TRACK ----------------

void previousTrack() {

  if (shuffleEnabled) {

    // Move backward in shuffled order
    if (shuffleIndex > 0) {

      shuffleIndex--;

      startTrack(shuffleOrder[shuffleIndex]);
    }

  }
  else {

    // Normal sequential playback
    // DO NOT wrap around at track 1.
    if (currentTrack > 1) {

      currentTrack--;

      startTrack(currentTrack);
    }
  }
}


// ---------------- PLAY / PAUSE ----------------

void togglePlayPause() {

  if (playing) {

    player.pause();

    pausedTime =
      (millis() - trackStartTime) / 1000;

    playing = false;

    Serial.println("Paused");

  }
  else {

    player.start();

    trackStartTime =
      millis() - (pausedTime * 1000);

    playing = true;

    Serial.println("Playing");
  }

  updateDisplay();
}


// ---------------- VOLUME KNOB ----------------

void updateVolumeFromKnob() {

  long total = 0;

  // Take 8 readings and average them
  for (int i = 0; i < 8; i++) {
    total += analogRead(volumePot);
  }

  int potValue = total / 8;

  int targetVolume = map(potValue, 0, 1023, 0, 30);

  // Only change volume when the knob has moved
  // at least 2 levels away from the current volume.
  if (abs(targetVolume - volume) >= 2) {

    volume = targetVolume;

    player.volume(volume);

    Serial.print("Volume: ");
    Serial.println(volume);

    updateDisplay();
  }
}


// ---------------- SETUP ----------------

void setup() {

  Serial.begin(115200);
  Serial1.begin(9600);


  // Buttons
  pinMode(playPauseButton, INPUT_PULLUP);
  pinMode(nextButton, INPUT_PULLUP);
  pinMode(previousButton, INPUT_PULLUP);
  pinMode(shuffleButton, INPUT_PULLUP);


  Serial.println("Starting MP3 Player...");


  // DFPlayer
  if (!player.begin(Serial1)) {

    Serial.println("DFPlayer connection failed!");

    while (true);
  }

  Serial.println("DFPlayer connected!");


  // Starting volume
  player.volume(volume);


  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED not found!");

    while (true);
  }


  // Start first track
  startTrack(1);


  // Seed random number generator
  randomSeed(micros());


  Serial.println("Play/Pause button ready.");
  Serial.println("Next button ready.");
  Serial.println("Previous button ready.");
  Serial.println("Shuffle button ready.");
}


// ---------------- LOOP ----------------

void loop() {



  // ---------------- VOLUME KNOB ----------------

  updateVolumeFromKnob();



  // ---------------- DFPLAYER EVENTS ----------------

  if (player.available()) {

    uint8_t type = player.readType();

    int value = player.read();


    if (type == DFPlayerPlayFinished) {

      Serial.print("Track ");
      Serial.print(value);
      Serial.println(" finished.");


      // Ignore duplicate/stale finish events
      // immediately after changing tracks
      if (millis() - lastTrackChangeTime <
          finishIgnoreTime) {

        return;
      }


      // Only advance if the reported track
      // is the track we are currently playing
      if (value == currentTrack) {

        if (shuffleEnabled) {

          nextTrack();

        }
        else {

          currentTrack++;

          if (currentTrack > TOTAL_SONGS) {
            currentTrack = 1;
          }

          startTrack(currentTrack);
        }
      }
    }
  }


  // ---------------- PLAY / PAUSE BUTTON ----------------

  if (digitalRead(playPauseButton) == LOW) {

    togglePlayPause();

    while (digitalRead(playPauseButton) == LOW) {
      delay(10);
    }

    delay(50);
  }


  // ---------------- NEXT BUTTON ----------------

  if (digitalRead(nextButton) == LOW) {

    nextTrack();

    while (digitalRead(nextButton) == LOW) {
      delay(10);
    }

    delay(50);
  }


  // ---------------- PREVIOUS BUTTON ----------------

  if (digitalRead(previousButton) == LOW) {

    previousTrack();

    while (digitalRead(previousButton) == LOW) {
      delay(10);
    }

    delay(50);
  }


  // ---------------- SHUFFLE BUTTON ----------------

  if (digitalRead(shuffleButton) == LOW) {

    toggleShuffle();

    while (digitalRead(shuffleButton) == LOW) {
      delay(10);
    }

    delay(50);
  }


  // ---------------- UPDATE PROGRESS ----------------

  static unsigned long lastDisplayUpdate = 0;


  if (millis() - lastDisplayUpdate >= 250) {

    lastDisplayUpdate = millis();

    updateDisplay();
  }


  // ---------------- SERIAL COMMANDS ----------------

  if (Serial.available()) {

    char command = Serial.read();


    // Play / Pause
    if (command == 'p') {

      togglePlayPause();
    }


    // Next
    else if (command == 'n') {

      nextTrack();
    }


    // Previous
    else if (command == 'b') {

      previousTrack();
    }


    // Shuffle
    else if (command == 's') {

      toggleShuffle();
    }


    // Volume up
    else if (command == '+') {

      if (volume < 30) {

        volume++;

        player.volume(volume);

        Serial.print("Volume: ");
        Serial.println(volume);

        updateDisplay();
      }
    }


    // Volume down
    else if (command == '-') {

      if (volume > 0) {

        volume--;

        player.volume(volume);

        Serial.print("Volume: ");
        Serial.println(volume);

        updateDisplay();
      }
    }
  }
}