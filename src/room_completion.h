#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>

namespace ff_rooms {
enum class Room {None,Elevator,Office,LaboratoryLift,Laboratory,Ship,Mall};
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
        bool recognized=false;
        if(lift)recognized=count==184 && length==160 && box.x0==-923 && box.x1==922 && box.z0==-81 && box.z1==918;
        if(room==Room::Office)recognized=count==119 && length==93 && box.x0==-1037 && box.x1==1035 && box.z0>=-2700 && box.z0<=-2550 && box.z1>=-875 && box.z1<=-800;
        if(room==Room::LaboratoryLift) {
            // Shaft sections, lower/upper landings and their separate panel
            // shells. The moving platform is deliberately not a profile.
            recognized=((count==448 && length==330) || (count==432 && length==323) ||
                (count==439 && length==329) || (count==180 && length==111) || (count==412 && length==284)) &&
                box.x0>=-330 && box.x0<=-165 && box.x1>=2474 && box.x1<=2640 &&
                box.z0>=-330 && box.z0<=-165 && box.z1>=2474 && box.z1<=2475;
        }
        if(room==Room::Laboratory)recognized=count==272 && length==214 && box.x0==-1856 && box.x1==895 && box.z0==2255 && box.z1==7194;
        if(room==Room::Ship)recognized=count==287 && length==238 && box.x0==-1100 && box.x1==1099 && box.z0==424 && box.z1==6545;
        if(room==Room::Mall)recognized=count==185 && length==133 && box.x0==-2187 && box.x1==1562 && box.z0==0 && box.z1==3200;
        if(!recognized)return false;
        for(unsigned i=0;i<length;++i)if(polygons[i].index[3]>=0)
            for(int v:polygons[i].index)if(v<0 || unsigned(v)>=count)return false;
        vertices.assign(source,source+count);faces.assign(polygons,polygons+length);
        if(room==Room::Laboratory || room==Room::Ship || room==Room::Mall) {
            const unsigned labDonors[]={74,73,61,64},shipDonors[]={83,85,70,73},mallDonors[]={41,42};
            const auto donors=room==Room::Laboratory?labDonors:room==Room::Ship?shipDonors:mallDonors;
            const unsigned donorCount=room==Room::Mall?2:4;
            for(unsigned i=0;i<donorCount;++i)if(polygons[donors[i]].material<0 || polygons[donors[i]].index[3]<0) {
                vertices.clear();faces.clear();return false;
            }
            auto quad=[&](const Face& donor,Vertex a,Vertex b,Vertex c,Vertex d) {
                Face copy=donor;copy.flags&=~2;
                const Vertex points[]={a,b,c,d};
                for(unsigned i=0;i<4;++i){copy.index[i]=int16_t(vertices.size());vertices.push_back(points[i]);}
                faces.push_back(copy);++added;
            };
            // Tile each horizontal band at the installed panel scale. Taper
            // the end wall to the actual side-wall/ceiling junction rather
            // than placing a rectangle through the sloping ceiling.
            auto band=[&](unsigned donorIndex,int left0,int right0,int left1,int right1,int y0,int y1,int z,int tiles) {
                const auto& donor=polygons[donorIndex];
                for(int i=0;i<tiles;++i) {
                    const int x0=left0+(right0-left0)*i/tiles,x1=left0+(right0-left0)*(i+1)/tiles;
                    const int x2=left1+(right1-left1)*(i+1)/tiles,x3=left1+(right1-left1)*i/tiles;
                    quad(donor,{int16_t(x3),int16_t(y1),int16_t(z),0},{int16_t(x2),int16_t(y1),int16_t(z),0},
                        {int16_t(x1),int16_t(y0),int16_t(z),0},{int16_t(x0),int16_t(y0),int16_t(z),0});
                }
            };
            if(room==Room::Laboratory) {
                // Back bulkhead only. The original lower-Z access tunnel and
                // all side equipment bays remain native geometry.
                band(74,-1658,696,-1658,696,0,229,7192,8);
                band(73,-1658,696,-1607,645,229,458,7192,8);
                band(61,-1607,645,-1532,570,458,687,7192,8);
                band(64,-1532,570,-1314,353,687,876,7192,8);
                band(64,-1314,353,-481,-481,876,881,7192,1);
            } else if(room==Room::Ship) {
                // Continue the metal wall courses underneath the roof arch.
                // The entrance hatch at the other end is not reconstructed.
                band(83,-1004,1003,-1004,1003,0,234,6544,8);
                band(85,-1004,1003,-1004,1003,234,468,6544,8);
                band(70,-1004,1003,-1004,1003,468,701,6544,8);
                band(73,-1004,1003,-587,586,701,1064,6544,6);
                band(73,-587,586,0,0,1064,1124,6544,2);
            } else {
                // Keep the central corridor (-625..0) fully open. Complete
                // only its two flanks with matching upper/lower wall courses.
                for(int side=0;side<2;++side) {
                    const int left=side?0:-2187,right=side?1562:-625;
                    band(41,left,right,left,right,0,300,2913,5);
                    band(42,left,right,left,right,300,600,2913,5);
                }
            }
            return true;
        }
        if(room==Room::LaboratoryLift) {
            // The rear posts already exist. Continue only the opposite flat
            // panels and cross-members, with their native UVs and openings.
            // Do not duplicate the external landings, ladders or platform.
            for(unsigned i=0;i<length;++i) {
                const auto& face=polygons[i];if(face.material<0 || face.index[3]<0)continue;
                Vertex corners[4];for(unsigned j=0;j<4;++j)corners[j]=source[face.index[j]];
                const auto b=Extent(corners,4);
                if(b.x0<0 || b.x1>2310 || b.x0==b.x1 || b.y0==b.y1 ||
                    b.z0!=b.z1 || b.z0< -121 || b.z1> -43)continue;
                Face copy=face;
                for(unsigned j=0;j<4;++j) {
                    auto v=corners[j];v.x=int16_t(2310-int(v.x));v.z=int16_t(2310-int(v.z));
                    copy.index[j]=int16_t(vertices.size());vertices.push_back(v);
                }
                faces.push_back(copy);++added;
            }
            if(!added){vertices.clear();faces.clear();return false;}return true;
        }
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
