#pragma once

#include "..\Math\Rotation.h"
#include "..\Math\Transform.h"
#include "..\Render\CameraLayout.h"
#include "PixelGroup.h"
#include "Scene.h"
#include "Triangle2D.h"

class Camera {
private:
    Transform* transform;
    CameraLayout* cameraLayout;
    PixelGroup* pixelGroup;
    Quaternion rayDirection;
    Quaternion lookDirection;
    Quaternion lookOffset;

    RGBColor CheckRasterPixel(Triangle2D** triangles, int numTriangles, Vector2D pixelRay){
        float zBuffer = 3.402823466e+38f;
        int triangle = 0;
        bool didIntersect = false;
        float u = 0.0f, v = 0.0f, w = 0.0f, uf = 0.0f, vf = 0.0f, wf = 0.0f;;
        RGBColor color;
        
        for (int t = 0; t < numTriangles; t++) {
            if (triangles[t]->DidIntersect(pixelRay.X, pixelRay.Y, u, v, w)){
                if(triangles[t]->averageDepth < zBuffer){
                    uf = u;
                    vf = v;
                    wf = w;
                    zBuffer = triangles[t]->averageDepth;
                    triangle = t;
                    didIntersect = true;
                }
            }
        }

        if(didIntersect){
            Vector3D intersect = (*triangles[triangle]->t3p1 * uf) + (*triangles[triangle]->t3p2 * vf) + (*triangles[triangle]->t3p3 * wf);

            intersect = rayDirection.UnrotateVector(intersect);
            
            color = triangles[triangle]->GetMaterial()->GetRGB(intersect, *triangles[triangle]->normal);
        }
        
        return color;
    }

public:
    Camera(Transform* transform, CameraLayout* cameraLayout, PixelGroup* pixelGroup){
        this->transform = transform;
        this->pixelGroup = pixelGroup;
        this->cameraLayout = cameraLayout;

        transform->SetBaseRotation(cameraLayout->GetRotation());
    }

    Transform* GetTransform(){
        return transform;
    }

    void SetLookOffset(Quaternion lookOffset){
        this->lookOffset = lookOffset;
    }

    void Rasterize(Scene* scene) {
        int numTriangles = 0;
    
        //for each object in the scene, get the triangles
        for(int i = 0; i < scene->numObjects; i++){
            if(scene->objects[i]->IsEnabled()){
                numTriangles += scene->objects[i]->GetTriangleGroup()->GetTriangleCount();
            }
        }
        
        Triangle2D** triangles = new Triangle2D*[numTriangles];
        int triangleCounter = 0;
        
        lookDirection = transform->GetRotation().Conjugate() * lookOffset;
        rayDirection  = transform->GetRotation().Multiply(lookDirection);

        //for each object in the scene, get the triangles
        for(int i = 0; i < scene->numObjects; i++){
            if(scene->objects[i]->IsEnabled()){
                //for each triangle in object, project onto 2d surface, but pass material
                for (int j = 0; j < scene->objects[i]->GetTriangleGroup()->GetTriangleCount(); j++) {
                    triangles[triangleCounter] = new Triangle2D(lookDirection, transform, &scene->objects[i]->GetTriangleGroup()->GetTriangles()[j], scene->objects[i]->GetMaterial());
                    
                    bool triangleInView = false;

                    if (triangles[triangleCounter]->averageDepth > 0){//cull behind camera
                        Vector2D p1 = lookDirection.UnrotateVector(triangles[triangleCounter]->GetP1()) * transform->GetScale();
                        Vector2D p2 = lookDirection.UnrotateVector(triangles[triangleCounter]->GetP2()) * transform->GetScale();
                        Vector2D p3 = lookDirection.UnrotateVector(triangles[triangleCounter]->GetP3()) * transform->GetScale();

                        BoundingBox2D triangleBox;

                        triangleBox.UpdateBounds(p1);
                        triangleBox.UpdateBounds(p2);
                        triangleBox.UpdateBounds(p3);

                        triangleInView = pixelGroup->Overlaps(&triangleBox);//cull outside camera view
                    }

                    triangleInView = 1;
                    
                    if(triangleInView) triangleCounter++;
                    else delete triangles[triangleCounter];//out of view space remove from array
                }
            }
        }

        Serial.print(triangleCounter);
        Serial.print("\t");
        Serial.print(numTriangles);
        Serial.print("\t");

        for (unsigned int i = 0; i < pixelGroup->GetPixelCount(); i++) {
            Vector2D pixelRay = Vector2D(lookDirection.RotateVector(pixelGroup->GetPixel(i)->GetPosition() * transform->GetScale()));//scale pixel location prior to rotating and moving
            pixelGroup->GetPixel(i)->Color = CheckRasterPixel(triangles, triangleCounter, pixelRay);
        }

        for (int i = 0; i < triangleCounter; i++){
            delete triangles[i];
        }
        
        delete[] triangles;
    }
    
};
