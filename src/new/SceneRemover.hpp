#include "opencv2/opencv.hpp"
#include <iostream>

#define BUFSIZE 180
#define SC_N_DIFFS 5

using namespace std;
using namespace cv;


typedef vector<Mat> Framebuf;
typedef vector<int> Framelog;
typedef struct _Scenechange {
	int n_diffs;
	double diffs[SC_N_DIFFS];
	int count;
} Scenechange;

class SceneRemover{
	Framebuf buf;
	Framelog cuts;
	Framelog scenelength;
	int nframes;
	int bufsize;
	int lastcut;
	double fps;
	VideoCapture *cap;
	VideoWriter *out;
	Scenechange sc;

	SceneRemover(VideoCapture *capture, int _bufsize = BUFSIZE);
	double getFrameScore (Mat* f1, Mat* f2);
	bool isChange(Mat *oldframe, Mat *frame, Scenechange *sc);
	int fillBuffer();
	int detectScenes();
	int deleteScenes();
	void filter();
	void deleteScene(int first, int last);
public:
	mutex bufMutex;
	void start();

};
