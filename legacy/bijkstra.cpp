// legacy/bijkstra.cpp
// ---------------------------------------------------------------------------
// Original single-file Google Maps–style Dijkstra demo (preserved verbatim).
// This file is kept for reference and is NOT built by the modern project.
// See ../src/ for the modular, multi-stage rewrite.
// ---------------------------------------------------------------------------
#include "raylib.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <cmath>
#include <sstream>

using namespace std;
#define COL_BG        CLITERAL(Color){232,224,216,255}
#define COL_PARK      CLITERAL(Color){200,230,192,255}
#define COL_WATER     CLITERAL(Color){170,218,255,255}
#define COL_BUILDING  CLITERAL(Color){220,213,200,255}
#define COL_ROAD_MAJ  CLITERAL(Color){255,255,255,255}
#define COL_ROAD_MIN  CLITERAL(Color){245,241,236,255}
#define COL_ROAD_STR  CLITERAL(Color){200,193,180,255}
#define COL_PATH      CLITERAL(Color){ 66,133,244,255}
#define COL_PATH_DASH CLITERAL(Color){255,255,255,180}
#define COL_NODE      CLITERAL(Color){255,255,255,255}
#define COL_NODE_STR  CLITERAL(Color){170,170,170,255}
#define COL_START     CLITERAL(Color){ 52,168, 83,255}
#define COL_END       CLITERAL(Color){234, 67, 53,255}
#define COL_ONPATH    CLITERAL(Color){ 66,133,244,255}
#define COL_CAR       CLITERAL(Color){251,188,  4,255}
#define COL_SIDEBAR   CLITERAL(Color){255,255,255,255}
#define COL_LABEL     CLITERAL(Color){ 50, 50, 50,255}
#define COL_MUTED     CLITERAL(Color){128,134,139,255}
#define COL_BLUE_TXT  CLITERAL(Color){ 26,115,232,255}

struct Edge { string dest; int dist; };

struct Node {
    string name;
    Vector2 pos;
    float rawX, rawY;
};

struct Park  { float x,y,w,h; };
struct Water { float x,y,w,h; };
struct Block { float x,y,w,h; };

class NavSystem {
public:
    unordered_map<string,Node> nodes;
    unordered_map<string,vector<Edge>> graph;

    void addNode(const string& name, float rx, float ry){
        nodes[name] = {name,{0,0},rx,ry};
    }
    void addRoute(const string& a, const string& b, int d){
        graph[a].push_back({b,d});
        graph[b].push_back({a,d});
    }

    void layout(float W, float H,
                float padL=310, float padR=30,
                float padT=60,  float padB=60){
        float mnx=1e9,mxx=-1e9,mny=1e9,mxy=-1e9;
        for(auto&[k,n]:nodes){ mnx=min(mnx,n.rawX); mxx=max(mxx,n.rawX);
                               mny=min(mny,n.rawY); mxy=max(mxy,n.rawY); }
        float aw=W-padL-padR, ah=H-padT-padB;
        float sc=min(aw/(mxx-mnx), ah/(mxy-mny));
        float offX=padL+(aw-(mxx-mnx)*sc)/2;
        float offY=padT+(ah-(mxy-mny)*sc)/2;
        for(auto&[k,n]:nodes){
            n.pos = { offX+(n.rawX-mnx)*sc, offY+(n.rawY-mny)*sc };
        }
    }

    vector<string> findPath(const string& start, const string& end){
        unordered_map<string,double> dist;
        unordered_map<string,string> prev;
        for(auto&[k,_]:nodes) dist[k]=1e18;
        dist[start]=0;
        priority_queue<pair<double,string>,
            vector<pair<double,string>>,greater<>> pq;
        pq.push({0,start});
        while(!pq.empty()){
            auto [d,u]=pq.top(); pq.pop();
            if(u==end) break;
            if(d>dist[u]) continue;
            for(auto&e:graph[u]){
                double nd=d+e.dist;
                if(nd<dist[e.dest]){
                    dist[e.dest]=nd;
                    prev[e.dest]=u;
                    pq.push({nd,e.dest});
                }
            }
        }
        vector<string> path;
        if(dist[end]>=1e17) return path;
        for(string v=end; v!=start; v=prev[v]) path.push_back(v);
        path.push_back(start);
        reverse(path.begin(),path.end());
        return path;
    }

    int routeDist(const vector<string>& path){
        int total=0;
        for(int i=0;i+1<(int)path.size();i++){
            for(auto&e:graph[path[i]])
                if(e.dest==path[i+1]){ total+=e.dist; break; }
        }
        return total;
    }

