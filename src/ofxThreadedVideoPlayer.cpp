//
//  ofxThreadedVideoPlayer.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 04/03/14.
//
//

#include "ofxThreadedVideoPlayer.h"

ofxThreadedVideoPlayer::ofxThreadedVideoPlayer(){
	loadNow = playNow = stopNow = false;
	player = NULL;
	loaded = false;
	loopMode = OF_LOOP_NORMAL;
	needToNotifyDelegate = false;
	destroying = false;
	readyForPlayback = false;
    //debug only
    totalOffset = 0.f;
}

ofxThreadedVideoPlayer::~ofxThreadedVideoPlayer(){
	destroying = true;
	cout << "~ofxThreadedVideoPlayer()" << endl;
	stop();
	waitForThread();
	delete player;
}

void ofxThreadedVideoPlayer::loadVideo(string path){
	videopPath = path;
	loadNow = true;
	if(!isThreadRunning()) startThread();
        totalOffset = 0.f;
}

void ofxThreadedVideoPlayer::play(){
	playNow = true;
	if(!isThreadRunning()) startThread();
}

//added by Mike
void ofxThreadedVideoPlayer::play( float curTime){
    player->setPositionInSeconds(curTime);
	play();
}

void ofxThreadedVideoPlayer::stop(){
	stopNow = true;
}

void ofxThreadedVideoPlayer::setLoopMode(ofLoopType loop){
	loopMode = loop;
	if(player){
		player->setLoopState(loop);
	}
}


bool ofxThreadedVideoPlayer::isReadyForPlayback(){
	return readyForPlayback;
}


void ofxThreadedVideoPlayer::threadedFunction(){

	while(isThreadRunning()){

		if (loadNow){	//////////////////////////// LOAD
			lock();
				if(player){
					player->stop();
					delete player;
				}
				player = new ofxAVFVideoPlayerExtension();
				loaded = player->loadMovie(videopPath);
				needToNotifyDelegate = true;
				player->setLoopState(loopMode);
			unlock();
			loadNow = false;
		}

		if (stopNow){	/////////////////////////// STOP
			if(loaded){
				lock();
				player->setPaused(true);
				unlock();
			}else{
				cout << "can't stop playing before we load a movie!" << endl;
			}
			stopNow = false;
			stopThread();
		}

		if (playNow){	///////////////////////////// PLAY
			if(loaded){
				lock();
				player->setPaused(false);
				player->play();
				unlock();
			}else{
				cout << "can't play before we load a movie!" << endl;
			}
			playNow = false;
		}

		if(player){
			if(player->isReallyLoaded()){
				lock();
				player->update();
				unlock();
			}
		}

		ofSleepMillis(1); //mm todo!

		if (player && !destroying){
			if(player->getIsMovieDone() && loopMode == OF_LOOP_NONE){
				//lock();
				//player->stop();
				//unlock();
				stopThread();
			}
		}
	}
}

bool ofxThreadedVideoPlayer::hasFinished(){
	bool ret = false;
	lock();
	ret = player->getIsMovieDone();
	unlock();
	return ret;
}

bool ofxThreadedVideoPlayer::isPlaying(){
	bool ret = false;
	lock();
	ret = player->isPlaying();
	unlock();
	return ret;
}

void ofxThreadedVideoPlayer::update(){

	lock();
	if(player){
		bool reallyLoaded = player->isReallyLoaded();
		ofTexture * tex = player->getTexture();

		if( reallyLoaded && tex){

			if(needToNotifyDelegate){ //notify our delegate from the main therad, just in case (draw() always called from main thread)
				ofxThreadedVideoPlayerStatus status;
				status.path = videopPath;
				status.player = this;
				ofNotifyEvent( videoIsReadyEvent, status, this );
				needToNotifyDelegate = false;
				readyForPlayback = true;
			}
		}
	}
	unlock();

}

