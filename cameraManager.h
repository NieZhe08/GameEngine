#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <iostream>
#include "glm/glm.hpp"


class CameraManager {
    private:
        glm::vec2 cam;
        float zoom_factor;
    
    public:
        CameraManager() : cam(0.0f, 0.0f), zoom_factor(1.0f) {}

        void setPosition (float x, float y) {
            cam = glm::vec2(x, y);
        }

        float getPositionX() {
            return cam.x;
        }

        float getPositionY() {
            return cam.y;
        }

        void setZoom(float zoom) {
            zoom_factor = (zoom <= 0.0f) ? 1.0f : zoom;
        }

        float getZoom(){
            return zoom_factor;
        }
};

#endif