#!/usr/bin/env bash

function main() {
  rm -rf /tmp/dsserver
  mkdir /tmp/dsserver
#  mkfifo /tmp/dsserver/input

  while [ ! -p /tmp/dsserver/screen.bgra ]; do sleep 1; done

  ffmpeg -y -f rawvideo -pix_fmt bgra -s:v 256x384 -r 60 -i /tmp/dsserver/screen.bgra -tune zerolatency -c:v libx264 screen.mp4 &
  FFMPEG_VIDEO_PID=$!

  while [ ! -p /tmp/dsserver/audio.pcm ]; do sleep 1; done

  ffmpeg -y -f s16le -ar 32.768k -ac 2 -i /tmp/dsserver/audio.pcm -b:a 128k audio.aac &
#  cat /tmp/dsserver/audio.pcm > /dev/null
  FFMPEG_AUDIO_ID=$!

#  printf "" > /tmp/dsserver/input

  sleep 30

  kill $FFMPEG_AUDIO_ID
  kill $FFMPEG_VIDEO_PID
}

main
exit