    int segDist(const string& a, const string& b){
        for(auto&e:graph[a]) if(e.dest==b) return e.dist;
        return 0;
    }
};

void DrawRoundRect(float x,float y,float w,float h,float r,Color c){
    DrawRectangleRounded({x,y,w,h},r/(min(w,h)*0.5f),8,c);
}
void DrawRoundRectLines(float x,float y,float w,float h,float r,float thick,Color c){
    DrawRectangleRoundedLines({x,y,w,h},r/(min(w,h)*0.5f),8,thick,c);
}

float Vec2Len(Vector2 v){ return sqrtf(v.x*v.x+v.y*v.y); }
Vector2 Vec2Norm(Vector2 v){ float l=Vec2Len(v); return l>0?Vector2{v.x/l,v.y/l}:Vector2{0,0}; }

void DrawDashedLine(Vector2 a, Vector2 b, float thick, float dashLen, float gapLen, Color c){
    Vector2 dir=Vec2Norm({b.x-a.x,b.y-a.y});
    float total=Vec2Len({b.x-a.x,b.y-a.y});
    float t=0;
    bool drawing=true;
    while(t<total){
        float seg=drawing?dashLen:gapLen;
        float end=min(t+seg,total);
        if(drawing){
            Vector2 p1={a.x+dir.x*t,   a.y+dir.y*t};
            Vector2 p2={a.x+dir.x*end, a.y+dir.y*end};
            DrawLineEx(p1,p2,thick,c);
        }
        t=end; drawing=!drawing;
    }
}

struct MapDeco {
    vector<Park>  parks;
    vector<Water> waters;
    vector<Block> blocks;
};

MapDeco buildDeco(){
    MapDeco d;
    d.parks = {
        {30, 30,130, 90},{590, 30,110,110},{340,320, 90, 70},
        { 60,460,100, 80},{480,430,120, 90}
    };
    d.waters = {
        {610, 30,130,150},{  0,590,210,110}
    };
    d.blocks = {
        {190, 70, 55, 45},{265, 60, 45, 55},
        {440,140, 60, 40},{510,150, 45, 50},
        {180,290, 50, 40},{235,300, 55, 35},
        {400,270, 45, 45},{450,280, 55, 40},
        {310,430, 60, 40},{375,440, 50, 45},
        {190,550, 45, 40},{240,545, 55, 45},
        {450,540, 60, 45},{510,535, 45, 50},
        {560,220, 55, 45},{615,230, 50, 40}
    };
    return d;
}

Rectangle decoToScreen(float rx,float ry,float rw,float rh,
                       float mnx,float mny,float sc,float offX,float offY){
    return {offX+(rx-mnx)*sc, offY+(ry-mny)*sc, rw*sc, rh*sc};
}

void buildCity(NavSystem& nav){
    nav.addNode("Northgate",  320, 60);
    nav.addNode("Westfield",  100,160);
    nav.addNode("Uptown",     520,110);
    nav.addNode("Riverside",  160,280);
    nav.addNode("Central",    380,260);
    nav.addNode("Eastpark",   600,220);
    nav.addNode("Harbor",      80,420);
    nav.addNode("Midtown",    300,400);
    nav.addNode("Commerce",   520,380);
    nav.addNode("Lakeside",   680,350);
    nav.addNode("Southport",  180,540);
    nav.addNode("Downtown",   400,520);
    nav.addNode("Airport",    620,510);
    nav.addNode("Beachfront", 260,650);
    nav.addNode("Terminal",   530,640);

    nav.addRoute("Northgate","Westfield",8);
    nav.addRoute("Northgate","Uptown",   7);
    nav.addRoute("Northgate","Central",  9);
    nav.addRoute("Westfield","Riverside",6);
    nav.addRoute("Westfield","Harbor",  10);
    nav.addRoute("Uptown",   "Eastpark", 5);
    nav.addRoute("Uptown",   "Central",  6);
    nav.addRoute("Riverside","Central",  5);
    nav.addRoute("Riverside","Harbor",   7);
    nav.addRoute("Riverside","Midtown",  6);
    nav.addRoute("Central",  "Eastpark", 6);
    nav.addRoute("Central",  "Midtown",  5);
    nav.addRoute("Central",  "Commerce", 7);
    nav.addRoute("Eastpark", "Lakeside", 4);
    nav.addRoute("Eastpark", "Commerce", 6);
    nav.addRoute("Harbor",   "Southport",6);
    nav.addRoute("Harbor",   "Midtown",  5);
    nav.addRoute("Midtown",  "Commerce", 4);
    nav.addRoute("Midtown",  "Southport",7);
    nav.addRoute("Midtown",  "Downtown", 5);
    nav.addRoute("Commerce", "Lakeside", 5);
    nav.addRoute("Commerce", "Downtown", 6);
    nav.addRoute("Commerce", "Airport",  7);
    nav.addRoute("Lakeside", "Airport",  6);
    nav.addRoute("Southport","Downtown", 6);
    nav.addRoute("Southport","Beachfront",5);
    nav.addRoute("Downtown", "Airport",  7);
    nav.addRoute("Downtown", "Beachfront",6);
    nav.addRoute("Downtown", "Terminal", 5);
    nav.addRoute("Airport",  "Terminal", 6);
    nav.addRoute("Beachfront","Terminal",7);
}

