#include "../src/dinput_proxy.cpp"
#include <fstream>
#include <iterator>
#include <set>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
using Vertex=ff_rooms::Vertex;using Face=ff_rooms::Face;
struct Source {
    std::vector<Vertex> vertices;std::vector<Face> faces;
    CameraRetailModel Model()const{return {reinterpret_cast<const int16_t*>(vertices.data()),unsigned(vertices.size()),reinterpret_cast<const int16_t*>(faces.data()),unsigned(faces.size())};}
};
static Source Fixture(bool lift) {
    Source s;s.vertices.resize(lift?184:119);s.faces.resize(lift?160:93);
    for(auto& face:s.faces)face.material=-1;
    const auto quad=[&](unsigned row,int x0,int x1,int y0,int y1,int z,int material) {
        const unsigned start=row*4;
        s.vertices[start]={int16_t(x0),int16_t(y0),int16_t(z),0};s.vertices[start+1]={int16_t(x1),int16_t(y0),int16_t(z),0};
        s.vertices[start+2]={int16_t(x1),int16_t(y1),int16_t(z),0};s.vertices[start+3]={int16_t(x0),int16_t(y1),int16_t(z),0};
        s.faces[row]={{int16_t(start),int16_t(start+1),int16_t(start+2),int16_t(start+3)},0,int16_t(material)};
    };
    if(lift) {
        quad(0,-765,-225,0,600,0,0);quad(1,225,765,0,600,0,1);quad(2,-608,-383,0,600,0,2);
        s.vertices[182]={-923,-140,-81,0};s.vertices[183]={922,700,918,0};
    } else {
        quad(0,-1037,1035,0,1057,-2567,3);
        s.vertices[118]={1035,0,-829,0};
        for(unsigned i=4;i<118;++i)s.vertices[i]={0,0,-2000,0};
        for(unsigned face:{7u,10u,54u})s.faces[face]=s.faces[0];
    }
    return s;
}
static bool Covers(const ff_rooms::Mesh& mesh,unsigned first,double x,double y,double near_z=-32768,double far_z=32767) {
    const auto area=[](double ax,double ay,double bx,double by,double px,double py){return (bx-ax)*(py-ay)-(by-ay)*(px-ax);};
    const int indices[][3]={{0,1,2},{0,2,3}};
    for(unsigned i=first;i<mesh.faces.size();++i)for(const auto& t:indices) {
        const auto& f=mesh.faces[i];if(f.material<0 || f.index[3]<0)continue;
        const auto a=mesh.vertices[f.index[t[0]]],b=mesh.vertices[f.index[t[1]]],c=mesh.vertices[f.index[t[2]]];
        const double total=area(a.x,a.y,b.x,b.y,c.x,c.y);if(fabs(total)<1)continue;
        double ab=area(a.x,a.y,b.x,b.y,x,y),bc=area(b.x,b.y,c.x,c.y,x,y),ca=area(c.x,c.y,a.x,a.y,x,y);
        const double z=(bc*a.z+ca*b.z+ab*c.z)/total;
        if(z>=near_z && z<=far_z && ((ab>=-.01 && bc>=-.01 && ca>=-.01)||(ab<=.01 && bc<=.01 && ca<=.01)))return true;
    }
    return false;
}
static void CheckMesh(const Source& source,ff_rooms::Room room) {
    ff_rooms::Mesh mesh;CHECK(mesh.Build(room,source.vertices.data(),unsigned(source.vertices.size()),source.faces.data(),unsigned(source.faces.size())));
    if(!mesh.added)return;
    CHECK(!memcmp(mesh.vertices.data(),source.vertices.data(),source.vertices.size()*sizeof(Vertex)));
    CHECK(!memcmp(mesh.faces.data(),source.faces.data(),source.faces.size()*sizeof(Face)));
    std::set<int> materials;for(const auto& f:source.faces)materials.insert(f.material);
    CHECK(mesh.vertices.size()==source.vertices.size()+mesh.added*4 && mesh.faces.size()==source.faces.size()+mesh.added);
    for(unsigned i=unsigned(source.faces.size());i<mesh.faces.size();++i) {
        const auto& f=mesh.faces[i];CHECK(materials.count(f.material)==1);
        for(int index:f.index)CHECK(index>=0 && unsigned(index)<mesh.vertices.size());
        for(int index:f.index)if(room==ff_rooms::Room::Elevator)CHECK(mesh.vertices[index].z>=729 && mesh.vertices[index].z<=918);
    }
    if(room==ff_rooms::Room::Office) {
        // A wooden interior wall and closed door must not borrow the exterior
        // window-grid materials, regardless of stage-specific material IDs.
        const std::set<int> interior={source.faces[7].material,source.faces[10].material,source.faces[54].material};
        for(unsigned i=unsigned(source.faces.size());i<mesh.faces.size();++i)CHECK(interior.count(mesh.faces[i].material)==1);
    }
    if(room==ff_rooms::Room::Elevator) {
        for(unsigned donor:{62u,63u,64u})if(source.faces[donor].material>=0) {
            unsigned panes=0;for(unsigned i=unsigned(source.faces.size());i<mesh.faces.size();++i)
                panes+=mesh.faces[i].material==source.faces[donor].material;
            CHECK(panes>=4);
        }
        int gaps=0;
        for(int x=-725;x<750;x+=50)for(int y:{15,175,225,375,425,575})if(!Covers(mesh,unsigned(source.faces.size()),x,y)) {
            if(gaps++<8)printf("Elevator gap at %d %d\n",x,y);
        }
        CHECK(gaps==0);
    } else {
        int gaps=0;
        // Forward rays include the native side-wall returns and bench bases,
        // which meet the reconstructed office wall at its lower corners.
        for(int x=-900;x<=900;x+=100)for(int y:{15,75,175,400,700,1000})if(!Covers(mesh,0,x,y,-1500,-750)) {
            if(gaps++<8)printf("Office gap at %d %d\n",x,y);
        }
        CHECK(gaps==0);
    }
}
static uint32_t Big(const BYTE* p){return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3];}
static std::vector<BYTE> ReadAsset(const std::string& wad,const char* wanted) {
    std::ifstream file(wad,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)),{});
    CHECK(bytes.size()>=4);if(bytes.size()<4)return {};const auto count=Big(bytes.data());
    CHECK(count<4096 && 4+count*20<=bytes.size());if(count>=4096 || 4+count*20>bytes.size())return {};
    for(unsigned i=0;i<count;++i) {
        const auto entry=bytes.data()+4+i*20;char name[13]={};memcpy(name,entry,12);if(strcmp(name,wanted))continue;
        const unsigned size=Big(entry+12),offset=Big(entry+16);CHECK(size>=20 && uint64_t(offset)+size<=bytes.size());if(size<20 || uint64_t(offset)+size>bytes.size())return {};
        return {bytes.begin()+offset,bytes.begin()+offset+size};
    }
    CHECK(false);return {};
}
static Source ReadModel(const std::string& wad,const char* wanted) {
    const auto asset=ReadAsset(wad,wanted);Source s;if(asset.size()<20)return s;
    const auto data=asset.data();int fields[5];memcpy(fields,data,20);
    CHECK(fields[1]==16 && fields[2]>0 && fields[4]>0 && fields[3]>16);if(fields[2]<=0 || fields[4]<=0)return s;
    const bool bounded=fields[1]>=0 && fields[3]>=0 && uint64_t(4)+fields[1]+uint64_t(fields[2])*8<=asset.size() &&
        uint64_t(4)+fields[3]+uint64_t(fields[4])*12<=asset.size();CHECK(bounded);if(!bounded)return s;
        s.vertices.resize(fields[2]);s.faces.resize(fields[4]);memcpy(s.vertices.data(),data+4+fields[1],s.vertices.size()*8);memcpy(s.faces.data(),data+4+fields[3],s.faces.size()*12);
        // Match the native loader, including rounding negative odd coordinates.
        for(auto& v:s.vertices){v.x>>=1;v.y>>=1;v.z>>=1;}
        for(auto& f:s.faces) {
            for(unsigned i=0;i<(f.index[3]==-1?2u:4u);++i){CHECK(f.index[i]%4==0);f.index[i]>>=2;}
            f.material=int16_t(f.material/6+((f.flags&2)?1:0));
        }
    CHECK(!s.vertices.empty());return s;
}
static void CheckLaboratoryLift(const Source& source,int type) {
    ff_rooms::Mesh mesh;
    CHECK(mesh.Build(ff_rooms::Room::LaboratoryLift,source.vertices.data(),unsigned(source.vertices.size()),source.faces.data(),unsigned(source.faces.size())));
    if(!mesh.added)return;
    CHECK(!memcmp(mesh.vertices.data(),source.vertices.data(),source.vertices.size()*sizeof(Vertex)));
    CHECK(!memcmp(mesh.faces.data(),source.faces.data(),source.faces.size()*sizeof(Face)));
    // Each added panel must be an exact half-turn of an installed front
    // panel, including the material and UV flags. No floor or landing copies.
    for(unsigned i=unsigned(source.faces.size());i<mesh.faces.size();++i) {
        const auto& copy=mesh.faces[i];bool found=false;
        for(const auto& original:source.faces) {
            if(original.material!=copy.material || original.flags!=copy.flags || original.index[3]<0)continue;
            bool same=true;
            for(int k=0;k<4;++k) {
                const auto a=source.vertices[original.index[k]],b=mesh.vertices[copy.index[k]];
                same&=b.x==2310-a.x && b.y==a.y && b.z==2310-a.z;
                CHECK(b.z>=2353 && b.z<=2431);
            }
            if(same){found=true;break;}
        }
        CHECK(found);
    }
    // The bottom and top liners close the full seven-panel width. Intermediate
    // shaft parts retain the native gaps between structural cross-members.
    const unsigned first=unsigned(source.faces.size());
    if(type==440 || type==442)for(int x=100;x<2300;x+=200)for(int y:{50,700,1400})CHECK(Covers(mesh,first,x,y));
    if(type==437 || type==441) {
        CHECK(Covers(mesh,first,1155,1000));CHECK(!Covers(mesh,first,1155,1300));
    }
    if(type==438){CHECK(Covers(mesh,first,1155,350));CHECK(!Covers(mesh,first,1155,1300));}
    printf("Laboratory lift %d: %u added faces\n",type,mesh.added);
}
static void CheckInterior(const Source& source,ff_rooms::Room room) {
    ff_rooms::Mesh mesh;
    auto unavailable=source;
    unavailable.faces[room==ff_rooms::Room::Laboratory?201:room==ff_rooms::Room::Ship?83:room==ff_rooms::Room::MallAtrium?19:41].material=-1;
    CHECK(!mesh.Build(room,unavailable.vertices.data(),unsigned(unavailable.vertices.size()),unavailable.faces.data(),unsigned(unavailable.faces.size())));
    CHECK(mesh.vertices.empty() && mesh.faces.empty());
    CHECK(mesh.Build(room,source.vertices.data(),unsigned(source.vertices.size()),source.faces.data(),unsigned(source.faces.size())));
    if(!mesh.added)return;
    CHECK(!memcmp(mesh.vertices.data(),source.vertices.data(),source.vertices.size()*sizeof(Vertex)));
    CHECK(!memcmp(mesh.faces.data(),source.faces.data(),source.faces.size()*sizeof(Face)));
    const unsigned first=unsigned(source.faces.size());
    std::set<int> materials;for(const auto& f:source.faces)materials.insert(f.material);
    for(unsigned i=first;i<mesh.faces.size();++i) {
        const auto& face=mesh.faces[i];CHECK(face.material>=0 && materials.count(face.material));
        for(int index:face.index)CHECK(index>=0 && unsigned(index)<mesh.vertices.size());
        const int near_z=room==ff_rooms::Room::Laboratory?7191:room==ff_rooms::Room::MallAtrium?4185:room==ff_rooms::Room::Ship?6540:2913;
        const int far_z=room==ff_rooms::Room::Laboratory?7543:room==ff_rooms::Room::MallAtrium?4603:room==ff_rooms::Room::Ship?6586:near_z;
        for(int index:face.index)CHECK(mesh.vertices[index].z>=near_z && mesh.vertices[index].z<=far_z);
    }
    if(room==ff_rooms::Room::Laboratory) {
        for(int x=-1500;x<600;x+=100)for(int y:{10,200,400,600})CHECK(Covers(mesh,first,x,y));
        CHECK(Covers(mesh,first,-481,879));CHECK(!Covers(mesh,first,-1500,875));
        // The opening has real depth; only the recessed door closes its
        // center. The arch retains the native panel scale and UV ordering.
        CHECK(!Covers(mesh,first,-481,200,7191,7524));
        CHECK(Covers(mesh,first,-481,200,7541,7543));
        unsigned copy=first;
        for(unsigned donor:{0u,1u,2u,96u,103u,104u,105u,199u,206u,207u}) {
            const auto& original=source.faces[donor];const auto& face=mesh.faces[copy++];
            CHECK(face.material==original.material && face.flags==original.flags);
            for(int i=0;i<4;++i) {
                const auto a=source.vertices[original.index[i]],b=mesh.vertices[face.index[i]];
                CHECK(b.x==-962-a.x && b.y==a.y && b.z==10087-a.z);
            }
        }
    } else if(room==ff_rooms::Room::MallAtrium) {
        // Close the entrance atrium's under-balcony void without filling the
        // vestibule itself or the native entrance at the other end.
        for(int x=25;x<1250;x+=50)for(int y:{10,299,301,490,590})CHECK(Covers(mesh,first,x,y));
        for(int x=-975;x<2250;x+=50)for(int y:{610,1000,1390})CHECK(Covers(mesh,first,x,y));
        CHECK(!Covers(mesh,first,625,250,4185,4584));
        CHECK(Covers(mesh,first,625,250,4601,4603));
        CHECK(!Covers(mesh,first,625,250,-1,1));
    } else if(room==ff_rooms::Room::Ship) {
        for(int x=-950;x<1000;x+=100)for(int y:{10,230,450,690})CHECK(Covers(mesh,first,x,y));
        CHECK(Covers(mesh,first,0,1120));CHECK(!Covers(mesh,first,950,1000));
        CHECK(!Covers(mesh,first,0,300,6540,6583));
        CHECK(Covers(mesh,first,0,300,6584,6586));
        CHECK(Covers(mesh,first,345,300,6540,6544));
        CHECK(!Covers(mesh,first,0,300,423,425)); // Existing entrance is untouched.
    } else {
        for(int x=-2100;x<1500;x+=100)for(int y:{10,299,301,590})
            CHECK(Covers(mesh,first,x,y)==(x<=-625 || x>=0));
        CHECK(!Covers(mesh,first,-313,300));
    }
}
static void CheckBridgeStreet(const Source& source,const std::string& wad) {
    ff_rooms::Mesh mesh;const auto room=ff_rooms::Room::BridgeStreet;
    auto changed=source;changed.faces[65].material=-1;
    CHECK(!mesh.Build(room,changed.vertices.data(),unsigned(changed.vertices.size()),changed.faces.data(),unsigned(changed.faces.size())));
    CHECK(mesh.vertices.empty() && mesh.faces.empty());
    CHECK(!mesh.Build(room,source.vertices.data(),unsigned(source.vertices.size())-1,source.faces.data(),unsigned(source.faces.size())));
    CHECK(mesh.Build(room,source.vertices.data(),unsigned(source.vertices.size()),source.faces.data(),unsigned(source.faces.size())));
    if(!mesh.added)return;
    CHECK(mesh.vertices.size()<4096);
    CHECK(!memcmp(mesh.vertices.data(),source.vertices.data(),source.vertices.size()*sizeof(Vertex)));
    CHECK(!memcmp(mesh.faces.data(),source.faces.data(),source.faces.size()*sizeof(Face)));
    const unsigned first=unsigned(source.faces.size());
    std::set<int> materials;for(const auto& f:source.faces)materials.insert(f.material);
    for(unsigned i=first;i<mesh.faces.size();++i) {
        const auto& f=mesh.faces[i];CHECK(materials.count(f.material));
        int low=32767,high=-32768;
        for(int index:f.index) {
            CHECK(index>=0 && unsigned(index)<mesh.vertices.size());
            const auto v=mesh.vertices[index];low=(std::min)(low,int(v.x));high=(std::max)(high,int(v.x));
            CHECK(v.y>=0 && v.z>=8216 && v.z<=12416);
        }
        // No added face crosses the road or fills the transverse street.
        CHECK(high<=-2748 || low>=-550);
    }
    CHECK(Covers(mesh,first,-3300,350));CHECK(Covers(mesh,first,300,1200));
    for(int x=-2700;x<-550;x+=100)for(int y:{10,350,700,1390})CHECK(!Covers(mesh,first,x,y));
    // Test rays into both longitudinal facades, including both ends and the
    // joins between native-sized panels, by exchanging the ray's X/Z axes.
    auto sides=mesh;for(auto& v:sides.vertices)std::swap(v.x,v.z);
    for(int z=8250;z<12416;z+=125) {
        for(int y:{10,174,176,349,351,699})CHECK(Covers(sides,first,z,y,-2749,-2747));
        for(int y:{10,174,176,349,351,699,701,1049,1051,1399})CHECK(Covers(sides,first,z,y,-551,-549));
    }
    CHECK(!Covers(sides,first,6500,350)); // Existing cross-street remains open.
    // The ground is authored separately from BR01. Verify its continued road
    // surface before relying on it, instead of drawing overlapping floor faces.
    const auto map=ReadAsset(wad,"STG33A.MPD");CHECK(map.size()>=8192);
    if(map.size()>=8192)for(int z=-9472;z>=-16128;z-=256)for(int x:{-768,-1024}) {
        const unsigned row=unsigned(-z/256-1)&63,col=unsigned(-x/256-1)&63;
        int16_t tile;memcpy(&tile,map.data()+(row*64+col)*2,2);CHECK(tile>=0);
    }
}
static float matrix[12]={1,0,0,0,0,1,0,0,0,0,1,0};
static const float* __cdecl Matrix(){return matrix;}
static unsigned projected_faces;
struct ProjectionDepth {
    float far_depth,start;unsigned shift;int fog,color_fade;
};
static ProjectionDepth projected_depth;
static ProjectionDepth ReadProjectionDepth(){return {CameraField<float>(0x6BC64),CameraField<float>(0x6BC68),
    CameraField<unsigned>(0x6BC6C),CameraField<int>(0x6BC60),CameraField<int>(0x6BC70)};}
