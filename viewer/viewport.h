#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__

#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>

class Viewport {

class CameraMotion{

public:
    CameraMotion() = default;

    CameraMotion(Viewport* viewport): 
        viewport(viewport), 
        transform(Eigen::Matrix4f::Identity()),
        deltaTransform(Eigen::Matrix4f::Identity()),
        last_z(0.98f),
        intersection_center(Eigen::Vector3f::Zero()){}

    void rotate(const Eigen::Vector2f& pos){
        Eigen::Vector2f offset = (pos - prevPos);
        float rf = 0.005f;

        Eigen::Matrix4f T1 = Eigen::Matrix4f::Identity();
        Eigen::Matrix3f Rx = Eigen::AngleAxisf(offset.y() * rf, Eigen::Vector3f::UnitX()).toRotationMatrix();
        Eigen::Matrix3f Ry = Eigen::AngleAxisf(offset.x() * rf, Eigen::Vector3f::UnitY()).toRotationMatrix();
        T1.block<3, 3>(0, 0) = Rx * Ry;
        T1.block<3, 1>(0, 3) = T1.block<3, 3>(0, 0) * -intersection_center;

        Eigen::Matrix4f T2 = Eigen::Matrix4f::Identity();
        T2.block<3, 1>(0, 3) = intersection_center;

        deltaTransform = T2 * T1;

        transform = transform * deltaTransform.inverse();
        deltaTransform = Eigen::Matrix4f::Identity();
        prevPos = pos;
    }

    void translate(const Eigen::Vector2f& pos){
        Eigen::Vector2f offset = (pos - prevPos);
        float tf = 0.1;
        if(last_z == 1){
            deltaTransform.block<3, 1>(0, 3) = Eigen::Vector3f(offset.x() * tf, -offset.y() * tf, 0);
        }else{
            Eigen::Vector3f Pw;
            Eigen::Vector3f Pc;
            viewport->pixelUnproject(pos, last_z, Pw, Pc);
            float deltaX = Pc.x() - intersection_center.x();
            float deltaY = Pc.y() - intersection_center.y();
            deltaTransform.block<3, 1>(0, 3) = Eigen::Vector3f(deltaX, deltaY, 0);
            intersection_center = Pc;
        }

        transform = transform * deltaTransform.inverse();
        deltaTransform = Eigen::Matrix4f::Identity();
        prevPos = pos;
    }

    void zoom(float delta){

        deltaTransform(2, 3) = (delta > 0? 1: -1);

        transform = transform * deltaTransform.inverse();
        deltaTransform = Eigen::Matrix4f::Identity();
    }

    void initScreenPos(const Eigen::Vector2f& pos){

        float zNDC;
        Eigen::Vector3f pc;
        Eigen::Vector3f pw;
        viewport->getPixelPosition(pos, zNDC, pw, pc, last_z);

        if(zNDC != 1){
            last_z = zNDC;
            intersection_center = pc;
        }
        prevPos = pos;
    }

    void initTransformation(const Eigen::Matrix4f& transform){
        this->transform = transform;
    }

    Eigen::Matrix3f getRotation() const{
        return transform.block<3, 3>(0, 0);
    }

    Eigen::Vector3f getPosition() const{
        return transform.block<3, 1>(0, 3);
    }

    Eigen::Matrix4f getTransformation() const{
        return transform;
    }

private:
    float last_z;
    Eigen::Vector3f intersection_center;
    Eigen::Matrix4f transform;      // camera to world
    Eigen::Matrix4f deltaTransform;
    Eigen::Vector2f prevPos;
    Viewport* viewport = nullptr;
};

public:
    Eigen::Vector2i windowSize;
    Eigen::Vector2i frameBufferSize;
    Eigen::Matrix4f openGLTransform;

    float zNear = 1.0e-1;
    float zFar = 1.0e2;
    float fov = 90.0f;

    CameraMotion camera;

