#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>

namespace ff_rooms {
enum class Room {None,Elevator,Office};
struct Vertex {int16_t x,y,z,pad;};
struct Face {int16_t index[4],flags,material;};
static_assert(sizeof(Vertex)==8 && sizeof(Face)==12,"Native model records");
struct Bounds {int x0=32767,x1=-32768,y0=32767,y1=-32768,z0=32767,z1=-32768;};
inline Bounds Extent(const Vertex* vertices,unsigned count) {
    Bounds b;
    for(unsigned i=0;i<count;++i) {
        const auto& v=vertices[i];b.x0=(std::min)(b.x0,int(v.x));b.x1=(std::max)(b.x1,int(v.x));
        b.y0=(std::min)(b.y0,int(v.y));b.y1=(std::max)(b.y1,int(v.y));
        b.z0=(std::min)(b.z0,int(v.z));b.z1=(std::max)(b.z1,int(v.z));
    }
    return b;
}
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    unsigned added=0;
    bool Build(Room room,const Vertex* source,unsigned count,const Face* polygons,unsigned length) {
        vertices.clear();faces.clear();added=0;
        if(!source || !polygons || room==Room::None || count>4096 || length>8192 || count<4 || !length)return false;
        const auto box=Extent(source,count);
        // Restrict reconstruction to the reviewed room silhouettes. Modified
        // or unrelated models retain their own authored geometry.
        const bool lift=room==Room::Elevator;
        // The native MOD loader halves all coordinates with an arithmetic
        // shift. These profiles and offsets use those loaded model units.
        if(lift?(count!=184 || length!=160 || box.x0!=-923 || box.x1!=922 || box.z0!=-81 || box.z1!=918):
            (count!=119 || length!=93 || box.x0!=-1037 || box.x1!=1035 || box.z0< -2700 || box.z0> -2550 || box.z1< -875 || box.z1> -800))return false;
        for(unsigned i=0;i<length;++i)if(polygons[i].index[3]>=0)
            for(int v:polygons[i].index)if(v<0 || unsigned(v)>=count)return false;
        vertices.assign(source,source+count);faces.assign(polygons,polygons+length);
        if(!lift) {
            // The missing office side faces the building interior. Reuse wood
            // from the side walls, not the opposite exterior window grid.
            const auto& wood=polygons[7];const auto& panel=polygons[10];const auto& trim=polygons[54];
            if(wood.material<0 || panel.material<0 || trim.material<0){vertices.clear();faces.clear();return false;}
            auto quad=[&](const Face& donor,Vertex a,Vertex b,Vertex c,Vertex d) {
                Face copy=donor;copy.flags&=~2; // Canonical UV orientation for the new rectangle.
                const Vertex points[]={a,b,c,d};
                for(unsigned i=0;i<4;++i){copy.index[i]=int16_t(vertices.size());vertices.push_back(points[i]);}
                faces.push_back(copy);++added;
            };
            const int z=box.z1-40;
            auto rectangle=[&](const Face& donor,int x0,int x1,int y0,int y1,int depth) {
                quad(donor,{int16_t(x0),int16_t(y1),int16_t(depth),0},{int16_t(x1),int16_t(y1),int16_t(depth),0},
                    {int16_t(x1),int16_t(y0),int16_t(depth),0},{int16_t(x0),int16_t(y0),int16_t(depth),0});
            };
            // Three native-sized bays on either side of a central closed door.
            const int edges[]={box.x0,-220,220,box.x1};
            for(unsigned side:{0u,2u})for(int bay=0;bay<3;++bay) {
                const int x0=edges[side]+(edges[side+1]-edges[side])*bay/3;
                const int x1=edges[side]+(edges[side+1]-edges[side])*(bay+1)/3;
                rectangle(panel,x0,x1,0,337,z);rectangle(panel,x0,x1,337,692,z);
                rectangle(wood,x0,x1,692,1058,z);
            }
            rectangle(wood,-220,220,620,1058,z);
            // A recessed pair of wooden leaves, with a narrow center seam.
            for(int side:{-1,1})for(int row=0;row<3;++row)
                rectangle(wood,side<0?-200:2,side<0?-2:200,row*200,(row+1)*200,z+12);
            rectangle(panel,-2,2,0,600,z+14);
            auto bar=[&](int x0,int x1,int y0,int y1,int front,int back) {
                rectangle(trim,x0,x1,y0,y1,front);
                quad(trim,{int16_t(x0),int16_t(y1),int16_t(back),0},{int16_t(x0),int16_t(y1),int16_t(front),0},
                    {int16_t(x0),int16_t(y0),int16_t(front),0},{int16_t(x0),int16_t(y0),int16_t(back),0});
                quad(trim,{int16_t(x1),int16_t(y1),int16_t(front),0},{int16_t(x1),int16_t(y1),int16_t(back),0},
                    {int16_t(x1),int16_t(y0),int16_t(back),0},{int16_t(x1),int16_t(y0),int16_t(front),0});
                quad(trim,{int16_t(x0),int16_t(y1),int16_t(back),0},{int16_t(x1),int16_t(y1),int16_t(back),0},
                    {int16_t(x1),int16_t(y1),int16_t(front),0},{int16_t(x0),int16_t(y1),int16_t(front),0});
                quad(trim,{int16_t(x0),int16_t(y0),int16_t(front),0},{int16_t(x1),int16_t(y0),int16_t(front),0},
                    {int16_t(x1),int16_t(y0),int16_t(back),0},{int16_t(x0),int16_t(y0),int16_t(back),0});
            };
            bar(-220,-200,0,620,z-8,z+12);bar(200,220,0,620,z-8,z+12);bar(-200,200,600,620,z-8,z+12);
            bar(-30,-23,265,320,z+4,z+12);bar(23,30,265,320,z+4,z+12);
            return true;
        }
        auto append=[&](const Face& face,int shift) {
            Face copy=face;
            // Continue the cabin side glazing through the new recessed bays,
            // preserving the installed glass materials and their blend rules.
            Vertex corners[4];for(unsigned i=0;i<4;++i)corners[i]=source[face.index[i]];
            const auto bounds=Extent(corners,4);
            if(((bounds.x0>=-608 && bounds.x1<=-383) || (bounds.x0>=382 && bounds.x1<=607)) && bounds.x1>bounds.x0) {
                const auto& glass=polygons[bounds.y1<=200?62:bounds.y0>=400?64:63];
                if(glass.material>=0){copy.material=glass.material;copy.flags=glass.flags;}
            }
            for(unsigned i=0;i<4;++i) {
                auto v=source[face.index[i]];
                v.x=int16_t(-int(v.x)-shift);
                v.z=int16_t(918-int(v.z));
                copy.index[i]=int16_t(vertices.size());vertices.push_back(v);
            }
            faces.push_back(copy);++added;
        };
        for(unsigned i=0;i<length;++i) {
            const auto& face=polygons[i];if(face.material<0 || face.index[3]<0)continue;
            Vertex quad[4];for(unsigned j=0;j<4;++j)quad[j]=source[face.index[j]];
            const auto b=Extent(quad,4);if(b.y1==b.y0)continue;
            // Reuse the interior posts and glazing profile on the opposite
            // cabin wall. Leave the entrance and its door/corridor untouched.
            if(b.z0<0 || b.z1>189 || b.y0<0 || b.y1>600 || b.x0< -765 || b.x1>765)continue;
            if(b.x1<=-225 || b.x0>=225)append(face,0);
            // Repeat a full-width bay twice instead of copying the entrance.
            if(b.x0>=-608 && b.x1<=-383){append(face,383);append(face,608);}
        }
        if(!added){vertices.clear();faces.clear();return false;}return true;
    }
};
}
