# scenecutremover
A Video Scene Cut Remover

# Build
```
mkdir build 
cd build
cmake ..
make
```

# Use

To just remove the scenes without audio:
```
./scenecutremover in.mp4 out.mp4
```

For ease use the script:
```
./filter.sh video.mp4
```

Will output a video_filter.mp4 with the audio.
