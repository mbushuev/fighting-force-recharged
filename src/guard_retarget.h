#pragma once
#include "guard_animation.h"
#include <math.h>

namespace ff_guard {
// Mesh rotations precede each bone's post rotation. Keeping the destination
// post transform is essential: Hawk's wrist translation uses the opposite axis.
struct Rotation {
    double m[3][3]={{1,0,0},{0,1,0},{0,0,1}};
    Rotation operator*(const Rotation& b) const {
        Rotation out;
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c) {
            out.m[r][c]=0;for(unsigned k=0;k<3;++k)out.m[r][c]+=m[r][k]*b.m[k][c];
        }
        return out;
    }
    Rotation Inverse() const {
        Rotation out;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)out.m[r][c]=m[c][r];return out;
    }
    static Rotation Euler(const int16_t* angles) {
        const double unit=6.2831853071795864769/4096;
        const double z=angles[0]*unit,y=angles[1]*unit,x=angles[2]*unit;
        const double cz=cos(z),sz=sin(z),cy=cos(y),sy=sin(y),cx=cos(x),sx=sin(x);
        Rotation out;
        out.m[0][0]=cz*cy;out.m[0][1]=cz*sy*sx-sz*cx;out.m[0][2]=cz*sy*cx+sz*sx;
        out.m[1][0]=sz*cy;out.m[1][1]=sz*sy*sx+cz*cx;out.m[1][2]=sz*sy*cx-cz*sx;
        out.m[2][0]=-sy;out.m[2][1]=cy*sx;out.m[2][2]=cy*cx;return out;
    }
    void Angles(int16_t* out,const int16_t* previous) const {
        const double pi=3.14159265358979323846,unit=4096/(2*pi);
        const double cy=hypot(m[0][0],m[1][0]);
        double y=atan2(-m[2][0],cy),z,x;
        if(cy>1e-8) {z=atan2(m[1][0],m[0][0]);x=atan2(m[2][1],m[2][2]);}
        else {x=previous[2]/unit;z=atan2(-m[0][1],m[1][1])+(y>0?x:-x);}
        const double branches[2][3]={{z,y,x},{z+pi,pi-y,x+pi}};
        int best=0x7FFFFFFF;
        for(const auto& branch:branches) {
            int16_t candidate[3];int score=0;
            for(unsigned c=0;c<3;++c) {
                candidate[c]=static_cast<int16_t>((static_cast<int>(lround(branch[c]*unit/2))*2)&4094);
                const int delta=AngleDelta(previous[c],candidate[c]);score+=delta*delta;
            }
            if(score<best){best=score;memcpy(out,candidate,sizeof(candidate));}
        }
    }
};
struct GuardBone {unsigned joint=0,parent=0;Rotation bind,post;};
struct GuardSkeleton {
    unsigned joints=0;std::vector<GuardBone> bones;
    explicit GuardSkeleton(Bytes model) {
        joints=model.U32(24);const unsigned groups=model.U32(32),vertices=model.U32(4),commands=model.U32(28);
        if(!joints || joints>48 || !groups || groups>96 || commands>model.size-4 || vertices>model.size-4 ||
           commands<32+groups*4 || vertices<=commands ||
           (vertices-commands)%4 || vertices-commands>65536)throw std::runtime_error("Invalid guard skeleton");
        const auto words=model.Slice(4+commands,vertices-commands);
        unsigned parent=0;bool seen[48]={};std::vector<unsigned> stack;bool ended=false;
        for(size_t cursor=0;cursor<words.size;) {
            const unsigned op=words.U32(cursor);cursor+=4;
            if(!op){ended=true;break;}
            if(op==1){if(stack.size()>=32)throw std::runtime_error("Deep guard skeleton");stack.push_back(parent);}
            else if(op==2){if(stack.empty())throw std::runtime_error("Invalid guard stack");parent=stack.back();stack.pop_back();}
            else if(op==3) {
                const auto record=words.Slice(cursor,44);cursor+=44;GuardBone bone;
                bone.joint=record.U32(28);bone.parent=parent;
                if(!bone.joint || bone.joint>=joints || seen[bone.joint] || bones.size()>=groups)
                    throw std::runtime_error("Invalid guard bone");
                seen[bone.joint]=true;int16_t bind[3],post[3];
                for(unsigned c=0;c<3;++c){bind[c]=static_cast<int16_t>(record.U32(16+c*4));post[c]=static_cast<int16_t>(record.U32(32+c*4));}
                bone.bind=Rotation::Euler(bind);bone.post=Rotation::Euler(post);bones.push_back(bone);parent=bone.joint;
            } else throw std::runtime_error("Unknown guard bone command");
        }
        if(!ended || !stack.empty() || bones.size()!=groups)throw std::runtime_error("Incomplete guard skeleton");
    }
};
struct UpperBodyClips:GuardClips {
    void Build(Bytes hero,Bytes donor,const char* hero_model,const char* hero_animation,
               const char* donor_model,const char* donor_animation,unsigned joints,unsigned upper) {
        const GuardSkeleton target(Member(hero,hero_model)),source(Member(donor,donor_model));
        if(target.joints!=joints || !upper || upper>=joints || upper>=source.joints)
            throw std::runtime_error("Unsupported guard mapping");
        // Only the reviewed matching upper-body tree is retargeted. The native
        // idle root, feet and accessory angles remain in their own bind spaces.
        for(unsigned i=0;i<upper;++i)if(i>=target.bones.size() || i>=source.bones.size() ||
            target.bones[i].joint!=i+1 || source.bones[i].joint!=i+1 ||
            target.bones[i].parent!=source.bones[i].parent)throw std::runtime_error("Unsupported guard tree");
        Pose idle=Decode(Clip(Member(hero,hero_animation),"STBY1"),joints)[0];idle.root[1]=idle.root[2]=0;
        const auto animations=Member(donor,donor_animation);
        const char* names[]={"BLOCKIN1","BLOCKSB1","BLOCKHT1","BLOCKOT1"};
        for(unsigned clip=0;clip<4;++clip) {
            const auto frames=Decode(Clip(animations,names[clip]),source.joints);Poses converted;Pose previous=idle;
            for(const auto& pose:frames) {
                Pose out=idle;Rotation from[48],to[48];
                for(unsigned b=0;b<upper;++b) {
                    const auto& s=source.bones[b];const auto& t=target.bones[b];
                    const auto mesh=from[s.parent]*s.bind*Rotation::Euler(&pose.angles[s.joint*3]);
                    from[s.joint]=mesh*s.post;
                    const auto basis=to[t.parent]*t.bind;
                    (basis.Inverse()*mesh).Angles(&out.angles[t.joint*3],&previous.angles[t.joint*3]);
                    to[t.joint]=basis*Rotation::Euler(&out.angles[t.joint*3])*t.post;
                }
                converted.push_back(out);previous=out;
            }
            if(clip==0) {
                Poses prefix;for(unsigned i=0;i<4;++i)prefix.push_back(Blend(idle,converted.front(),i,4));
                converted.insert(converted.begin(),prefix.begin(),prefix.end());
            } else if(clip==3) {
                const auto last=converted.back();for(unsigned i=1;i<=4;++i)converted.push_back(Blend(last,idle,i,4));
            }
            height[clip]=idle.root[0];clips[clip]=Encode(converted);
        }
    }
};
}
