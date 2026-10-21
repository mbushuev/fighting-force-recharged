#pragma once
#include "orbit_camera.h"
#include <vector>
#include <stddef.h>

namespace ff_camera {
inline Settings FirstPersonLookSettings(const Settings& orbit) {
    Settings result=orbit;
    result.mouse_x=0.30*kPi/180;
    result.mouse_y=0.24*kPi/180;
    result.stick_speed=240*kPi/180;
    return result;
}
struct BoneStep {
    int operation=0,special=0;
    int32_t translation[3]={},bind[3]={},post[3]={};
    unsigned joint=0,first=0,count=0;
    bool head=false;
};
struct HeadSkeleton {
    std::vector<BoneStep> steps;
    std::vector<unsigned char> head_vertices;
    int eye_step=-1;
    bool Build(const int32_t* commands,size_t words,const unsigned* counts,
               unsigned groups,unsigned vertices,unsigned joints) {
        *this={};
        if(!commands || !counts || !groups || groups>96 || !vertices || vertices>16384 ||
           !joints || joints>48 || words>16384)return false;
        bool ancestry[32]={},head=false;unsigned depth=0,group=0,first=0,heads=0;
        head_vertices.resize(vertices);
        for(size_t cursor=0;cursor<words;) {
            BoneStep step;step.operation=commands[cursor++];
            if(!step.operation) {
                if(depth || group!=groups || first!=vertices || heads!=1 || eye_step<0)break;
                return true;
            }
            if(step.operation==1) {
                if(depth>=31)break;
                ancestry[depth++]=head;
            } else if(step.operation==2) {
                if(!depth)break;
                head=ancestry[--depth];
            } else {
                if(step.operation!=3 || group>=groups || words-cursor<11)break;
                step.special=commands[cursor++];
                for(auto& value:step.translation)value=commands[cursor++];
                for(auto& value:step.bind)value=commands[cursor++];
                const int joint=commands[cursor++];
                if(joint<0 || unsigned(joint)>=joints)break;
                step.joint=unsigned(joint);
                for(auto& value:step.post)value=commands[cursor++];
                if(step.special==1) {head=true;++heads;}
                step.head=head;step.first=first;step.count=counts[group++];
                if(step.count>vertices-first)break;
                first+=step.count;
                if(head) {
                    for(unsigned v=step.first;v<first;++v)head_vertices[v]=1;
                    // Some characters put an empty neck node before the head
                    // mesh and hair. The largest mesh in that subtree is the skull.
                    if(step.count && (eye_step<0 || step.count>steps[eye_step].count))
                        eye_step=static_cast<int>(steps.size());
                }
            }
            steps.push_back(step);
        }
        *this={};return false;
    }
    bool HeadFace(const int16_t* face) const {
        for(int i=0;i<4;++i)
            if(face[i]>=0 && size_t(face[i])<head_vertices.size() && head_vertices[face[i]])return true;
        return false;
    }
};
struct FirstPersonView {
    bool selected=false,held=true,blocked=true;
    double yaw=0,pitch=0;
    void RequireRelease() {held=blocked=true;}
    bool Toggle(bool down,bool allowed) {
        if(!allowed) {RequireRelease();return false;}
        if(blocked) {held=down;if(!down)blocked=false;return false;}
        const bool fresh=down&&!held;held=down;
        if(fresh)selected=!selected;
        return fresh;
    }
    void Begin(int16_t heading) {yaw=Radians(BehindActorYaw(heading));pitch=0;}
    void Look(double mx,double my,double sx,double sy,double seconds,const Settings& settings) {
        const double dt=Clamp(seconds,0,0.1);
        yaw=Wrap(yaw-mx*settings.mouse_x-sx*settings.stick_speed*dt);
        const double vertical=my*settings.mouse_y-sy*settings.stick_speed*dt;
        pitch=Clamp(pitch+(settings.invert_y?-vertical:vertical),-80*kPi/180,80*kPi/180);
    }
    Point Eye(Point head,double up,double forward) const {
        return {head.x-sin(yaw)*forward,head.y-up,head.z+cos(yaw)*forward};
    }
    Pose Follow(Point eye) const {return {Fixed(eye.x),Fixed(eye.y),Fixed(eye.z),NativeAngle(yaw),NativeAngle(pitch)};}
    int16_t BodyHeading() const {
        return static_cast<int16_t>(static_cast<uint16_t>(uint16_t(NativeAngle(yaw))-32768u)>>5);
    }
};
} // namespace ff_camera
