# FaceOff
This is an attempt to recreate Michael Zolloeffer's work from Part I of his paper Zollhöfer, M. 2015, 'Real-Time Reconstruction of Static and Dynamic Scenes', pp. 1–196.

The paper describes a method for constructing personal avatars from Kinect RGB-D scans using a 3D Morphable Face Model
It's also an attempt by me to get to grips with some fundamental CV toolkits including OpenCV and PCL


## Dependencies
FaceOff uses OpenCV (built with version 2.4) for manipualting data and OpenNI for capturing Kinect input RGB-D frames.

## Building
Build using CMake

## Running
Currently launching the FaceOff binary will open a window and display the captured RGB image.  If there's a face present (occupying at least 30% of the width/height of the display) it will be outlined with a blue box and left and right eyes will be identified with green boxes.
