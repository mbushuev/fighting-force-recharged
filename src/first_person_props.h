#pragma once
#include "first_person_clip.h"

struct FirstPersonPropModel {
    const int16_t* vertices;unsigned vertex_count;
    const int16_t* faces;unsigned face_count;
};
struct FirstPersonPropMaterial {float u0,u1,v0,v1;DWORD unused;int16_t blend,texture;DWORD tail[2];};
struct FirstPersonPropScreenVertex {float x,y,z,rhw;DWORD color,specular;float u,v;};
struct FirstPersonPropPolygon {
    DWORD next;FirstPersonPropScreenVertex vertices[8];
    float sort_depth;DWORD count,texture,kind,blend,flags;
};
static_assert(sizeof(FirstPersonPropMaterial)==32,"Native prop material record");
static_assert(sizeof(FirstPersonPropPolygon)==0x11C,"Native clipped polygon record");
struct FirstPersonPropFragment {
    ff_camera::PropVertex vertices[8];unsigned count;
    const FirstPersonPropMaterial* material;
};
using FirstPersonProjector=int(__cdecl*)(const FirstPersonPropModel*,int,int);
static thread_local bool g_first_person_projecting;
static int CameraProjectNearModel(const FirstPersonPropModel* source,int flags,int depth,
                                 bool view_active,bool two_sided,FirstPersonProjector native,bool floor_pass=false,
                                 const double* rail_plane=nullptr) {
    if(g_first_person_projecting || !view_active || (depth && !rail_plane) || !CameraReadable(source,sizeof(*source)) ||
       !source->vertex_count || source->vertex_count>16384 || !source->face_count || source->face_count>32768 ||
       !CameraReadable(source->vertices,source->vertex_count*8) ||
       !CameraReadable(source->faces,source->face_count*12))return native(source,flags,depth);
    const auto matrix=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    const ff_camera::PropProjection projection={CameraField<float>(0xAD214),CameraField<float>(0xAD234),
        CameraField<float>(0x20C50C),CameraField<float>(0x20C508),
        CameraField<float>(0x9D170),CameraField<float>(0x9D174),CameraField<float>(0x5E8A4)};
    if(projection.focal_x<=0 || projection.focal_y<=0 || projection.width<=0 || projection.height<=0)
        return native(source,flags,depth);
    const double far_depth=(floor_pass || rail_plane)?CameraField<float>(0x6BC64):0;
    const bool clip_far=floor_pass && isfinite(far_depth) && far_depth>1;
    static thread_local std::vector<int16_t> faces;
    static thread_local std::vector<ff_camera::PropVertex> vertices;
    static thread_local std::vector<FirstPersonPropFragment> fragments;
    try {
        vertices.resize(source->vertex_count);fragments.clear();
        bool any_near=false,any_far=false,any_forward=false;
        for(unsigned i=0;i<source->vertex_count;++i) {
            const auto v=source->vertices+i*4;
            vertices[i]={matrix[0]*double(v[0])+matrix[1]*double(v[1])+matrix[2]*double(v[2])+matrix[3],
                matrix[4]*double(v[0])+matrix[5]*double(v[1])+matrix[6]*double(v[2])+matrix[7],
                matrix[8]*double(v[0])+matrix[9]*double(v[1])+matrix[10]*double(v[2])+matrix[11],0,0};
            any_near|=vertices[i].z<1;any_forward|=vertices[i].z>=1;
            any_far|=clip_far && vertices[i].z>=far_depth;
        }
        if(!two_sided && ((!any_near && !any_far && !rail_plane) || !any_forward))return native(source,flags,depth);
        faces.assign(source->faces,source->faces+source->face_count*6);
        for(unsigned i=0;i<source->face_count;++i) {
            auto face=faces.data()+i*6;
            if(face[5]<0 || face[3]<0)continue;
            bool valid=true,crosses_near=false,crosses_far=false,occluded_rail=false,rail_far=false;ff_camera::PropVertex quad[4];
            for(unsigned j=0;j<4;++j) {
                if(face[j]<0 || unsigned(face[j])>=source->vertex_count) {valid=false;break;}
                quad[j]=vertices[face[j]];
                if(!isfinite(quad[j].x) || !isfinite(quad[j].y) || !isfinite(quad[j].z)) {valid=false;break;}
                crosses_near|=quad[j].z<1;
                crosses_far|=clip_far && quad[j].z>=far_depth;
                rail_far|=rail_plane && quad[j].z>=far_depth;
                // Only the recessed sides can be hidden by the platform.
                // The original top ledges keep their full extent and UVs.
                if(rail_plane && source->vertices[face[j]*4+1]<0)
                    occluded_rail|=quad[j].x*rail_plane[0]+quad[j].y*rail_plane[1]+quad[j].z*rail_plane[2]<0;
            }
            // Retain the native whole-face far rejection for depth-biased
            // rails; this local occlusion repair does not extend their range.
            if(!valid || rail_far)continue;
            // An eye-level camera can see the reverse of a held flat prop.
            // Depth testing still resolves its front and back surfaces.
            if(two_sided)face[4]|=1;
            if(!crosses_near && !crosses_far && !occluded_rail)continue;
            if(!(face[4]&1)) {
                const auto a=quad[0],b=quad[1],c=quad[2];
                const double facing=a.x*(b.y*c.z-b.z*c.y)+a.y*(b.z*c.x-b.x*c.z)+a.z*(b.x*c.y-b.y*c.x);
                // Retain native winding when a scene face crosses the eye.
                // Screen-space winding is undefined until after near clipping.
                if(facing<=0) {face[5]=-1;continue;}
            }
            const auto material=reinterpret_cast<const FirstPersonPropMaterial*>(g_base+0x451DA8+unsigned(face[5])*32);
            if(!CameraReadable(material,sizeof(*material)) || !isfinite(material->u0) || !isfinite(material->u1) ||
               !isfinite(material->v0) || !isfinite(material->v1))continue;
            quad[0].u=quad[3].u=material->u0;quad[1].u=quad[2].u=material->u1;
            quad[0].v=quad[1].v=material->v0;quad[2].v=quad[3].v=material->v1;
            const unsigned indices[2][3]={{0,1,2},{0,2,3}};
            for(const auto& index:indices) {
                const ff_camera::PropVertex triangle[]={quad[index[0]],quad[index[1]],quad[index[2]]};
                ff_camera::PropVertex far_polygon[4];
                unsigned count=3;
                if(occluded_rail)count=ff_camera::ClipPropPlaneTriangle(triangle,rail_plane,far_polygon);
                else if(crosses_far)count=ff_camera::ClipPropFarTriangle(triangle,far_depth,far_polygon);
                else for(unsigned j=0;j<3;++j)far_polygon[j]=triangle[j];
                for(unsigned j=2;j<count;++j) {
                    const ff_camera::PropVertex part[]={far_polygon[0],far_polygon[j-1],far_polygon[j]};
                    FirstPersonPropFragment fragment={};fragment.material=material;
                    fragment.count=ff_camera::ClipPropTriangle(part,projection,fragment.vertices);
                    if(fragment.count)fragments.push_back(fragment);
                }
            }
            face[5]=-1; // Emit the clipped fragments instead of dropping this entire face.
        }
    } catch(const std::bad_alloc&) {return native(source,flags,depth);}
    struct Guard {
        Guard(){g_first_person_projecting=true;}
        ~Guard(){g_first_person_projecting=false;}
    } guard;
    const DWORD alpha=floor_pass?0xFF000000:DWORD(CameraField<int>(0x8689EC)&255)<<24;
    FirstPersonPropModel copy=*source;copy.faces=faces.data();
    const int result=native(&copy,flags,depth);
    for(const auto& fragment:fragments) {
        auto& count=CameraField<unsigned>(0x1EC4D0);
        // Use the retail frame pool so sorted transparent polygons remain valid
        // until its normal flush. Never retain pointers into a temporary vector.
        if(count>=4096)break;
        auto polygon=reinterpret_cast<FirstPersonPropPolygon*>(g_base+0xC84C8+count++*0x124);
        *polygon={};polygon->count=fragment.count;polygon->kind=2;
        polygon->texture=fragment.material->texture;polygon->blend=fragment.material->blend;
        for(unsigned i=0;i<fragment.count;++i) {
            const auto& v=fragment.vertices[i];auto& out=polygon->vertices[i];
            out.x=float(ff_camera::Clamp(projection.center_x+v.x*projection.focal_x/v.z,0,projection.width));
            out.y=float(ff_camera::Clamp(projection.center_y+v.y*projection.focal_y/v.z,0,projection.height));
            out.z=float((v.z+depth)*projection.depth_scale);out.rhw=float(1/(v.z+depth));
            if(floor_pass) {
                // Ground is a painter-ordered background, not depth-tested
                // world geometry. Retain its sentinel for the far-depth guard.
                const DWORD floor_depth=0x3F7FFF58;memcpy(&out.z,&floor_depth,4);
            }
            out.color=alpha|0xFFFFFF;out.specular=0;out.u=float(v.u);out.v=float(v.v);
            int fog=0;
            if(v.z>CameraField<float>(0x6BC68)) {
                const int shift=(std::min)(31u,CameraField<unsigned>(0x6BC6C));
                fog=(std::min)(255,int(v.z-CameraField<float>(0x6BC68))>>shift);
            }
            if(CameraField<int>(0x6BC70))out.color=alpha|DWORD(255-fog)*0x010101;
            else if(CameraField<int>(0x6BC60))out.specular=DWORD(255-fog)<<24;
        }
        reinterpret_cast<void(__cdecl*)(FirstPersonPropPolygon*,const FirstPersonPropMaterial*,int)>(g_base+0x265B0)(
            polygon,fragment.material,floor_pass?flags:(CameraField<int>(0x9D188)^1));
    }
    return result;
}
static int FirstPersonProjectModel(const FirstPersonPropModel* source,int flags,int depth,
                                  bool two_sided,FirstPersonProjector native,bool floor_pass=false) {
    return CameraProjectNearModel(source,flags,depth,FirstPersonActive(),two_sided,native,floor_pass);
}
static int __cdecl FirstPersonHeldProp(const FirstPersonPropModel* source,int flags,int depth) {
    return FirstPersonProjectModel(source,flags,depth,true,
        reinterpret_cast<FirstPersonProjector>(g_base+0x26DE0));
}
static void (*g_held_weapon_pose)(const FirstPersonPropModel*,uintptr_t,int,unsigned);
static int __cdecl FirstPersonHeldPose(const FirstPersonPropModel* source,int flags,int depth,uintptr_t actor,int type,unsigned hand) {
    if(g_held_weapon_pose)g_held_weapon_pose(source,actor,type,hand);
    return FirstPersonHeldProp(source,flags,depth);
}
// The reviewed callers retain render actor + 0x62 in ESI and weapon type in EBX.
__declspec(naked) static void FirstPersonHeldRight() {
    __asm {
        push 1
        push ebx
        push esi
        push dword ptr [esp+24]
        push dword ptr [esp+24]
        push dword ptr [esp+24]
        call FirstPersonHeldPose
        add esp,24
        ret
    }
}
__declspec(naked) static void FirstPersonHeldLeft() {
    __asm {
        push 0
        push ebx
        push esi
        push dword ptr [esp+24]
        push dword ptr [esp+24]
        push dword ptr [esp+24]
        call FirstPersonHeldPose
        add esp,24
        ret
    }
}
