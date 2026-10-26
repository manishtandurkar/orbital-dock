#!/usr/bin/env bash
# Records the scripted demo mission to an MP4 (macOS and Linux; needs ffmpeg).
# The game streams raw frames into a named pipe that ffmpeg encodes on the fly,
# and writes its own sound mix to audio.wav, which is muxed in at the end.
#
# Usage: tools/record_demo.sh [output.mp4]
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
output="$(cd "$(dirname "${1:-media/orbital_dock_demo.mp4}")" && pwd)/$(basename "${1:-media/orbital_dock_demo.mp4}")"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

mkfifo "$work/video.fifo"
"$repo/build/orbital_dock" --record-demo "$work" &
game=$!

while [ ! -s "$work/video_size.txt" ]; do
    if ! kill -0 "$game" 2>/dev/null; then
        echo "The game exited before recording started" >&2
        exit 1
    fi
    sleep 0.2
done
read -r width height < "$work/video_size.txt"
echo "Capturing ${width}x${height} at 60 fps"
scale="scale=${width}:${height}"
if [ "$height" -lt 1080 ]; then
    scale="scale=1920:1080:flags=lanczos"
fi

ffmpeg -hide_banner -loglevel error -y \
    -f rawvideo -pixel_format rgb24 -video_size "${width}x${height}" -framerate 60 -i "$work/video.fifo" \
    -vf "vflip,${scale},format=yuv420p" \
    -c:v libx264 -preset slow -crf 16 -tune animation -movflags +faststart "$work/video.mp4"
wait "$game"

ffmpeg -hide_banner -loglevel error -y -i "$work/video.mp4" -i "$work/audio.wav" \
    -c:v copy -c:a aac -b:a 192k -shortest -movflags +faststart "$output"
echo "Wrote $output"
