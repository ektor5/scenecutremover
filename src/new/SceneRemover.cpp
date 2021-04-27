#include "SceneRemover.hpp"

using namespace std;
using namespace cv;

SceneRemover::SceneRemover(VideoCapture *capture, int _bufsize) : 
	cap(capture), bufsize(_bufsize) {
	this->fps = cap->get(CAP_PROP_FPS);
	this->lastcut = 0;
}

int SceneRemover::fillBuffer(){
	Mat frame;
	lock_guard<mutex> lockGuard(this->bufMutex);

	if(!cap->isOpened()){
		cout << "Error opening video stream or file" << endl;
		return -1;
	}

	while (buf.size() != bufsize)
	{
		cap->read(frame);
		if (frame.empty()) {
		    cerr << "ERROR! blank frame grabbed\n";
		    return 1;
		}
		buf.push_back(frame);
		nframes++;
	}
	return 0;
}

int SceneRemover::detectScenes()
{
	Mat* lastframe = NULL;
	int nframe = nframes - buf.size();
	lock_guard<mutex> lockGuard(this->bufMutex);

	for ( auto&& frame : buf ) {
		if (lastframe != NULL){
			if(isChange(lastframe, &frame, &sc)){
				cout << "scene detected at " << nframe << "\n";
				cuts.push_back(nframe);
				scenelength.push_back(nframe-lastcut);
			}
		}
		lastcut = nframe;
		lastframe = &frame;
		nframe++;
	}
	return 0;
}

int SceneRemover::deleteScenes(){

	lock_guard<mutex> lockGuard(this->bufMutex);

	for ( auto&& scene : cuts ){


	}

	return 0;


}

void SceneRemover::filter()
{
	while(!fillBuffer()){
	       detectScenes();
	       deleteScenes();
	       //outputBuf();
	}
}


double SceneRemover::getFrameScore (Mat* f1, Mat* f2) {
  int score = 0;
  int width, height;

  width = f1->rows;
  height = f1->cols;

  for ( int i=0 ; i<width; i++){ 
	  for ( int j=0 ; j<width ; j++){
		 score += abs(f1->data[i*width+j] - f2->data[i*width+j]);
	  }
  }

  return ((double) score) / (width * height);
}


bool SceneRemover::isChange(Mat *oldframe, Mat *frame, Scenechange *sc){
	bool change;
	double threshold;
	double score;
	double score_min;
	double score_max;

	score = getFrameScore (oldframe, frame);

	memmove (sc->diffs, sc->diffs + 1, sizeof (double) * (SC_N_DIFFS - 1));

	sc->diffs[SC_N_DIFFS - 1] = score;
	sc->n_diffs++;

	score_min = sc->diffs[0];
	score_max = sc->diffs[0];
	for (int i = 1; i < SC_N_DIFFS - 1; i++) {
		score_min = MIN (score_min, sc->diffs[i]);
		score_max = MAX (score_max, sc->diffs[i]);
	}
	threshold = 1.8 * score_max - 0.8 * score_min;


	if (sc->n_diffs > (SC_N_DIFFS - 1)) {
		if (score < 5) {
			change = false;
		} else if (score / threshold < 1.0) {
			change = false;
		} else if ((score > 30)
				&& (score / sc->diffs[SC_N_DIFFS - 2] > 1.4)) {
			change = true;
		} else if (score / threshold > 2.3) {
			change = true;
		} else if (score > 50) {
			change = true;
		} else {
			change = false;
		}
	} else {
		change = false;
	}

	if (change == true) {
//		cout<< score<< "\t" << threshold << "\t" << score/threshold << "\n";
		memset (sc->diffs, 0, sizeof (double) * SC_N_DIFFS);
		sc->n_diffs = 0;
	}

	return change;

}