void ofxThreadedVideoPlayer::draw(float x, float y, float w, float h){

	if(player && loaded){
		lock();
		bool reallyLoaded = player->isReallyLoaded();
		ofTexture * tex = player->getTexture();

		if( reallyLoaded && tex){
			tex->draw(x,y, w, h ); //doing this instead if drawing the player directly to avoid 2 textureUpdate calls (one to see if texture is there, one to draw)
		}
		unlock();
	}
}


void ofxThreadedVideoPlayer::draw(float x, float y, bool drawDebug){

	if(player && loaded){

		lock();
		bool reallyLoaded = player->isReallyLoaded();
		ofTexture * tex = player->getTexture();

		if( reallyLoaded && tex){
			tex->draw(x,y, player->getWidth(), player->getHeight() ); //doing this instead if drawing the player directly to avoid 2 textureUpdate calls (one to see if texture is there, one to draw)
		}
		unlock();
	}
}

void ofxThreadedVideoPlayer::drawDebug(float x, float y){
	string debug =	"isThreadRunning: " + ofToString(isThreadRunning()) + "\n" +
	"loadNow: " + ofToString(loadNow) + "\n" +
	"playNow: " + ofToString(playNow) + "\n" +
	"stopNow: " + ofToString(stopNow) + "\n" +
	"hasFinished: " + ofToString(hasFinished()) + "\n" +
	"position: " + ofToString(getPosition()) + "\n" +
	"loop: " + string(loopMode == OF_LOOP_NONE ? "NO" : "YES") + "\n";
	ofDrawBitmapString(debug, x + 25, y + 55);
}

float ofxThreadedVideoPlayer::getWidth(){
	if(player){
		return player->getWidth();
	}
	return 0;
}


float ofxThreadedVideoPlayer::getHeight(){
	if(player){
		return player->getHeight();
	}
	return 0;
}


void ofxThreadedVideoPlayer::setPosition(float percent){
	if(player){
		lock();
		player->stop();
		player->play();
		player->setPosition( ofClamp(percent,0.0f,1.0f) );
		unlock();
		if(!isThreadRunning()) startThread();
	}
}

//added by mike
void ofxThreadedVideoPlayer::setSpeed(float speed){
	if(player){
		lock();
		player->setSpeed( speed );
		unlock();
		if(!isThreadRunning()) startThread();
	}
}

//added by Mike
void ofxThreadedVideoPlayer::syncToPlayhead(float playHead){//in in seconds
    //unsigned long long playHead = ofGetSystemTime() - vidStartTime;
    if(player){
        lock();
        if (!player->isLoaded()){
            unlock();
            return;
        }
        float curVidTime = (player->getPosition() * player->getDuration() );
        float curOffset = (playHead -curVidTime);
        float adjustedSpeed = CLAMP( 1.f + ( curOffset )/1, .1f, 1.9f);
        //
        float curSpeed = player->getSpeed();
        
        /*if ( abs(curOffset) >1.9f ){
            cout << "jumping playhead\n";
            player->setSpeed(1.f);
            player->setPositionInSeconds(playHead + .5f); //set it to the time at next frame and see what happens then
        }*/
        
        //if they are both the same sign (like readjust when it crosses from having to speed up to slow down)
        //and the are pretty close, then don't readjust
        /*else*/ if ( (adjustedSpeed * curSpeed > 0) && abs( adjustedSpeed - curSpeed ) > 0.05f ){
            cout << "adjusting speed\n";
            player->setSpeed( adjustedSpeed ); //maybe setting the playspeed too much slows it down
        }
        
        unlock();
        totalOffset+= abs( curOffset );
        //cout << "playhead:"<<playHead<< "curVidTime:" << curVidTime << ", curOffset:" << curOffset << ", adjustedSpeed:" << adjustedSpeed << ", getSpeed:"<<player->getSpeed()<<endl;
    }
    
}

float ofxThreadedVideoPlayer::getPosition(){
	if(player){
		lock();
		float p = player->getPosition();
		unlock();
		return p;
	}
	return 0;
}


float ofxThreadedVideoPlayer::getDuration(){
	if(player){
		lock();
		float d = player->getDuration();
		unlock();
		return d;
	}
	return 0;
}
