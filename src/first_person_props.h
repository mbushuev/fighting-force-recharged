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
static int __cdecl FirstPersonHeldProp(const FirstPersonPropModel* source,int flags,int depth) {
    auto native=reinterpret_cast<int(__cdecl*)(const FirstPersonPropModel*,int,int)>(g_base+0x26DE0);
    if(!FirstPersonActive() || depth || !CameraReadable(source,sizeof(*source)) ||
       !source->vertex_count || source->vertex_count>4096 || !source->face_count || source->face_count>8192 ||
       !CameraReadable(source->vertices,source->vertex_count*8) ||
       !CameraReadable(source->faces,source->face_count*12))return native(source,flags,depth);
    const auto matrix=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    const ff_camera::PropProjection projection={CameraField<float>(0xAD214),CameraField<float>(0xAD234),
        CameraField<float>(0x20C50C),CameraField<float>(0x20C508),
        CameraField<float>(0x9D170),CameraField<float>(0x9D174),CameraField<float>(0x5E8A4)};
    if(projection.focal_x<=0 || projection.focal_y<=0 || projection.width<=0 || projection.height<=0)
        return native(source,flags,depth);
    static thread_local std::vector<int16_t> faces;
    static thread_local std::vector<ff_camera::PropVertex> vertices;
    static thread_local std::vector<FirstPersonPropFragment> fragments;
    try {
        faces.assign(source->faces,source->faces+source->face_count*6);
        vertices.resize(source->vertex_count);fragments.clear();
        for(unsigned i=0;i<source->vertex_count;++i) {
            const auto v=source->vertices+i*4;
            vertices[i]={matrix[0]*double(v[0])+matrix[1]*double(v[1])+matrix[2]*double(v[2])+matrix[3],
                matrix[4]*double(v[0])+matrix[5]*double(v[1])+matrix[6]*double(v[2])+matrix[7],
                matrix[8]*double(v[0])+matrix[9]*double(v[1])+matrix[10]*double(v[2])+matrix[11],0,0};
        }
        for(unsigned i=0;i<source->face_count;++i) {
            auto face=faces.data()+i*6;
            if(face[5]<0 || face[3]<0)continue;
            bool valid=true,crosses_near=false;ff_camera::PropVertex quad[4];
            for(unsigned j=0;j<4;++j) {
                if(face[j]<0 || unsigned(face[j])>=source->vertex_count) {valid=false;break;}
                quad[j]=vertices[face[j]];
                if(!isfinite(quad[j].x) || !isfinite(quad[j].y) || !isfinite(quad[j].z)) {valid=false;break;}
                crosses_near|=quad[j].z<1;
            }
            if(!valid)continue;
            // An eye-level camera can see the reverse of a held flat prop.
            // Depth testing still resolves its front and back surfaces.
            face[4]|=1;
            if(!crosses_near)continue;
            const auto material=reinterpret_cast<const FirstPersonPropMaterial*>(g_base+0x451DA8+unsigned(face[5])*32);
            if(!CameraReadable(material,sizeof(*material)) || !isfinite(material->u0) || !isfinite(material->u1) ||
               !isfinite(material->v0) || !isfinite(material->v1))continue;
            quad[0].u=quad[3].u=material->u0;quad[1].u=quad[2].u=material->u1;
            quad[0].v=quad[1].v=material->v0;quad[2].v=quad[3].v=material->v1;
            const unsigned indices[2][3]={{0,1,2},{0,2,3}};
            for(const auto& index:indices) {
                const ff_camera::PropVertex triangle[]={quad[index[0]],quad[index[1]],quad[index[2]]};
                FirstPersonPropFragment fragment={};fragment.material=material;
                fragment.count=ff_camera::ClipPropTriangle(triangle,projection,fragment.vertices);
                if(fragment.count)fragments.push_back(fragment);
            }
            face[5]=-1; // Emit the clipped fragments instead of dropping this entire face.
        }
    } catch(const std::bad_alloc&) {return native(source,flags,depth);}
    const DWORD alpha=DWORD(CameraField<int>(0x8689EC)&255)<<24;
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
            out.z=float(v.z*projection.depth_scale);out.rhw=float(1/v.z);
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
            polygon,fragment.material,CameraField<int>(0x9D188)^1);
    }
    return result;
}
