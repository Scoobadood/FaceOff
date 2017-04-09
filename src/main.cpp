
#include <iostream>
#include <OpenNI.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include "FaceOffConfig.h"

const int SAMPLE_READ_WAIT_TIMEOUT = 2000; // ms

const float FACE_SIZE_RATIO = 0.25f;
const float EYE_SIZE_RATIO  = 0.25f;
const float EYE_REGION_HEIGHT_PROPORTION = 0.6f;
const cv::Scalar BLUE( 255, 0, 0 );
const cv::Scalar GREEN( 0, 255, 0 );

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

/**
 * @return the path to the directory containing this executable file
 */
int getPathToExe( char * buffer, int buffer_length ) {
	char szTmp[32];
	sprintf(szTmp, "/proc/%d/exe", getpid());
	int bytes = MIN(readlink(szTmp, buffer, buffer_length), buffer_length - 1);
	if(bytes >= 0)
    	buffer[bytes] = '\0';

    // Now search from the end of the string to find the last file separator
    while( (--bytes > 0 ) && buffer[bytes] != '/');
    if( buffer[bytes] == '/') {
    	buffer[bytes] = '\0';
    } else {
    	bytes = strlen( buffer );
    }

	return bytes;
}

/*
 * Load Haar classifiers for face and eyes
 */
bool loadHaarClassifiers( cv::CascadeClassifier& face_classifier, cv::CascadeClassifier& left_eye_classifier, cv::CascadeClassifier& right_eye_classifier ) {
	// Work out where this code is running from
	char exe_dir[1024];
	getPathToExe( exe_dir, 1024 );

	std::string base( exe_dir );
	base += "/data";

	// Classifiers should be in a data subdirectory
	std::cout << "Loading haar cascades from " << base << std::endl;

	bool loaded_ok = true;
	loaded_ok &= face_classifier.load( base + "/haarcascade_frontalface_default.xml" );
	loaded_ok &= left_eye_classifier.load( base + "/haarcascade_lefteye_2splits.xml" );
	loaded_ok &= right_eye_classifier.load( base + "/haarcascade_righteye_2splits.xml" );

	return loaded_ok;
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


	// Load classifiers
	cv::CascadeClassifier faceClassifier, leftEyeClassifier, rightEyeClassifier;
	if( !loadHaarClassifiers( faceClassifier, leftEyeClassifier, rightEyeClassifier ) ) {
		std::cout << "Failed to load classfiers" << std::endl;
		return -1;
	}



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
			// Grab pointer to data in appropriate format
			const RGB888Pixel* imageBuffer = (const openni::RGB888Pixel*)frame.getData();

			// Create a Mat
			cv::Mat rgbImage;
			rgbImage.create( frame.getHeight(), frame.getWidth(), CV_8UC3 );
			memcpy( rgbImage.data, imageBuffer, 3 * frame.getHeight()*frame.getWidth()*sizeof(uint8_t) );

			// Manage BGR to RGB conversion
			cv::cvtColor( rgbImage, rgbImage, CV_BGR2RGB);

			// Make a grey version for face detection
			cv::Mat greyImage( frame.getHeight(), frame.getWidth(), CV_8UC1 );
			cv::cvtColor( rgbImage, greyImage, CV_RGB2GRAY);


			// Find the face
			std::vector<cv::Rect> faces;
			cv::Size desiredSize( frame.getWidth() * FACE_SIZE_RATIO, frame.getHeight() * FACE_SIZE_RATIO );
			faceClassifier.detectMultiScale( greyImage, faces, 1.1, 3, 0, desiredSize );

			// Render if there is one
			if( faces.size() > 0 ) {
				// Render face box into image
				cv::rectangle( rgbImage, faces[0], BLUE, 3 );

				// And find eyes
				cv::Rect eyeRegion( faces[0].x, faces[0].y, faces[0].width, faces[0].height * EYE_REGION_HEIGHT_PROPORTION );
				cv::Mat eyeROI = greyImage( eyeRegion );
				cv::Size desiredSize( eyeRegion.width * EYE_SIZE_RATIO, eyeRegion.width * EYE_SIZE_RATIO );

				std::vector<cv::Rect> leftEyes;
				leftEyeClassifier.detectMultiScale( eyeROI, leftEyes, 1.1, 2, 0, desiredSize );
				if( leftEyes.size() > 0 ) {
					leftEyes[0].x += faces[0].x;
					leftEyes[0].y += faces[0].y;
					cv::rectangle( rgbImage, leftEyes[0], GREEN, 2 );
				}

				std::vector<cv::Rect> rightEyes;
				rightEyeClassifier.detectMultiScale( eyeROI, rightEyes, 1.1, 2, 0, desiredSize );
				if( rightEyes.size() > 0 ) {
					rightEyes[0].x += faces[0].x;
					rightEyes[0].y += faces[0].y;
					cv::rectangle( rgbImage, rightEyes[0], GREEN, 2 );
				}
			}


			// Render
			cv::imshow( "RGB", rgbImage );

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
