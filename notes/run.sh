#!/bin/sh
while [ 1 ]; do
    markdown_py coffee-script.md  > coffee-script.html
    inotifywait -r -q -e modify .
    # Sync takes some time, wait to avoid races.
    sleep 0.1
done