int main(){
    const int SW = 1200, SH = 720;
    InitWindow(SW, SH, "Google Maps — Dijkstra Navigation");
    SetTargetFPS(60);

    Font font = GetFontDefault();
    NavSystem nav;
    buildCity(nav);
    nav.layout((float)SW,(float)SH);

    float mnx=1e9,mxx=-1e9,mny=1e9,mxy=-1e9;
    for(auto&[k,n]:nav.nodes){
        mnx=min(mnx,n.rawX); mxx=max(mxx,n.rawX);
        mny=min(mny,n.rawY); mxy=max(mxy,n.rawY);
    }
    float padL=310,padR=30,padT=60,padB=60;
    float aw=SW-padL-padR, ah=SH-padT-padB;
    float sc=min(aw/(mxx-mnx), ah/(mxy-mny));
    float offX=padL+(aw-(mxx-mnx)*sc)/2;
    float offY=padT+(ah-(mxy-mny)*sc)/2;

    MapDeco deco = buildDeco();

    string startNode = "Westfield";
    string endNode   = "Airport";
    vector<string> path = nav.findPath(startNode, endNode);

    int    carSeg  = 0;
    float  carFrac = 0.0f;
    float  carSpeedPx = 80.0f;
    float dashTimer = 0.0f;
    const float SB_X=0, SB_W=298, SB_H=(float)SH;

    while(!WindowShouldClose()){
        float dt = GetFrameTime();
        dashTimer += dt * 40.0f;
        Vector2 mp = GetMousePosition();

        auto hitNode = [&]() -> string {
            for(auto&[name,node]:nav.nodes){
                if(CheckCollisionPointCircle(mp, node.pos, 14.0f))
                    return name;
            }
            return "";
        };

        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            string hit = hitNode();
            if(!hit.empty() && hit!=startNode){
                endNode = hit;
                path = nav.findPath(startNode, endNode);
                carSeg=0; carFrac=0;
            }
        }
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
            string hit = hitNode();
            if(!hit.empty() && hit!=endNode){
                startNode = hit;
                path = nav.findPath(startNode, endNode);
                carSeg=0; carFrac=0;
            }
        }

        if((int)path.size()>=2 && carSeg < (int)path.size()-1){
            Vector2 a = nav.nodes[path[carSeg]].pos;
            Vector2 b = nav.nodes[path[carSeg+1]].pos;
            float segLen = Vec2Len({b.x-a.x, b.y-a.y});
            if(segLen>0){
                carFrac += (carSpeedPx*dt)/segLen;
                if(carFrac>=1.0f){
                    carFrac=0;
                    carSeg++;
                    if(carSeg>=(int)path.size()-1){ carSeg=0; }
                }
            }
        }

        BeginDrawing();
        ClearBackground(COL_BG);
        for(auto&p:deco.parks){
            Rectangle r=decoToScreen(p.x,p.y,p.w,p.h,mnx,mny,sc,offX,offY);
            DrawRectangleRec(r, COL_PARK);
            DrawRectangleLinesEx(r, 1, CLITERAL(Color){170,210,160,255});
        }
        for(auto&w:deco.waters){
            Rectangle r=decoToScreen(w.x,w.y,w.w,w.h,mnx,mny,sc,offX,offY);
            DrawRectangleRec(r, COL_WATER);
        }
        for(auto&b:deco.blocks){
            Rectangle r=decoToScreen(b.x,b.y,b.w,b.h,mnx,mny,sc,offX,offY);
            DrawRectangleRec(r, COL_BUILDING);
        }

        for(auto&[src,edges]:nav.graph){
            for(auto&e:edges){
                Vector2 a=nav.nodes[src].pos;
                Vector2 b=nav.nodes[e.dest].pos;
                bool major=(e.dist>=8);
                DrawLineEx(a,b, major?10.0f:6.0f, COL_ROAD_STR);
                DrawLineEx(a,b, major?7.0f:4.0f,  major?COL_ROAD_MAJ:COL_ROAD_MIN);
            }
        }
        for(auto&[src,edges]:nav.graph){
            for(auto&e:edges){
                if(e.dist>=8 && src<e.dest){
                    Vector2 a=nav.nodes[src].pos;
                    Vector2 b=nav.nodes[e.dest].pos;
                    Vector2 mid={(a.x+b.x)/2,(a.y+b.y)/2};
                    string lbl=to_string(e.dist)+"km";
                    int tw=MeasureText(lbl.c_str(),10);
                    DrawRectangle((int)mid.x-tw/2-2,(int)mid.y-8,tw+4,14,
                                  CLITERAL(Color){255,255,255,200});
                    DrawText(lbl.c_str(),(int)mid.x-tw/2,(int)mid.y-7,10,COL_MUTED);
                }
            }
        }
        if((int)path.size()>=2){
            for(int i=0;i+1<(int)path.size();i++){
                Vector2 a=nav.nodes[path[i]].pos;
                Vector2 b=nav.nodes[path[i+1]].pos;
                DrawLineEx(a,b,9.0f,COL_PATH);
            }
            for(int i=0;i+1<(int)path.size();i++){
                Vector2 a=nav.nodes[path[i]].pos;
                Vector2 b=nav.nodes[path[i+1]].pos;
                Vector2 dir=Vec2Norm({b.x-a.x,b.y-a.y});
                float total=Vec2Len({b.x-a.x,b.y-a.y});
                float dashLen=12,gapLen=10,stride=dashLen+gapLen;
                float offset=fmodf(dashTimer,stride);
                float t=-offset;
                while(t<total){
                    float t0=max(t,0.0f);
                    float t1=min(t+dashLen,total);
                    if(t1>t0){
                        Vector2 p0={a.x+dir.x*t0,a.y+dir.y*t0};
                        Vector2 p1={a.x+dir.x*t1,a.y+dir.y*t1};
                        DrawLineEx(p0,p1,3.0f,COL_PATH_DASH);
                    }
                    t+=stride;
                }
            }
        }

        for(auto&[name,node]:nav.nodes){
            bool isStart = name==startNode;
            bool isEnd   = name==endNode;
            bool onPath  = !path.empty() &&
                           find(path.begin(),path.end(),name)!=path.end();
            float r = isStart||isEnd ? 13.0f : onPath ? 10.0f : 8.0f;
            Color fill   = isStart ? COL_START : isEnd ? COL_END :
                           onPath  ? COL_ONPATH : COL_NODE;
            Color stroke = isStart ? CLITERAL(Color){40,130,60,255} :
                           isEnd   ? CLITERAL(Color){180,50,40,255} :
                           onPath  ? CLITERAL(Color){40,100,200,255} :
                           COL_NODE_STR;
            DrawCircleV({node.pos.x+2,node.pos.y+2},r+1,
                        CLITERAL(Color){0,0,0,40});
            DrawCircleV(node.pos,r,fill);
            DrawCircleLines((int)node.pos.x,(int)node.pos.y,(int)r,stroke);
            if(isStart||isEnd) DrawCircleV(node.pos,4.0f,WHITE);
            int fs=11;
            int tw=MeasureText(name.c_str(),fs);
            DrawRectangle((int)node.pos.x-tw/2-3,(int)node.pos.y-(int)r-18,
                          tw+6,15,CLITERAL(Color){255,255,255,210});
            DrawText(name.c_str(),(int)node.pos.x-tw/2,(int)node.pos.y-(int)r-17,
                     fs, COL_LABEL);
        }

        if((int)path.size()>=2 && carSeg<(int)path.size()-1){
            Vector2 a=nav.nodes[path[carSeg]].pos;
            Vector2 b=nav.nodes[path[carSeg+1]].pos;
            Vector2 carPos={a.x+(b.x-a.x)*carFrac, a.y+(b.y-a.y)*carFrac};
            float angle=atan2f(b.y-a.y,b.x-a.x);
            DrawCircleV({carPos.x+2,carPos.y+2},10,CLITERAL(Color){0,0,0,60});
            DrawCircleV(carPos,10,COL_CAR);
            DrawCircleLines((int)carPos.x,(int)carPos.y,10,
                            CLITERAL(Color){210,155,0,255});
            float ax=carPos.x+cosf(angle)*6;
            float ay=carPos.y+sinf(angle)*6;
            float bx=carPos.x+cosf(angle+2.3f)*5;
            float by=carPos.y+sinf(angle+2.3f)*5;
            float cx2=carPos.x+cosf(angle-2.3f)*5;
            float cy2=carPos.y+sinf(angle-2.3f)*5;
            DrawTriangle({ax,ay},{bx,by},{cx2,cy2},
                         CLITERAL(Color){100,60,0,220});
        }

        DrawRectangle((int)SB_X+3,0,(int)SB_W,SH,CLITERAL(Color){0,0,0,30});
        DrawRectangle((int)SB_X,0,(int)SB_W,SH,COL_SIDEBAR);
        DrawLine((int)SB_W,0,(int)SB_W,SH,CLITERAL(Color){220,220,220,255});
        DrawRectangle(0,0,(int)SB_W,64,WHITE);
        DrawLine(0,64,(int)SB_W,64,CLITERAL(Color){230,230,230,255});
        DrawCircleV({24,24},8,COL_END);
        DrawCircleV({24,24},4,WHITE);
        DrawTriangle({19,30},{29,30},{24,38},COL_END);
        DrawText("Navigation", 40, 14, 18, COL_LABEL);
        DrawText("Left-click = destination  |  Right-click = start",
                 10, 42, 10, COL_MUTED);

        int y = 80;
        if((int)path.size()>=2){
            int totalKm = nav.routeDist(path);
            int mins    = totalKm*2 + (int)path.size();
            DrawRectangle(0,y,SB_W,50,CLITERAL(Color){232,240,253,255});
            DrawLine(0,y+50,SB_W,y+50,CLITERAL(Color){200,220,250,255});
            string distStr = to_string(totalKm)+" km   "+to_string(mins)+" min";
            DrawText(distStr.c_str(),14,y+8,16,COL_BLUE_TXT);
            DrawText("Fastest route",14,y+30,11,COL_MUTED);
            y+=60;
            string stopStr=to_string((int)path.size()-2)+" stops";
            string timeStr=to_string(mins)+" min";
            DrawRoundRect(12,y,90,24,12,CLITERAL(Color){241,243,244,255});
            DrawText(stopStr.c_str(),22,y+6,11,COL_LABEL);
            DrawRoundRect(110,y,80,24,12,CLITERAL(Color){241,243,244,255});
            DrawText(timeStr.c_str(),120,y+6,11,COL_LABEL);
            y+=40;
            DrawLine(14,y,SB_W-14,y,CLITERAL(Color){230,230,230,255});
            y+=12;
            DrawText("Turn-by-turn",14,y,12,COL_MUTED);
            y+=20;
            int visibleSteps=min((int)path.size(), (SH-y-10)/38);
            for(int i=0;i<visibleSteps;i++){
                bool isS=(i==0), isE=(i==(int)path.size()-1);
                bool isActive=(carSeg==i && (int)path.size()>=2);
                if(isActive)
                    DrawRectangle(0,y-2,SB_W,36,CLITERAL(Color){232,240,253,255});
                Color dotC = isS?COL_START : isE?COL_END : COL_ONPATH;
                DrawCircleV({20,(float)y+14},isS||isE?7.0f:5.0f,dotC);
                if(isS||isE) DrawCircleV({20,(float)y+14},3,WHITE);
                if(i<visibleSteps-1)
                    DrawLine(20,y+21,20,y+38,CLITERAL(Color){200,210,230,255});
                DrawText(path[i].c_str(),34,y+7,13,COL_LABEL);
                if(i+1<(int)path.size()){
                    int segD=nav.segDist(path[i],path[i+1]);
                    string segStr=to_string(segD)+" km";
                    int tw2=MeasureText(segStr.c_str(),11);
                    DrawText(segStr.c_str(),SB_W-tw2-12,y+9,11,COL_MUTED);
                }
                y+=38;
            }
        } else {
            DrawText("No path found",14,y,13,CLITERAL(Color){200,60,60,255});
        }
        DrawText("Map data (c) 2025  |  Dijkstra routing",
                 SW-310, SH-18, 10, COL_MUTED);
        DrawRectangle(SW-44,SH-90,32,80,WHITE);
        DrawRectangleLinesEx({(float)SW-44,(float)SH-90,32,80},1,
                             CLITERAL(Color){200,200,200,255});
        DrawLine(SW-44,SH-50,SW-12,SH-50,CLITERAL(Color){200,200,200,255});
        DrawText("+",SW-35,SH-84,22,COL_MUTED);
        DrawText("-",SW-33,SH-46,22,COL_MUTED);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
