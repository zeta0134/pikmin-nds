#include "RedPikmin.h"
#include "pikmin_generic_dsgx.h"

RedPikmin::RedPikmin() {
    u32* data = (u32*)pikmin_generic_dsgx;
    
    Vector3<v16,12> model_center;
    model_center.x = (gx::Fixed<v16,12>)floattov16(((float*)data)[0]);
    model_center.y = (gx::Fixed<v16,12>)floattov16(((float*)data)[1]);
    model_center.z = (gx::Fixed<v16,12>)floattov16(((float*)data)[2]);
    v16 radius = floattov16(((float*)data)[3]);
    int cull_cost = (int)data[4];
    
    setActor(
        &data[5], //start of model data, including number of commands
        model_center,
        radius,
        cull_cost);
}

void RedPikmin::update(MultipassEngine* engine) {
    setRotation({0,rotation,0});
    rotation += 1;
}