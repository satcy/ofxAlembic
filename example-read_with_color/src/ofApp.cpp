#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetVerticalSync(true);
    ofSetFrameRate(60);
    ofBackground(200);
    
    string path = "mountain.abc";
    
    // load allembic file
    abc.open(path);
    
    // show all drawable names
    abc.dumpFullnames();
}
void ofApp::exit()
{
    abc.close();
}
//--------------------------------------------------------------
void ofApp::update(){
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofEnableDepthTest();
    cam.begin();
    
    glPointSize(4);
    
    
    float t = fmod(ofGetElapsedTimef(), abc.getMaxTime());
    abc.setTime(t);
    
    {
        ofMesh mesh;
        abc.get("/pointwrangle1", mesh);
//        cout << mesh.getNumVertices() << endl;
//        cout << mesh.getNumNormals() << endl;
//        cout << mesh.getNumIndices() << endl;
//        cout << mesh.getNumColors() << endl;
//        cout << mesh.getNumTexCoords() << endl;
//        cout << "*****" << endl;
        ofSetColor(255);
        ofPushMatrix();
        ofScale(100, 100, 100);
        mesh.draw();
        ofPopMatrix();
    }
    
    // or simply, abc.draw();
    
    cam.end();
    
    ofSetColor(255);
    
    ofDrawBitmapString(ofToString(abc.getTime()) + "/" + ofToString(abc.getMaxTime()), 10, 20);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
