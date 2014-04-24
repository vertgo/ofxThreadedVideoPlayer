//
//  ofxThreadedVideoPlayer.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 04/03/14.
//
//

#ifndef BaseApp_ofxThreadedVideoPlayer_h
#define BaseApp_ofxThreadedVideoPlayer_h

#include "ofMain.h"
#include "ofxAVFVideoPlayerExtension.h"

class ofxThreadedVideoPlayer;

struct ofxThreadedVideoPlayerStatus{
	ofxThreadedVideoPlayer* player;
	bool ready;
	string path;
	ofxThreadedVideoPlayerStatus(){ ready = true; player = NULL; }
};

class ofxThreadedVideoPlayer: public ofThread{

public:

	ofxThreadedVideoPlayer();
	~ofxThreadedVideoPlayer();

	void loadVideo(string path);
	void play();
    //added by mike
    void play( float curTime );
	void stop();

	void setLoopMode(ofLoopType loop);
	bool hasFinished();

	void setPosition(float percent);
    //added by mike
    void setPaused(bool inPaused);
    void setVolume( float inVolume);
    void setSpeed( float speed);
    
    
	float getPosition();
	float getDuration();

	bool isReadyForPlayback();

	ofTexture* getTexture();

	void draw(float x, float y, bool drawDebug = false);
	void draw(float x, float y, float w, float h);
	void drawDebug(float x, float y);
	void update();
    //added by mike
    void syncToPlayhead( float playHead); //in seconds


	float getWidth();
	float getHeight();
    
    //debug only
    float totalOffset;
	//public ofEvent api
	//call ofAddListener(v->videoIsReadyEvent, this, &testApp::videoIsReadyCallback);
	//to get notified when the video is ready for playback
	ofEvent<ofxThreadedVideoPlayerStatus>	videoIsReadyEvent;
    bool isPlaying();

private:

	void threadedFunction();

	string									videopPath;
	ofLoopType								loopMode;
	bool									loadNow;
	bool									playNow;
	bool									stopNow;
	bool									loaded;
	ofxAVFVideoPlayerExtension *			player;
	bool									destroying; //if true, a destructor call has been issued
	bool									readyForPlayback;



	bool									needToNotifyDelegate;
};


#endif
