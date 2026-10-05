#include "./Animation.h"
Animation::Animation(): index(0),numFrames(0),animationSpeed(0){

}
Animation::Animation(unsigned int index,unsigned int numFrames,unsigned int animationSpeed):
index(index),numFrames(numFrames),animationSpeed(animationSpeed){
    
}