static int __cdecl Project(const CameraRetailModel* model,int,int){projected_faces=model->face_count;projected_depth=ReadProjectionDepth();return 1;}
static void CheckDepthRestored(const ProjectionDepth& expected) {
    const auto actual=ReadProjectionDepth();
    CHECK(actual.far_depth==expected.far_depth && actual.start==expected.start && actual.shift==expected.shift);
    CHECK(actual.fog==expected.fog && actual.color_fade==expected.color_fade);
}
static void Branch(DWORD rva,void* target){auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=0xE9;LONG delta=LONG(reinterpret_cast<BYTE*>(target)-p-5);memcpy(p+1,&delta,4);}
static double Contact(bool lift) {
    const ff_camera::Point focus={0,300,lift?500.0:-1700.0},end={0,300,lift?1100.0:-500.0};double fraction=1;
    for(const auto& triangle:g_camera_geometry_frame.triangles)fraction=ff_camera::SweepCameraProbe(focus,end,48,triangle,fraction);
    return fraction;
}
static BYTE* native_asset;
static BYTE* __cdecl Resource(const char*){return native_asset;}
static void CheckNative(const char* executable,const std::string& directory) {
    std::ifstream file(executable,std::ios::binary);std::vector<BYTE> image((std::istreambuf_iterator<char>(file)),{});
    CHECK(image.size()>0x50000);if(image.size()<=0x50000)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(image.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);
    // Execute the original MOD loader and the reviewed environment handlers. Only
    // resource lookup and the final projection boundary are substituted.
    for(const auto range:{std::pair<DWORD,DWORD>{0x43580,0x4367A},{0xFC50,0xFC96},{0xFCD0,0xFCEE},{0x459C0,0x459DB}}) {
        bool copied=false;
        for(unsigned s=0;s<nt->FileHeader.NumberOfSections;++s)if(range.first>=sections[s].VirtualAddress && range.second<=sections[s].VirtualAddress+sections[s].SizeOfRawData) {
            memcpy(reinterpret_cast<BYTE*>(g_base)+range.first,image.data()+sections[s].PointerToRawData+range.first-sections[s].VirtualAddress,range.second-range.first);copied=true;
        }
        CHECK(copied);if(!copied)return;
        for(DWORD rva=range.first;rva+4<=range.second;++rva) {
            DWORD value;memcpy(&value,reinterpret_cast<void*>(g_base+rva),4);
            for(DWORD global:{0xC651C0u,0xC6D140u,0xC5A6F8u,0xC689ECu})if(value==global) {
                value=DWORD(g_base)+value-0x400000;memcpy(reinterpret_cast<void*>(g_base+rva),&value,4);rva+=3;break;
            }
        }
    }
    Branch(0x43390,reinterpret_cast<void*>(Resource));Branch(0x26DE0,reinterpret_cast<void*>(GameCameraModel));
    for(unsigned slot:{132u,141u,142u,271u}) {
        const bool lift=slot==132,bridge=slot==271;const auto wad=directory+(lift?"/MODELS13.WAD":bridge?"/MODEL33A.WAD":"/MODELS15.WAD");
        const char* name=lift?"NEWZL2.MOD":bridge?"BR01.MOD":slot==141?"ROOM15.MOD":"ROOM153.MOD";auto asset=ReadAsset(wad,name);
        if(asset.empty())continue;native_asset=asset.data();
        reinterpret_cast<void(__cdecl*)(const char*,unsigned,int)>(g_base+0x43580)("fixture",slot,0);
        const auto model=CameraField<const CameraRetailModel*>(0x85A6F8+slot*4);
        Source loaded;loaded.vertices.assign(reinterpret_cast<const Vertex*>(model->vertices),reinterpret_cast<const Vertex*>(model->vertices)+model->vertex_count);
        loaded.faces.assign(reinterpret_cast<const Face*>(model->faces),reinterpret_cast<const Face*>(model->faces)+model->face_count);
        const auto expected=ReadModel(wad,name);CHECK(expected.vertices.size()==loaded.vertices.size() && expected.faces.size()==loaded.faces.size());
        CHECK(!memcmp(expected.vertices.data(),loaded.vertices.data(),loaded.vertices.size()*sizeof(Vertex)));
        CHECK(!memcmp(expected.faces.data(),loaded.faces.data(),loaded.faces.size()*sizeof(Face)));
        if(bridge)CheckBridgeStreet(loaded,directory+"/STAGE33A.WAD");
        else CheckMesh(loaded,lift?ff_rooms::Room::Elevator:ff_rooms::Room::Office);
        if(lift) {
            const auto pane=loaded.faces[63].material;CameraField<BYTE>(0x451DBC+unsigned(pane)*32)=1;
        }
        const auto before=asset;CameraField<int>(0x85A6B8)=lift?2:bridge?11:4;ClearCameraGeometry();g_camera_geometry_frame.triangles.clear();
        // Native BRONX uses a 3200-unit fog start, shift 2 and 4800-unit
        // clipping plane. Its added far curb is already beyond the black fade.
        CameraField<float>(0x6BC64)=4800;CameraField<float>(0x6BC68)=3200;
        CameraField<unsigned>(0x6BC6C)=2;CameraField<int>(0x6BC60)=1;CameraField<int>(0x6BC70)=0;
        const auto original_depth=ReadProjectionDepth();
        int object[9]={int(slot)};projected_faces=0;
        const auto handler=reinterpret_cast<void(__cdecl*)(int*)>(g_base+(lift?0xFC50:bridge?0x459C0:0xFCD0));
        handler(object);
        CHECK(projected_faces>model->face_count);CHECK(asset==before);if(!bridge)CHECK(Contact(lift)<1);
        CheckDepthRestored(original_depth);
        CHECK(projected_depth.start==3200 && projected_depth.fog==1 && projected_depth.color_fade==0);
        CHECK(projected_depth.far_depth==(bridge?12000:4800) && projected_depth.shift==unsigned(bridge?5:2));
        printf("Native room %u: %u -> %u faces\n",slot,model->face_count,projected_faces);
        if(bridge) {
            g_controls.complete_rooms=false;handler(object);CHECK(projected_faces==model->face_count);
            CHECK(projected_depth.far_depth==4800 && projected_depth.shift==2);CheckDepthRestored(original_depth);
            g_controls.complete_rooms=true;
            // Do not re-enable F2 fog or switch its vertex-color fallback.
            for(int color_fade:{0,1}) {
                CameraField<int>(0x6BC60)=0;CameraField<int>(0x6BC70)=color_fade;
                const auto toggled=ReadProjectionDepth();handler(object);
                CHECK(projected_depth.fog==0 && projected_depth.color_fade==color_fade);
                CHECK(projected_depth.far_depth==12000 && projected_depth.shift==5);CheckDepthRestored(toggled);
            }
            // Existing longer ranges must survive both during and after draw.
            CameraField<float>(0x6BC64)=20000;CameraField<unsigned>(0x6BC6C)=6;
            const auto longer=ReadProjectionDepth();handler(object);
            CHECK(projected_depth.far_depth==20000 && projected_depth.shift==6);CheckDepthRestored(longer);
        }
        CameraField<const CameraRetailModel*>(0x85A6F8+slot*4)=nullptr;
    }
}
int main(int argc,char** argv) {
    auto lift=Fixture(true),office=Fixture(false);
    CheckMesh(lift,ff_rooms::Room::Elevator);CheckMesh(office,ff_rooms::Room::Office);
    ff_rooms::Mesh rejected;CHECK(!rejected.Build(ff_rooms::Room::None,lift.vertices.data(),184,lift.faces.data(),160));
    CHECK(!rejected.Build(ff_rooms::Room::Elevator,lift.vertices.data(),183,lift.faces.data(),160));
    auto invalid=lift;invalid.faces[0].index[0]=184;CHECK(!rejected.Build(ff_rooms::Room::Elevator,invalid.vertices.data(),184,invalid.faces.data(),160));
    if(argc>1) {
        const std::string directory=argv[1];
        for(const auto name:{"ROOM15.MOD","ROOM153.MOD"})CheckMesh(ReadModel(directory+"/MODELS15.WAD",name),ff_rooms::Room::Office);
        CheckMesh(ReadModel(directory+"/MODELS13.WAD","NEWZL2.MOD"),ff_rooms::Room::Elevator);
    }
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(base);if(!base)return 1;
    g_base=reinterpret_cast<uintptr_t>(base);Branch(0x1980,reinterpret_cast<void*>(Matrix));
    g_free_orbit=g_camera_active=true;g_camera_cooperative=false;g_controls.complete_rooms=true;
    CameraField<int>(0x8745A0)=1;CameraField<int16_t>(0x86A542)=384;CameraField<unsigned>(0x85B560)=100;
    CameraField<int>(0x85A6B8)=2;CameraField<int>(0x85A668)=0;
    auto model=lift.Model();CameraField<const CameraRetailModel*>(0x85A6F8+132*4)=&model;
    auto& frame=g_camera_geometry_frame;frame.environment=frame.active=frame.basis=true;frame.inverse[0]=frame.inverse[4]=frame.inverse[8]=1;
    const auto complete=CompleteCameraRoom(&model);CHECK(complete!=&model && complete->face_count>model.face_count);
    CHECK(CompleteCameraRoom(&model)==complete);
    g_camera_scene_projector=Project;GameCameraModel(&model,0,0);CHECK(projected_faces>model.face_count && !frame.triangles.empty());
    CHECK(Contact(true)<1);
    lift.faces[0].material=7;CHECK(CompleteCameraRoom(&model)->faces[5]==7);lift.faces[0].material=0;
    ++g_camera_geometry_epoch;CHECK(CompleteCameraRoom(&model)->faces[5]==0);
    CHECK(lift.faces[0].flags==0); // Native model faces are never changed.
    g_controls.complete_rooms=false;CHECK(CompleteCameraRoom(&model)==&model);
    frame.triangles.clear();GameCameraModel(&model,0,0);CHECK(Contact(true)==1);g_controls.complete_rooms=true;
    for(int condition=0;condition<4;++condition) {
        g_free_orbit=condition!=0;g_camera_active=condition!=1;frame.environment=condition!=2;CameraField<int>(0x85A668)=condition==3?1:0;
        CHECK(CompleteCameraRoom(&model)==&model);
    }
    g_free_orbit=g_camera_active=frame.environment=true;CameraField<int>(0x85A668)=0;
    // Either split view and the surviving second player share the completion.
    g_camera_cooperative=true;CameraField<int>(0x8745A0)=2;CameraField<int16_t>(0x86A542)=0;CameraField<int16_t>(0x86A636)=384;
    for(int owner:{-1,0,1}){CameraPlayerScope scope(owner);CHECK(CompleteCameraRoom(&model)!=&model);}
    g_camera_cooperative=false;CameraField<int>(0x8745A0)=1;CameraField<int16_t>(0x86A542)=384;
    CameraField<int>(0x85A6B8)=4;auto room=office.Model();CameraField<unsigned>(0x85B560)=659;
    CameraField<const CameraRetailModel*>(0x85A6F8+141*4)=&room;CHECK(CompleteCameraRoom(&room)==&room);
    CameraField<unsigned>(0x85B560)=660;
    for(unsigned type:{141u,142u}){CameraField<const CameraRetailModel*>(0x85A6F8+type*4)=&room;CHECK(CompleteCameraRoom(&room)!=&room);CameraField<const CameraRetailModel*>(0x85A6F8+type*4)=nullptr;}
    CameraField<int>(0x85A6B8)=3;CHECK(CompleteCameraRoom(&room)==&room);
    if(argc>1) {
        const char* names[]={"LIFT01.MOD","LIFT02.MOD","LIFTBOT.MOD","LIFTBOT2.MOD","LIFTTOP.MOD","LIFTTOP2.MOD"};
        for(unsigned i=0;i<_countof(names);++i) {
            const unsigned type=437+i;auto shell=ReadModel(std::string(argv[1])+"/MODELS72.WAD",names[i]);
            CheckLaboratoryLift(shell,int(type));auto native=shell.Model();
            CameraField<int>(0x85A6B8)=23;CameraField<const CameraRetailModel*>(0x85A6F8+type*4)=&native;
            ++g_camera_geometry_epoch;
            const auto completed=CompleteCameraRoom(&native);CHECK(completed!=&native && completed->face_count>native.face_count);
            g_controls.complete_rooms=false;CHECK(CompleteCameraRoom(&native)==&native);g_controls.complete_rooms=true;
            CameraField<int>(0x85A6B8)=22;CHECK(CompleteCameraRoom(&native)==&native);
            CameraField<const CameraRetailModel*>(0x85A6F8+type*4)=nullptr;
        }
        struct Interior {int stage,type;const char* wad;const char* name;ff_rooms::Room room;};
        const Interior interiors[]={
            {22,401,"MODELS71.WAD","LAB01S.MOD",ff_rooms::Room::Laboratory},
            {19,365,"MODELS65.WAD","NAVEL05.MOD",ff_rooms::Room::Ship},
            {7,207,"MODELS31.WAD","MALL01.MOD",ff_rooms::Room::MallAtrium},
            {7,210,"MODELS31.WAD","MALL02A.MOD",ff_rooms::Room::Mall},
            {11,271,"MODEL33A.WAD","BR01.MOD",ff_rooms::Room::BridgeStreet}};
        for(const auto& profile:interiors) {
            auto shell=ReadModel(std::string(argv[1])+"/"+profile.wad,profile.name);
            if(profile.room==ff_rooms::Room::BridgeStreet)CheckBridgeStreet(shell,std::string(argv[1])+"/STAGE33A.WAD");
            else CheckInterior(shell,profile.room);
            auto native=shell.Model();CameraField<int>(0x85A6B8)=profile.stage;
            CameraField<const CameraRetailModel*>(0x85A6F8+profile.type*4)=&native;++g_camera_geometry_epoch;
            CHECK(CompleteCameraRoom(&native)!=&native);
            g_controls.complete_rooms=false;CHECK(CompleteCameraRoom(&native)==&native);g_controls.complete_rooms=true;
            CameraField<int>(0x85A6B8)=0;CHECK(CompleteCameraRoom(&native)==&native);
            CameraField<const CameraRetailModel*>(0x85A6F8+profile.type*4)=nullptr;
        }
    }
    if(argc>2)CheckNative(argv[2],argv[1]);
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Room completion: FAIL":"Room completion: PASS");return failures?1:0;
}
