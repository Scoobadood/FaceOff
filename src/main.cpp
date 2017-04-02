
#include <iostream>
#include <OpenNI.h>
#include <opencv2/highgui.hpp>

#include "FaceOffConfig.h"

const int SAMPLE_READ_WAIT_TIMEOUT = 2000; // ms

/** 
 * Setup and initialise openNI
 * or fail
 */
int initializeDepthCamera( openni::Device& device, openni::VideoStream& depth, openni::VideoStream& colour ) {
	using namespace openni;

	Status rc = OpenNI::initialize();
	if (rc != STATUS_OK) {
		std::cout << "Initialize failed: " << OpenNI::getExtendedError() << std::endl;
		return 1;
	}

	rc = device.open(ANY_DEVICE);
	if (rc != STATUS_OK) {
		std::cout << "Couldn't open device: " << OpenNI::getExtendedError() << std::endl;
		return 2;
	}

	if (device.getSensorInfo(SENSOR_DEPTH) != NULL) {
		rc = depth.create(device, SENSOR_DEPTH);
		if (rc == STATUS_OK) {
			rc = depth.start();
			if (rc != STATUS_OK) {
				std::cout << "Couldn't start the depth stream: " << OpenNI::getExtendedError() << std::endl;
			}
		}
		else {
			std::cout << "Couldn't create depth stream: " << OpenNI::getExtendedError() << std::endl;
		}
	}

	if (device.getSensorInfo(SENSOR_COLOR) != NULL) {
		rc = colour.create(device, SENSOR_COLOR);
		if (rc == STATUS_OK) {
			rc = colour.start();
			if (rc != STATUS_OK) {
				std::cout << "Couldn't start the colour stream: " << OpenNI::getExtendedError() << std::endl;
			}
		}
		else {
			std::cout << "Couldn't create colour stream: " << OpenNI::getExtendedError() << std::endl;
		}
	}

	return 0;
}

/**
 * Finish with the device, close down stream etc
 */
void finalizeDevice( openni::Device& device, openni::VideoStream& depth, openni::VideoStream& colour ) {

	depth.stop();
	colour.stop();
	depth.destroy();
	colour.destroy();
	device.close();
	openni::OpenNI::shutdown();
}

/**
 * Get a pair of frames; depth and RGB which are close together in time
 * @return -1 If no frame returned, 0 for depth and 1 for colour
 */
int getFrame( openni::VideoStream& depth, openni::VideoStream& colour, openni::VideoFrameRef& frame) {
	using namespace openni;

	int readyStream = -1;
	VideoStream * streams[] = { &depth, &colour };
	Status rc = OpenNI::waitForAnyStream(streams, 2, &readyStream, SAMPLE_READ_WAIT_TIMEOUT);
	if (rc != STATUS_OK) {
		std::cout << "Wait failed! (timeout is " << SAMPLE_READ_WAIT_TIMEOUT << " ms): " << OpenNI::getExtendedError() << std::endl;
	} 

	// Status was OK, we got a frame
	else {
		switch (readyStream) {
			case 0:
				// Depth
				depth.readFrame(&frame);
				break;

			case 1:
				// Colour
				colour.readFrame(&frame);
				break;

			default:
				std::cerr << "Unxpected stream: " << readyStream << std::endl;
		}
	}

	return readyStream;
}


int main( int argc, char *argv[] ) {
	using namespace openni;

	std::cout << "FaceOff v" << VERSION_MAJOR << "." << VERSION_MINOR << std::endl;
	std::cout << "hit a key to quit" << std::endl;

	// Set up the capture device
	Device device;
	VideoStream colour, depth;
	if( initializeDepthCamera( device, depth, colour ) != 0 ) {
		return 1;
	}


	VideoFrameRef frame;

	// Create a window to display RGB feed
	cv::namedWindow( "RGB" );

	int numDepthFrames = 0;
	int numColourFrames= 0;
	while ( cv::waitKey(10) == -1 )  {

		int frameType = getFrame( depth, colour, frame );

		// Depth frame
		if( frameType == 0 ) {
			// Register this to colour


			// Process frame
			numDepthFrames++;
		} 

		else if ( frameType == 1 ) {
			// Render this to screen
			numColourFrames++;
		}

		std::cout << "\rDepth frames : " << numDepthFrames << "  Colour frames : " << numColourFrames << std::flush;
	}

	std::cout << std::endl;

	cv::destroyWindow( "RGB" );

	// Shutdown the device
	finalizeDevice( device, depth, colour );
	return 0;
}
