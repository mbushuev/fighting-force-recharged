#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>

namespace ff_rooms {
enum class Room {None,Elevator,Office,LaboratoryLift,Laboratory,Ship,Mall,MallAtrium,BridgeStreet,SubwayEntrance,BronxBlock};
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
        if(room==Room::MallAtrium)recognized=count==203 && length==132 && box.x0==-1000 && box.x1==2250 && box.z0==0 && box.z1==4185;
        if(room==Room::BridgeStreet)recognized=count==331 && length==215 && box.x0==-3848 && box.x1==1650 && box.y0==0 && box.y1==1400 && box.z0==0 && box.z1==5600;
        if(room==Room::SubwayEntrance)recognized=count==286 && length==166 && box.x0==-1442 && box.x1==1442 && box.y0==0 && box.y1==862 && box.z0==-2026 && box.z1==2025;
        if(room==Room::BronxBlock)recognized=count==336 && length==202 && box.x0==-3848 && box.x1==1649 && box.y0==0 && box.y1==1750 && box.z0==-875 && box.z1==4028;
        if(!recognized)return false;
        for(unsigned i=0;i<length;++i)if(polygons[i].index[3]>=0)
            for(int v:polygons[i].index)if(v<0 || unsigned(v)>=count)return false;
        vertices.assign(source,source+count);faces.assign(polygons,polygons+length);
        if(room==Room::BronxBlock) {
            const unsigned donors[]={25,21,151,156};
            for(unsigned i:donors)if(polygons[i].material<0 || polygons[i].index[3]<0) {
                vertices.clear();faces.clear();return false;
            }
            // BR02A's shop projects past the adjoining low brick frontage.
            // Close only its exposed return at the step, using the existing
            // five courses and the outside depth of its authored opposite end.
            // Follow the slightly sloped window edge instead of a rectangular
            // approximation that leaves thin gaps at the upper joins.
            auto band=[&](unsigned donor,int left,int bottomRight,int topRight,int bottom,int top,int tiles) {
                for(int i=0;i<tiles;++i) {
                    const int a=left+(bottomRight-left)*i/tiles,b=left+(bottomRight-left)*(i+1)/tiles;
                    const int c=left+(topRight-left)*i/tiles,d=left+(topRight-left)*(i+1)/tiles;
                    const Vertex points[]={{int16_t(c),int16_t(top),2275,0},{int16_t(d),int16_t(top),2275,0},
                        {int16_t(b),int16_t(bottom),2275,0},{int16_t(a),int16_t(bottom),2275,0}};
                    Face copy=polygons[donor];copy.flags&=~2;
                    for(unsigned k=0;k<4;++k){copy.index[k]=int16_t(vertices.size());vertices.push_back(points[k]);}
                    faces.push_back(copy);++added;
                }
            };
            band(25,-2748,-2198,-2198,0,175,3);
            band(21,-2748,-2198,-2198,175,350,3);
            band(151,-2748,-2198,-2167,350,700,2);
            band(156,-3298,-2167,-2177,700,1050,3);
            band(156,-3298,-2177,-2169,1050,1400,3);
            return true;
        }
        if(room==Room::SubwayEntrance) {
            // SUBENT1 has tiled pilasters with only one authored return. Copy
            // each installed height band to the missing side, preserving the
            // panel widths, tile/stripe alignment and sloping cornice. The
            // first pair already has its two lower bands on both sides.
            struct Return {unsigned face;int front,back,top;};
            const Return returns[]={
                {21,-669,-669,-669},{25,-669,-669,-669},{29,-669,-669,-669},{67,-669,-669,-669},
                {51,802,800,800},{57,802,800,800},{63,802,800,800},{69,802,800,800},
                {76,-1890,-1888,-1890},{79,-1890,-1888,-1890},
                {102,-698,-698,-698},{106,-698,-698,-698},{110,-698,-698,-698},{148,-698,-698,-698},
                {132,773,771,771},{138,773,771,771},{144,773,771,771},{150,773,771,771},
                {157,-1919,-1917,-1919},{160,-1919,-1917,-1919}
            };
            for(const auto& panel:returns)if(polygons[panel.face].material<0 || polygons[panel.face].index[3]<0) {
                vertices.clear();faces.clear();return false;
            }
            for(const auto& panel:returns) {
                Face copy=polygons[panel.face];copy.flags&=~2;
                for(unsigned i=0;i<4;++i) {
                    auto v=source[copy.index[i]];
                    const bool front=v.x>-1300 && v.x<1300;
                    v.z=int16_t(front?(v.y==862?panel.top:panel.front):panel.back);
                    copy.index[i]=int16_t(vertices.size());vertices.push_back(v);
                }
                faces.push_back(copy);++added;
            }
            return true;
        }
        if(room==Room::BridgeStreet) {
            const unsigned donors[]={65,76,75,66,85,90,95,173,187,188,189,157,115,135,148,153,170};
            for(unsigned i:donors)if(polygons[i].material<0 || polygons[i].index[3]<0) {
                vertices.clear();faces.clear();return false;
            }
            auto quad=[&](unsigned donor,Vertex a,Vertex b,Vertex c,Vertex d) {
                Face copy=polygons[donor];copy.flags&=~2;
                const Vertex points[]={a,b,c,d};
                for(unsigned i=0;i<4;++i){copy.index[i]=int16_t(vertices.size());vertices.push_back(points[i]);}
                faces.push_back(copy);++added;
            };
            // BR01 ends beside a transverse street, not a missing room wall.
            // Resume the brick and concrete blocks beyond its far pavement.
            // The native ground map already continues both roads through this
            // intersection; do not cover it with another coplanar floor.
            constexpr int start=8216,end=12416;
            auto course=[&](bool brick,int x0,int x1,int z0,int z1) {
                auto panel=[&](unsigned donor,int bottom,int top,int from=0,int to=2) {
                    const int a=x0+(x1-x0)*from/2,b=x0+(x1-x0)*to/2;
                    const int c=z0+(z1-z0)*from/2,d=z0+(z1-z0)*to/2;
                    quad(donor,{int16_t(a),int16_t(top),int16_t(c),0},{int16_t(b),int16_t(top),int16_t(d),0},
                        {int16_t(b),int16_t(bottom),int16_t(d),0},{int16_t(a),int16_t(bottom),int16_t(c),0});
                };
                panel(brick?65:173,0,175,0,1);panel(brick?76:187,0,175,1,2);
                panel(brick?75:188,175,350,0,1);panel(brick?66:189,175,350,1,2);
                panel(brick?85:157,350,700);
                if(brick){panel(90,700,1050);panel(95,1050,1400);}
            };
            for(int side=0;side<2;++side) {
                const bool brick=side!=0;
                const int x=brick?-550:-2748,height=brick?1400:700;
                for(int bay=0;bay<12;++bay)course(brick,x,x,start+350*bay,start+350*(bay+1));
                // Corner returns run away from the road. Its full central
                // width and the nearer crossing remain open in both views.
                const int outside=brick?1650:-3848;
                const int tiles=brick?6:3;
                for(int bay=0;bay<tiles;++bay) {
                    const int a=x+(outside-x)*bay/tiles,b=x+(outside-x)*(bay+1)/tiles;
                    course(brick,a,b,start,start);
                    for(int z=start;z<end;z+=350)quad(157,
                        {int16_t(a),int16_t(height),int16_t(z),0},
                        {int16_t(b),int16_t(height),int16_t(z),0},
                        {int16_t(b),int16_t(height),int16_t(z+350),0},
                        {int16_t(a),int16_t(height),int16_t(z+350),0});
                }
            }
            // Nearer BR01 facades also omit returns on the side hidden by
            // the original camera. Join their actual stepped outlines; the
            // central road and the transverse street stay open. Tile each
            // course instead of stretching one texture across a building.
            auto returnBand=[&](unsigned donor,int left0,int right0,int left1,int right1,
                                int bottom,int top,int z,int tiles) {
                for(int i=0;i<tiles;++i) {
                    const int a=left0+(right0-left0)*i/tiles,b=left0+(right0-left0)*(i+1)/tiles;
                    const int c=left1+(right1-left1)*i/tiles,d=left1+(right1-left1)*(i+1)/tiles;
                    quad(donor,{int16_t(c),int16_t(top),int16_t(z),0},{int16_t(d),int16_t(top),int16_t(z),0},
                        {int16_t(b),int16_t(bottom),int16_t(z),0},{int16_t(a),int16_t(bottom),int16_t(z),0});
                }
            };
            // Brick frontage steps back at the foot of the first tower.
            returnBand(115,-550,0,-550,0,0,175,700,3);
            returnBand(135,-550,0,-550,0,175,350,700,3);
            returnBand(148,-550,0,-550,0,350,700,700,2);
            returnBand(153,-550,550,-550,550,700,1050,700,3);
            // The opposite shop has a recessed plinth. Preserve its soffit
            // and meet the slightly sloped edge of the upper window course.
            returnBand(173,-3023,-2748,-3023,-2748,0,175,2449,2);
            returnBand(188,-3298,-2748,-3298,-2748,175,350,2449,3);
            returnBand(157,-3298,-2748,-3298,-2748,350,700,2449,2);
            returnBand(170,-3848,-2748,-3848,-2718,700,1050,2449,3);
            // Close the native block ends at the near curb, separately from
            // the already extended blocks beyond the intersection.
            returnBand(65,-550,1650,-550,1650,0,175,5600,12);
            returnBand(75,-550,1650,-550,1650,175,350,5600,12);
            returnBand(85,-550,1650,-550,1650,350,700,5600,6);
            returnBand(90,-550,1650,-535,1650,700,1050,5600,6);
            returnBand(95,-535,1650,-524,1650,1050,1400,5600,6);
            returnBand(173,-3848,-2748,-3848,-2748,0,175,5600,6);
            returnBand(188,-3848,-2748,-3848,-2748,175,350,5600,6);
            returnBand(157,-3848,-2748,-3848,-2748,350,700,5600,3);
            return true;
        }
        if(room==Room::Laboratory || room==Room::MallAtrium) {
            const unsigned labDonors[]={0,1,2,96,103,104,105,199,206,207,204,205,201,99};
            const unsigned mallDonors[]={19,36,95,70,120,94,5};
            const bool lab=room==Room::Laboratory;
            const unsigned* donors=lab?labDonors:mallDonors;
            const unsigned donorCount=lab?unsigned(sizeof(labDonors)/sizeof(*labDonors)):unsigned(sizeof(mallDonors)/sizeof(*mallDonors));
            for(unsigned i=0;i<donorCount;++i)if(polygons[donors[i]].material<0 || polygons[donors[i]].index[3]<0) {
                vertices.clear();faces.clear();return false;
            }
            auto quad=[&](unsigned donor,Vertex a,Vertex b,Vertex c,Vertex d) {
                Face copy=polygons[donor];copy.flags&=~2;
                const Vertex points[]={a,b,c,d};
                for(unsigned i=0;i<4;++i){copy.index[i]=int16_t(vertices.size());vertices.push_back(points[i]);}
                faces.push_back(copy);++added;
            };
            auto wall=[&](unsigned donor,int x0,int x1,int y0,int y1,int z) {
                quad(donor,{int16_t(x0),int16_t(y1),int16_t(z),0},{int16_t(x1),int16_t(y1),int16_t(z),0},
                    {int16_t(x1),int16_t(y0),int16_t(z),0},{int16_t(x0),int16_t(y0),int16_t(z),0});
            };
            auto side=[&](unsigned donor,int x,int y0,int y1,int z0,int z1) {
                quad(donor,{int16_t(x),int16_t(y1),int16_t(z0),0},{int16_t(x),int16_t(y1),int16_t(z1),0},
                    {int16_t(x),int16_t(y0),int16_t(z1),0},{int16_t(x),int16_t(y0),int16_t(z0),0});
            };
            auto horizontal=[&](unsigned donor,int x0,int x1,int y,int z0,int z1) {
                quad(donor,{int16_t(x0),int16_t(y),int16_t(z0),0},{int16_t(x1),int16_t(y),int16_t(z0),0},
                    {int16_t(x1),int16_t(y),int16_t(z1),0},{int16_t(x0),int16_t(y),int16_t(z1),0});
            };
            if(lab) {
                // Reuse the entrance arch at its original scale and with its
                // original UV ordering. The rear recess has separate jambs,
                // lintel and metal leaves instead of stretched side-wall tiles.
                for(unsigned donor:{0u,1u,2u,96u,103u,104u,105u,199u,206u,207u}) {
                    Face copy=polygons[donor];
                    for(unsigned j=0;j<4;++j) {
                        auto v=source[copy.index[j]];v.x=int16_t(-962-int(v.x));v.z=int16_t(10087-int(v.z));
                        copy.index[j]=int16_t(vertices.size());vertices.push_back(v);
                    }
                    faces.push_back(copy);++added;
                }
                for(int x:{-1270,308}) {
                    side(204,x,0,231,7191,7525);side(205,x,231,458,7191,7525);
                    side(99,x,458,687,7191,7525);
                }
                for(int x=-1270;x<308;x+=263) {
                    horizontal(99,x,x+263,687,7191,7525);
                    horizontal(204,x,x+263,0,7191,7525);
                    wall(207,x,x+263,458,687,7525);
                }
                for(int x:{-1270,45}) {
                    wall(204,x,x+263,0,231,7525);wall(205,x,x+263,231,458,7525);
                }
                // Corrugated leaves and a narrow dark center joint. The
                // doorway is decorative; native entrances and scripts stay put.
                wall(201,-1007,-483,0,458,7541);wall(201,-479,45,0,458,7541);
                wall(99,-483,-479,0,458,7543);
                for(int x:{-1007,45})side(99,x,0,458,7525,7543);
                horizontal(99,-1007,45,458,7525,7543);
                horizontal(204,-1007,45,0,7525,7543);
            } else {
                // MALL01 is the entrance atrium, not the later MALL02A
                // corridor. Its rear opening is beneath the balcony beam.
                // Extend it into a shallow service vestibule, leaving the
                // native entrance and all connections at the other end open.
                for(int x:{0,1250})for(int bay=0;bay<2;++bay) {
                    side(19,x,0,300,4185+bay*200,4385+bay*200);
                    side(36,x,300,600,4185+bay*200,4385+bay*200);
                }
                for(int x=0;x<1250;x+=250) {
                    horizontal(70,x,x+250,600,4185,4585);
                    horizontal(95,x,x+250,0,4185,4585);
                }
                for(int x:{0,250,1000}) {
                    const int right=x==250?375:x+250;
                    wall(19,x,right,0,300,4585);wall(36,x,right,300,600,4585);
                }
                wall(19,875,1000,0,300,4585);wall(36,875,1000,300,600,4585);
                wall(36,375,875,500,600,4585);
                wall(5,375,623,0,500,4601);wall(5,627,875,0,500,4601);
                wall(95,623,627,0,500,4603);
                for(int x:{375,875})side(94,x,0,500,4585,4603);
                horizontal(94,375,875,500,4585,4603);
                horizontal(95,375,875,0,4585,4603);
                // Continue the upper gallery behind the existing beam. The
                // central void below remains the recessed doorway, not a wall.
                for(int x=-1000;x<2250;x+=250)wall(120,x,x+250,600,1400,4185);
            }
            return true;
        }
        if(room==Room::Ship || room==Room::Mall) {
            const unsigned shipDonors[]={83,85,70,73,3,1},mallDonors[]={41,42};
            const auto donors=room==Room::Ship?shipDonors:mallDonors;
            const unsigned donorCount=room==Room::Mall?2:6;
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
            if(room==Room::Ship) {
                // Leave a recognizable entrance in the bulkhead. Its closed
                // steel leaves sit behind a hazard-striped frame; the native
                // hatch and scripted route at the other end remain untouched.
                for(int side=0;side<2;++side) {
                    const int left=side?360:-1004,right=side?1003:-360;
                    band(83,left,right,left,right,0,234,6544,3);
                    band(85,left,right,left,right,234,468,6544,3);
                    band(70,left,right,left,right,468,701,6544,3);
                }
                band(73,-360,360,-360,360,660,701,6544,3);
                band(73,-1004,1003,-587,586,701,1064,6544,6);
                band(73,-587,586,0,0,1064,1124,6544,2);
                for(int side=0;side<2;++side) {
                    const int left=side?4:-330,right=side?330:-4;
                    band(83,left,right,left,right,0,220,6584,1);
                    band(85,left,right,left,right,220,440,6584,1);
                    band(73,left,right,left,right,440,630,6584,1);
                    const int x0=side?330:-360,x1=x0+30;
                    band(3,x0,x1,x0,x1,0,660,6540,1);
                    const int x=side?330:-330;
                    quad(polygons[73],{int16_t(x),630,6540,0},{int16_t(x),630,6586,0},
                        {int16_t(x),0,6586,0},{int16_t(x),0,6540,0});
                    const int handle=side?40:-60;
                    band(73,handle,handle+20,handle,handle+20,270,350,6572,1);
                    for(int hx:{handle,handle+20})quad(polygons[73],
                        {int16_t(hx),350,6572,0},{int16_t(hx),350,6584,0},
                        {int16_t(hx),270,6584,0},{int16_t(hx),270,6572,0});
                }
                band(3,-330,330,-330,330,630,660,6540,2);
                band(1,-4,4,-4,4,0,630,6586,1);
                quad(polygons[73],{-330,630,6540,0},{330,630,6540,0},{330,630,6586,0},{-330,630,6586,0});
                quad(polygons[83],{-330,0,6586,0},{330,0,6586,0},{330,0,6540,0},{-330,0,6540,0});
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
