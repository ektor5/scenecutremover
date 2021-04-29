#include "opencv2/opencv.hpp"
#include <iostream>

#define BUFSIZE 200
#define THRESHOLD 1.5
#define SC_N_DIFFS 5
using namespace std;
using namespace cv;

typedef Vec<bool,BUFSIZE> Scenes;
typedef vector<Mat> Framebuf;
typedef vector<int> Framelog;

typedef struct _Scenechange 
{
  int n_diffs;
  double diffs[SC_N_DIFFS];
  int count;
} Scenechange;

double get_frame_score (Mat* f1, Mat* f2) {
  int score = 0;
  int width, height;

  width = f1->rows;
  height = f1->cols;

  Mat(*f1 - *f2).forEach<int>([&score](int &p, const int * position) -> void {
    score += abs(p);
  });

  return ((double) score) / (width * height);
}


bool is_change(Mat *oldframe, Mat *frame, Scenechange *sc){
	bool change;
	double threshold;
	double score;
	double score_min;
	double score_max;

	score = get_frame_score (oldframe, frame);

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

void remove_fadeblack(Framebuf *buf, int start, int end){
	int size = end - start;
	int center = size/2;
	int a = start % BUFSIZE;
	int b = end % BUFSIZE;
	cout << "fadeblack" << "\n";
	for (int i=start; i != end ; i++){
		if (i<center+start){
			double perc = 1-(i-start)/(static_cast<double>(center));
			//cout << i << " "  << std::fixed << std::setprecision(3) << perc << "\n";
			(*buf)[i%BUFSIZE] = (*buf)[a]*perc;
		}else{
			double perc = 1-(end-i)/(static_cast<double>(center));
			//cout << i << " " << std::fixed << std::setprecision(3) << perc << "\n";
			(*buf)[i%BUFSIZE] = (*buf)[b]*perc;
		}
	}
}

void remove_freeze(Framebuf *buf, int start, int end){
	int size = end - start;
	int center = size/2;
	int a = start % BUFSIZE;
	int b = end % BUFSIZE;
	/*freeze*/
	cout << "freeze" << "\n";
	for (int i=start; i != end ; i++){
		double perca = 1-(i-start)/(static_cast<double>(size));
		double percb = 1-perca;
		//cout << i-start << " "  << std::fixed << std::setprecision(3) << perc << "\n";
		(*buf)[i%BUFSIZE] = (*buf)[a]*perca+(*buf)[b]*percb;
		//cout << end-i << " "  << std::fixed << std::setprecision(3) << perc << "\n";
	}
}

void remove_stretch(Framebuf *buf, int start, int end){
	int size = end - start;
	int center = size/2;
	cout << "stretch" << "\n";

	Mat tmp[2*size];
	int newstart = start - center;
	for (int i=start-center; i != end+center; i++){
		if (i-newstart < size){
			int bufa = (newstart+(i-newstart)/2);
			cout << bufa << "\t a-> \t"<< i  << "\n";
			tmp[i-newstart] = (*buf)[bufa%BUFSIZE];
		}else{
			double percb = (i-newstart-size)/(static_cast<double>(size));
			int bufb = (end+(i-newstart-size)/2);
			cout << bufb << "\t b-> \t"<< i  << "\n";
			tmp[i-newstart] = (*buf)[bufb%BUFSIZE];
		}
	} 
	for (int i=start-center; i != end+center; i++){
		int bufi = i%BUFSIZE;
		(*buf)[bufi] = tmp[i-newstart];
	}
}

void remove_crossfade(Framebuf *buf, int start, int end ){
	int size = end - start;
	int center = size/2;
	int newstart = start - size;
	int newend = end + size;
	Mat tmp[3*size];
	cout << "crossfade" << "\n";
	for (int i=newstart; i != newend; i++){
		double perca = 1-(i-newstart)/(static_cast<double>(size*3));
		double percb = 1-perca;
		int bufi = i%BUFSIZE;
		int bufa = (newstart+(i-newstart)/3)%BUFSIZE;
		int bufb = (end+(i-newstart)/3)%BUFSIZE;
		cout << bufa << "\t a-> \t" << i<< " "  << std::fixed << std::setprecision(3) << perca << "\n";
		cout << bufb << "\t b-> \t" << i<< " "  << std::fixed << std::setprecision(3) << percb << "\n";
		tmp[i-newstart] = (*buf)[bufa]*perca + (*buf)[bufb]*percb ;
	}
	cout << "copying tmp" << "\n";
	for (int i=newstart; i != newend; i++){
		int bufi = i%BUFSIZE;
		(*buf)[bufi] = tmp[i-newstart];
	}
}

enum transitions : int {
	FADEBLACK,
	FREEZE,
	CROSSFADE,
	STRETCH
};

typedef struct _LastTransition {

	int trans;
	int b;
	int bf;
	int trigger;
} LastTransition;

int main(int argc, char* argv[]){

	if (argc < 2 ){
		cout << "argument missing" << "\n";
		return 1;
	}

	char* filename = argv[1];
	char* outputname = argv[2];

	// Create a VideoCapture object and open the input file
	// If the input is the web camera, pass 0 instead of the video file name
	VideoCapture cap(filename); 
	double fps = cap.get(CAP_PROP_FPS);

	// Check if camera opened successfully
	if(!cap.isOpened()){
		cout << "Error opening video stream or file" << endl;
		return -1;
	}
	int ex = static_cast<int>(cap.get(CAP_PROP_FOURCC));
	Size S = Size((int) cap.get(CAP_PROP_FRAME_WIDTH),    // Acquire input size
                  (int) cap.get(CAP_PROP_FRAME_HEIGHT));

	VideoWriter writer;
	writer.open(outputname, ex, fps, S, true);
	if (!writer.isOpened())
	{
		cout  << "Could not open the output video for write: " << outputname << endl;
		return -1;
	}

	double score_min;
	double score_max;
	double threshold;
	double score;
	Scenechange sc;
	Mat oldframe;
	int nframes = 0;
	int lasttrans = -1;

	vector<LastTransition> delayed;

	Scenes v;
	Framebuf buf(BUFSIZE);
	Framelog cuts;

	sc.n_diffs = 0;

		while(1){

			Mat frame;
			int n = nframes%BUFSIZE;

			// Capture frame-by-frame
			cap >> frame;

			// If the frame is empty, break immediately
			if (frame.empty())
				break;

			buf[n] = frame;

			if (nframes) {
				if (!oldframe.empty())
				{
					v[n] = is_change(&frame, &oldframe, &sc);

					int total = sum(v)[0];

					//cout << total << "\n";
			/*	
					if (total > THRESHOLD) {
						cout << cuts.front() << "\n";
						for (int i=cuts.front() ; i<nframes ; i++){
							buf[i%BUFSIZE] = buf[cuts.front()%BUFSIZE];
						}
						cuts.clear();
					}
					*/

					if (v[n]){
						cuts.push_back(nframes);

						if (cuts.size() > 2)
						{
							auto b = cuts.back();
							auto bf = cuts[cuts.size()-2];
							auto diff = b - bf;
							double sec = diff / fps;
							cout << nframes/fps << "\t" << bf << " " << b << " " << diff << "\n";
							if (sec < THRESHOLD ){
								cout << "Short scene detected!" << "\n";

									if (sec > 0.8){
										remove_fadeblack(&buf,bf,b);
										lasttrans = FADEBLACK;
									} else if (sec > 0.5){
										if (lasttrans == STRETCH)
										{
											LastTransition t = { 
												.trans = CROSSFADE,
												.b = b, 
												.bf = bf, 
												.trigger = nframes + diff
											};
											delayed.emplace_back(move(t));
											lasttrans = CROSSFADE;
											cout << "delayed crossfade" << "\n";
										}else{
											LastTransition t = { 
												.trans = STRETCH,
												.b = b, 
												.bf = bf, 
												.trigger = nframes + diff/2
											};
											delayed.emplace_back(move(t));
											lasttrans = STRETCH;
											cout << "delayed stretch" << "\n";
										}
									} else {
										remove_freeze(&buf,bf,b);
										lasttrans = FREEZE;
									}
							}
						}
					}

					if (delayed.size())
						for (auto&& lastt : delayed ){
							if (nframes == lastt.trigger){
								switch (lastt.trans){
									case CROSSFADE: remove_crossfade(&buf,lastt.bf, lastt.b);
										     break;
									case STRETCH: remove_stretch(&buf,lastt.bf, lastt.b);
										     break;
								}
								delayed.pop_back();
							}
						}



					//if (total > THRESHOLD) 
					//	v = 0;

				}
			}

			oldframe = frame;

			nframes++;
			n = nframes%BUFSIZE;

			// Display the resulting frame
			if (nframes > BUFSIZE)
			{
				//imshow( "Frame", buf[n] );
				writer << buf[n];
			}
			//imshow( "Nowframe", frame );

			// Press  ESC on keyboard to exit
			//char c=(char)waitKey(25);
			//if(c==27)
				//break;


		}

		if (nframes > BUFSIZE)
		{

			for ( int i=0 ; i < BUFSIZE; i++){
				//imshow( "Frame", buf[(nframes+i)%BUFSIZE] );
				writer << buf[(nframes+i)%BUFSIZE] ;
				// Press  ESC on keyboard to exit
				//char c=(char)waitKey(25);
				//if(c==27)
				//	break;
			}
		}
		cout << "cuts" << "\n";
		int lastcut = 0;
		for ( auto &i : cuts ) {
			cout << i<< "\t" << i/fps << "\t"<< (i-lastcut)/fps << "\n";
			lastcut = i;
		}

	// When everything done, release the video capture object
	cap.release();

	// Closes all the frames
	destroyAllWindows();

	return 0;
}

