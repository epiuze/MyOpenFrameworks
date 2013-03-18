/******
 
 This example updates 1M particles on the GPU using OpenCL
 The OpenCL kernel writes position data directly to a VBO stored in the OpenGL device memory
 so now data transfer between host and device during runtime
 
 Based on Rui's ofxOpenCL particle example opencl particles 001b.zip 
 at http://code.google.com/p/ruisource/ 
 *****/


#include "testApp.h"
#include "MSAOpenCL.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <sstream>
//#include <iomanip>
//#include <math.h>
//#include <vector>
//#include <string>

#define USE_OPENGL_CONTEXT


#define NUM_PARTICLES (1000*1000)
#define POINT_SIZE 2
const float CALPHA = 0.1f;

typedef struct{
	float2 vel;
	float mass;
//	float4 col;
    float alpha;
    
    // need this to make sure the float2 vel is aligned to a 16 byte boundary
//	float dummy;
} Particle;


float2				mousePos;
float2				dimensions;

msa::OpenCL			opencl;
msa::OpenCLKernel	*kernelUpdate;


Particle			particles[NUM_PARTICLES];
msa::OpenCLBuffer	clMemParticles;		// stores above data


float2				particlesPos[NUM_PARTICLES];
msa::OpenCLBuffer	clMemPosVBO;		// stores above data

float4				particlesCol[NUM_PARTICLES];
//msa::OpenCLBuffer	clMemColVBO;		// stores above data

GLuint				vbo[1];
GLuint				cvbo[1];


//--------------------------------------------------------------
void testApp::setup(){
	ofBackground(0, 0, 0);
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetVerticalSync(false);
	
#ifdef USE_OPENGL_CONTEXT
	opencl.setupFromOpenGL();
#else	
	opencl.setup(CL_DEVICE_TYPE_CPU, 2);
#endif	
	
	for(int i=0; i<NUM_PARTICLES; i++) {
		Particle &p = particles[i];
		p.vel.set(0, 0);
		p.mass = ofRandom(0.5, 1);
        p.alpha = CALPHA;
        
		particlesPos[i].set(ofRandomWidth(), ofRandomHeight());
        particlesCol[i].set(ofRandom(1), ofRandom(1), ofRandom(1), CALPHA);
	}
	
    /*
     * Create the VBOs
     */
    
    // Position buffer
	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float2) * NUM_PARTICLES, particlesPos, GL_DYNAMIC_COPY);

    // Color buffer
	glGenBuffers(1, cvbo);
	glBindBuffer(GL_ARRAY_BUFFER, cvbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float4) * NUM_PARTICLES, particlesCol, GL_DYNAMIC_COPY);
    
    // Return
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    
	opencl.loadProgramFromFile("MSAOpenCL/Particle.cl");
	kernelUpdate = opencl.loadKernel("updateParticle");
	
	
	clMemParticles.initBuffer(sizeof(Particle) * NUM_PARTICLES, CL_MEM_READ_WRITE, particles);
#ifdef USE_OPENGL_CONTEXT
	clMemPosVBO.initFromGLObject(vbo[0]);
#else
	clMemPosVBO.initBuffer(sizeof(Vec2) * NUM_PARTICLES, CL_MEM_READ_WRITE, particlesPos);
#endif	
	
	kernelUpdate->setArg(0, clMemParticles.getCLMem());
	kernelUpdate->setArg(1, clMemPosVBO.getCLMem());
	kernelUpdate->setArg(2, mousePos);
	kernelUpdate->setArg(3, dimensions);

	// Set opengl parameters
	glPointSize(POINT_SIZE);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
}


//--------------------------------------------------------------
void testApp::update(){
	mousePos.x = ofGetMouseX();
	mousePos.y = ofGetMouseY();
	dimensions.x = ofGetWidth();
	dimensions.y = ofGetHeight();
	
	kernelUpdate->setArg(2, mousePos);
	kernelUpdate->setArg(3, dimensions);
	kernelUpdate->run1D(NUM_PARTICLES);
}

//--------------------------------------------------------------
void testApp::draw(){

//	glColor4f(0.0f, 1.0f, 1.0f, 0.1f);
    
#ifdef USE_OPENGL_CONTEXT
    printf("Operating in opencl mode...");
	opencl.finish();
#else
    printf("Operating in opengl mode...");
//	opencl.readBuffer(sizeof(Particle) * NUM_PARTICLES, clMemParticles, particles);
//	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle) * NUM_PARTICLES, particles);
    
	opencl.readBuffer(sizeof(Vec2) * NUM_PARTICLES, clMemPosVBO, particlesPos);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec2) * NUM_PARTICLES, particlesPos);
#endif	

    // Bind color array
	glBindBuffer(GL_ARRAY_BUFFER, cvbo[0]);
    
    // Update colors
//    for(int i=0; i<NUM_PARTICLES; i++) {
//		Particle &p = particles[i];
////        particlesCol[i].set(ofRandom(1), ofRandom(1), ofRandom(1), CALPHA);
//        particlesCol[i].set(particlesCol[i].x, particlesCol[i].y, particlesCol[i].z, p.alpha);
//	}
    
	glBufferData(GL_ARRAY_BUFFER, sizeof(float4) * NUM_PARTICLES, particlesCol, GL_DYNAMIC_COPY);
    glColorPointer(4, GL_FLOAT, 0, 0);

    // Bind vertex array
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexPointer(2, GL_FLOAT, 0, 0);
    
    // Enable states
    glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
    
    // Draw particles
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
    
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

	glColor4f(0, 1, 0, 1);
	string info = "fps: " + ofToString(ofGetFrameRate()) + "\nnumber of particles: " + ofToString(NUM_PARTICLES);
	ofDrawBitmapString(info, 20, 20);

}

//--------------------------------------------------------------
void testApp::exit() {
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	if(key == 'f'){
        ofToggleFullscreen();
	}
    else if(key == 'r'){
        setup();
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
	
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
	
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
	
}