    Viewport(
        size_t width            = 1280,
        size_t height           = 720,
        Eigen::Vector3f eye     = Eigen::Vector3f(1, 1, 1), 
        Eigen::Vector3f center  = Eigen::Vector3f(0, 0, 0), 
        Eigen::Vector3f up      = Eigen::Vector3f(0, 0, 1)) // up usually be set to (0, 1, 0)
    {
        windowSize = Eigen::Vector2i(width, height);
        Eigen::Vector3f zAxis = (center - eye).normalized();
        Eigen::Vector3f xAxis = up.cross(zAxis).normalized();
        Eigen::Vector3f yAxis = zAxis.cross(xAxis).normalized();

        Eigen::Matrix3f R = Eigen::Matrix3f::Identity();
        R.col(0) = xAxis;
        R.col(1) = yAxis;
        R.col(2) = -zAxis;
        
        Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
        transform.block<3, 3>(0, 0) = R;
        transform.block<3, 1>(0, 3) = (eye - center) * 3.0f + center;

        camera = CameraMotion(this);
        camera.initTransformation(transform);

        openGLTransform = Eigen::Matrix4f::Identity();
        openGLTransform(1, 1) = -1;
        openGLTransform(2, 2) = -1;
    }

    float getFocal() const {
        return windowSize.y() / (tan((fov / 180.0f * M_PI) / 2.0f) * 2.0f);
    }

    Eigen::Vector2f getTanXY() const {
        float tanHalfFov = tan((fov / 180.0 * M_PI) / 2.0f);
        float aspect = static_cast<float>(frameBufferSize.x()) / frameBufferSize.y();
        return Eigen::Vector2f(aspect * tanHalfFov, tanHalfFov);
    }

    Eigen::Vector3f getCameraPosition() const {
        return camera.getPosition();
    }

    Eigen::Matrix3f getCameraRotation() const {
        return camera.getRotation();
    }

    Eigen::Vector2i getFrameBufferSize() const {
        return frameBufferSize;
    }

    void getPixelPosition(const Eigen::Vector2f& pos, float& zNDC, Eigen::Vector3f& Pw, Eigen::Vector3f& Pc, float default_z){
        Eigen::Vector2i pick_pos = {int(pos.x()), int(windowSize.y() - pos.y())};
        glReadPixels(pick_pos.x(), pick_pos.y(), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &zNDC);

        if(zNDC == 1){
            zNDC = default_z;
        }

        pixelUnproject(pos, zNDC, Pw, Pc);
    }

    void pixelUnproject(const Eigen::Vector2f& pos, const float& zNDC, Eigen::Vector3f& Pw, Eigen::Vector3f& Pc){
        Eigen::Vector3d Pc_;
        Eigen::Matrix4d Identity = Eigen::Matrix4d::Identity();
        Eigen::Matrix4d Projmatrix = getProjectionMatrix().cast<double>();
        Eigen::Vector4i Viewport = Eigen::Vector4i(0, 0, windowSize.x(), windowSize.y());
        Eigen::Vector2i pick_pos = {int(pos.x()), int(windowSize.y() - pos.y())};
        gluUnProject(pick_pos.x(), pick_pos.y(), zNDC, Identity.data(), Projmatrix.data(), Viewport.data(), &Pc_.x(), &Pc_.y(), &Pc_.z());
        Pc = Pc_.cast<float>();
        Pw = camera.getRotation() * Pc + camera.getPosition();
    }

    Eigen::Matrix4f getViewMatrix() const{
        return camera.getTransformation().inverse();
    }

    Eigen::Matrix4f getProjectionMatrix() const {

        float tanHalfFov = tan((fov / 180.0 * M_PI) / 2.0f);
        float aspect = static_cast<float>(frameBufferSize.x()) / frameBufferSize.y();

        Eigen::Matrix4f projmatrix = Eigen::Matrix4f::Zero();
        projmatrix(0, 0) = 1.0f / (aspect * tanHalfFov);
        projmatrix(1, 1) = 1.0f / tanHalfFov;
        projmatrix(2, 2) = -(zFar + zNear) / (zFar - zNear);
        projmatrix(2, 3) = -(2.0f * zFar * zNear) / (zFar - zNear);
        projmatrix(3, 2) = -1.0f; 

        return projmatrix;
    }

    void setFoV(float fov){
        this->fov = fov;
    }

    void setViewMatrix(Eigen::Matrix4f pose, float dist){
        Eigen::Matrix4f delta = Eigen::Matrix4f::Identity();
        delta(2, 3) = -dist;
        camera.initTransformation(pose * delta * openGLTransform);
    }

    void setViewMatrix(Eigen::Matrix4f viewmat){
        camera.initTransformation(viewmat);
    }

    void setProjectionMatrix(float zNear, float zFar, float fov){
        this->zNear = zNear;
        this->zFar = zFar;
        this->fov = fov;
    }
};

#endif