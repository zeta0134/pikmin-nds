#!/bin/sh

# run the dockerfile on this directory, which will build the project
docker run -it --rm -v $(pwd):/source pikmin-nds bash
