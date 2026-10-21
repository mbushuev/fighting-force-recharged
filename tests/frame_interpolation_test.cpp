#include <stdio.h>
#include <limits.h>
#include "../src/frame_interpolation.h"
#include "../src/frame_rate.h"
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static void RootVector(const int16_t* angles,double (&p)[3]) {
    // Native root order: Rx(-z) Ry(-y) Rz(x), before the actor's yaw.
    const double unit=2*acos(-1.0)/4096;
    double c=cos(angles[0]*unit),s=sin(angles[0]*unit),a=p[0],b=p[1];
    p[0]=a*c-b*s;p[1]=a*s+b*c;
    c=cos(-angles[1]*unit);s=sin(-angles[1]*unit);a=p[0];b=p[2];
    p[0]=a*c+b*s;p[2]=-a*s+b*c;
    c=cos(-angles[2]*unit);s=sin(-angles[2]*unit);a=p[1];b=p[2];
    p[1]=a*c-b*s;p[2]=a*s+b*c;
}
static void RotationBranches() {
    using namespace ff_frame;
    // Equivalent Euler branches must not introduce a roll or pitch between
    // identical endpoint orientations, including wrapped and singular angles.
    for(int x:{0,128,1024,1920,2048,3072,4095})
    for(int y:{0,128,1024,1920,2048,3072,4095})
    for(int z:{0,128,1024,1920,2048,3072,4095}) {
        const int16_t before[]={int16_t(x),int16_t(y),int16_t(z)};
        const int16_t now[]={int16_t((x+2048)&4095),int16_t((2048-y)&4095),int16_t((z+2048)&4095)};
        int16_t out[3];JointRotation(out,before,now,0);
        CHECK(!memcmp(out,before,sizeof(out)));
        for(int axis=0;axis<3;++axis) {
            double a[3]={},b[3]={},mid[3]={};a[axis]=b[axis]=mid[axis]=1;
            RootVector(before,a);RootVector(now,b);RootVector(out,mid);
            for(int i=0;i<3;++i)CHECK(fabs(a[i]-b[i])<1e-9 && fabs(a[i]-mid[i])<1e-9);
        }
    }
    // A turn crossing the middle-axis singularity stays upright; interpolating
    // the three native components independently would introduce a body tilt.
    const int16_t before[]={0,3800,0},now[]={2048,3200,2048};int16_t mid[3],reverse[3];
    JointRotation(mid,before,now,0);JointRotation(reverse,now,before,0);
    double up[3]={0,1,0},back[3]={0,1,0};RootVector(mid,up);RootVector(reverse,back);
    CHECK(up[1]>0.999999 && back[1]>0.999999);
    double old_up[3]={0,1,0};int16_t independent[3];
    for(unsigned i=0;i<3;++i)independent[i]=int16_t(Angle(before[i],now[i],4096));
    RootVector(independent,old_up);CHECK(old_up[1]<0.9);
    const int16_t level[]={0,0,0},slide[]={0,0,1024};JointRotation(mid,level,slide,0);
    CHECK(mid[0]==0 && mid[1]==0 && mid[2]==512); // Real pitch/roll animation still blends.
}
int main() {
    using namespace ff_frame;
    RotationBranches();
    CHECK(Midpoint(INT_MIN,INT_MAX)==0);
    CHECK(Midpoint(INT_MAX,INT_MAX)==INT_MAX);
    CHECK(Midpoint(INT_MIN,INT_MIN)==INT_MIN);
    CHECK(Angle(2040,8,2048)==0 && Angle(8,2040,2048)==0);
    CHECK(Angle(4090,6,4096)==0 && Angle(6,4090,4096)==0);
    CHECK(Angle(-32,32,65536)==0);
    Snapshot a,b;a.stage=b.stage=1;a.players=b.players=2;a.tick=UINT_MAX;b.tick=0;
    CHECK(a.Consecutive(b));b.tick=1;CHECK(!a.Consecutive(b));
    b.tick=0;b.stage=2;CHECK(!a.Consecutive(b));b.stage=1;
    b.eye[1]=true;CHECK(!a.Consecutive(b));b.eye[1]=false;
    b.players=1;CHECK(!a.Consecutive(b));b.players=2;
    Write<int16_t>(a.actors[0],0x34,2);Write<int16_t>(b.actors[0],0x34,2);
    Write<int32_t>(a.actors[0],0x28,-100*65536);Write<int32_t>(b.actors[0],0x28,100*65536);
    Write<int32_t>(a.actors[0],0x10,10);Write<int32_t>(b.actors[0],0x10,30);
    Write<int16_t>(a.actors[0],0x38,2040);Write<int16_t>(b.actors[0],0x38,8);
    Write<int16_t>(a.palettes[0],0x118,4090);Write<int16_t>(b.palettes[0],0x118,6);
    Write<int32_t>(b.palettes[0],4,0x12345678); // Opaque native data stays exact.
    uint8_t actor[0xF4],palette[0x238];memcpy(actor,b.actors[0],sizeof(actor));memcpy(palette,b.palettes[0],sizeof(palette));
    CHECK(Actor(actor,palette,a.actors[0],b.actors[0],a.palettes[0],b.palettes[0]));
    CHECK(Read<int32_t>(actor,0x28)==0 && Read<int32_t>(actor,0x10)==20);
    CHECK(Read<int16_t>(actor,0x38)==0 && Read<int16_t>(palette,0x118)==0);
    CHECK(Read<int32_t>(palette,4)==0x12345678);
    // Backstep completion moves a half-turn between root and body heading.
    // The intermediate sample must retain that native pair in either direction,
    // while position and other joints keep their ordinary interpolation.
    for(int direction=0;direction<2;++direction) {
        Snapshot old_pose,new_pose;
        Write<int16_t>(old_pose.actors[0],0x34,2);Write<int16_t>(new_pose.actors[0],0x34,2);
        Write<int16_t>(old_pose.actors[0],0x38,int16_t(direction?1024:0));
        Write<int16_t>(new_pose.actors[0],0x38,int16_t(direction?0:1024));
        for(unsigned o:{0x118u,0x11Cu}) {
            Write<int16_t>(old_pose.palettes[0],o,int16_t(direction?0:2048));
            Write<int16_t>(new_pose.palettes[0],o,int16_t(direction?2048:0));
        }
        Write<int32_t>(new_pose.actors[0],0x28,100*65536);
        Write<int16_t>(new_pose.palettes[0],0x11E,100);
        int16_t naive_root[3];
        for(unsigned i=0;i<3;++i)naive_root[i]=int16_t(Angle(Read<int16_t>(old_pose.palettes[0],0x118+i*2),
            Read<int16_t>(new_pose.palettes[0],0x118+i*2),4096));
        double sideways[3]={0,1,0};RootVector(naive_root,sideways);CHECK(fabs(sideways[1])<1e-9);
        memcpy(actor,new_pose.actors[0],sizeof(actor));memcpy(palette,new_pose.palettes[0],sizeof(palette));
        CHECK(Actor(actor,palette,old_pose.actors[0],new_pose.actors[0],old_pose.palettes[0],new_pose.palettes[0]));
        CHECK(Read<int32_t>(actor,0x28)==50*65536 && Read<int16_t>(palette,0x11E)==50);
        CHECK(Read<int16_t>(actor,0x38)==Read<int16_t>(new_pose.actors[0],0x38));
        CHECK(!memcmp(palette+0x118,new_pose.palettes[0]+0x118,6));
        int16_t root[3];memcpy(root,palette+0x118,sizeof(root));double forward[3]={0,0,1};RootVector(root,forward);
        const double yaw=(1024-Read<int16_t>(actor,0x38))*2*acos(-1.0)/2048;
        const double x=cos(yaw)*forward[0]+sin(yaw)*forward[2];
        const double z=-sin(yaw)*forward[0]+cos(yaw)*forward[2];
        CHECK(fabs(x)<1e-9 && fabs(z-1)<1e-9);
        CHECK(Read<int32_t>(new_pose.actors[0],0x28)==100*65536 && Read<int16_t>(new_pose.palettes[0],0x11E)==100);
    }
    Write<int16_t>(b.actors[0],0x34,3);
    CHECK(!Actor(actor,palette,a.actors[0],b.actors[0],a.palettes[0],b.palettes[0]));
    Write<int16_t>(b.actors[0],0x34,2);Write<int32_t>(b.actors[0],0x28,2000*65536);
    CHECK(!Actor(actor,palette,a.actors[0],b.actors[0],a.palettes[0],b.palettes[0]));
    Write<int32_t>(a.objects[0],0,500);Write<int32_t>(b.objects[0],0,500);
    Write<int32_t>(b.objects[0],4,100*65536);Write<int16_t>(a.objects[0],0x10,-32);Write<int16_t>(b.objects[0],0x10,32);
    uint8_t object[36];memcpy(object,b.objects[0],sizeof(object));
    CHECK(Object(object,a.objects[0],b.objects[0]));
    CHECK(Read<int32_t>(object,4)==50*65536 && Read<int16_t>(object,0x10)==0);
    Write<int32_t>(b.objects[0],0,-1);CHECK(!Object(object,a.objects[0],b.objects[0]));
    Cadence pace;
    for(int i=1;i<=600;++i)CHECK(pace.Next((i-1)*100,6000)==i*100);
    CHECK(pace.Next(100000,6000)==100100); // A pause cannot cause catch-up simulation.
    CHECK(pace.Next(10,6000)==110);pace.Reset();CHECK(pace.Next(200,6000)==300);
    RateSamples samples;double fps=0,ups=0;
    for(unsigned i=0;i<121;++i)CHECK(samples.Add(i*100,6000,i/2,0,fps,ups)==(i==120));
    CHECK(fps==60 && ups==30);
    samples={};
    for(unsigned i=0;i<80;++i)CHECK(!samples.Add(i*100,6000,500+i/2,0,fps,ups));
    // Same-stage restart resets the aggregate even with no wall-clock gap.
    for(unsigned i=0;i<121;++i)CHECK(samples.Add(8000+i*100,6000,i/2,0,fps,ups)==(i==120));
    CHECK(fps==60 && ups==30);
    samples={};
    for(unsigned i=0;i<80;++i)CHECK(!samples.Add(i*100,6000,i/2,0,fps,ups));
    // A stage change with consecutive ticks also starts a fresh aggregate.
    for(unsigned i=0;i<121;++i)CHECK(samples.Add(8000+i*100,6000,40+i/2,1,fps,ups)==(i==120));
    CHECK(fps==60 && ups==30);
    samples={};
    for(unsigned i=0;i<80;++i)CHECK(!samples.Add(i*100,6000,i/2,0,fps,ups));
    for(unsigned i=0;i<121;++i)CHECK(samples.Add(20000+i*100,6000,40+i/2,0,fps,ups)==(i==120));
    CHECK(fps==60 && ups==30);
    samples={};
    for(unsigned i=0;i<121;++i)CHECK(samples.Add(i*100,6000,UINT_MAX-30+i/2,0,fps,ups)==(i==120));
    CHECK(fps==60 && ups==30); // Legitimate unsigned tick wrap remains valid.
    printf("Render interpolation and 60 Hz cadence: %s\n",failures?"FAILED":"passed");
    return failures?1:0;
}
