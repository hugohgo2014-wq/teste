// Bixu Evolutivo 2.0 — motor C para WebAssembly
// Mantém a API esperada pelo HTML original.
// Melhorias: sensores de energia/luz, visão de comida e rivais, mutação adaptativa,
// custo metabólico mais consistente, proteção de limites e estatísticas de gerações.
#define EXPORT __attribute__((visibility("default")))
#define MAXN 4096
#define MAXS 4096
#define MAXC 64
#define MAXF 16384
#define MAXM 4096
#define NIN 8
#define NHID 32

enum { N_SKIN, N_MOUTH, N_EYE, N_LEAF, N_STOMACH, N_BRAIN, N_WING, N_CLAW, N_GILL, N_HORN, N_JOINT, N_HEART, N_LUNG, N_FOOT, N_FIN, N_TAIL, N_TYPES };
enum { S_BONE, S_MUSCLE, S_TENDON, S_TYPES };
typedef unsigned char u8;

typedef struct {
  int alive, sid, n, m, gen;
  float x[MAXN], y[MAXN], px[MAXN], py[MAXN];
  u8 nt[MAXN], sa[MAXS], sb[MAXS], st[MAXS];
  float rest[MAXS], act[MAXS];
  float w1[NHID*NIN], w2[MAXS*NHID];
  float freq, phase, energy, age, brain_size, body_scale, tpref;
  float gx[MAXN], gy[MAXN];
} Cr;

static Cr cr[MAXC];
static float fx[MAXF], fy[MAXF]; static int nf;
static float mx[MAXM], my[MAXM], mvx[MAXM], mvy[MAXM], mr[MAXM], mm[MAXM]; static int nm;
static float PR=600, G0=.35f, food_rate=.05f, metab=1, mutr=1, air=.995f, sun=0, chosen_brain=1, sea=0, temp=0;
static unsigned rs=2463534242u;
static float dx_[MAXN],dy_[MAXN]; static u8 dt_[MAXN],sa_[MAXS],sb_[MAXS],st_[MAXS];
static float ccx[MAXC],ccy[MAXC];
static float rb[400000];

static float rnd(void){rs^=rs<<13;rs^=rs>>17;rs^=rs<<5;return (rs>>8)*(1.f/16777216.f);}
static float gau(void){return rnd()+rnd()+rnd()-1.5f;}
static int ri(int n){return n>0?(int)(rnd()*n):0;}
static float ab(float x){return x<0?-x:x;}
static float clampf(float x,float a,float b){return x<a?a:x>b?b:x;}
static float sq(float x){return __builtin_sqrtf(x);}
static float sn(float x){
  x-=6.2831853f*(int)(x*.15915494f);
  if(x>3.14159265f)x-=6.2831853f;else if(x< -3.14159265f)x+=6.2831853f;
  float y=1.27323954f*x-.405284735f*x*ab(x);
  return .225f*(y*ab(y)-y)+y;
}
static float th(float x){if(x>3)return 1;if(x< -3)return -1;float z=x*x;return x*(27+z)/(27+9*z);}
static int total(void){int n=0;for(int i=0;i<MAXC;i++)n+=cr[i].alive;return n;}
static int free_slot(void){for(int i=0;i<MAXC;i++)if(!cr[i].alive)return i;return -1;}
EXPORT void init(unsigned seed){rs=seed?seed:1;}
EXPORT void clear(void){for(int i=0;i<MAXC;i++)cr[i].alive=0;nf=nm=0;sun=0;}
EXPORT void set_planet(float r,float g,float food,float met,float mut,float a,float sea_,float temp_){sea=clampf(sea_,0,400);temp=clampf(temp_,-.5f,.5f);
 PR=clampf(r,100,10000);G0=clampf(g,0,2);food_rate=clampf(food,0,.5f);
 metab=clampf(met,.1f,5);mutr=clampf(mut,0,5);air=clampf(a,.8f,1);
}
EXPORT float planet_r(void){return PR;}
EXPORT float sun_angle(void){return sun;}
EXPORT void design_node(int i,float x,float y,int t){if(i>=0&&i<MAXN){dx_[i]=x;dy_[i]=y;dt_[i]=(u8)clampf(t,0,N_TYPES-1);}}
EXPORT void design_brain_size(float s){chosen_brain=s<1?1:s;}
EXPORT void design_spring(int i,int a,int b,int t){if(i>=0&&i<MAXS&&a>=0&&a<MAXN&&b>=0&&b<MAXN){sa_[i]=a;sb_[i]=b;st_[i]=(u8)clampf(t,0,S_TYPES-1);}}
EXPORT int spawn_species(int n,int m,int count,int sid){
 if(n<1||n>MAXN||m<1||m>MAXS||sid<0)return 0;int made=0;
 for(int k=0;k<count;k++){int ci=free_slot();if(ci<0)break;Cr*c=&cr[ci];*c=(Cr){0};
  c->alive=1;c->sid=sid;c->n=n;c->m=m;c->brain_size=chosen_brain;c->body_scale=1;
  float a=rnd()*6.2831853f,rx=sn(a+1.5708f),ry=sn(a),tx=-ry,ty=rx,h=PR+160;
  for(int i=0;i<n;i++){c->x[i]=c->px[i]=rx*h+tx*dx_[i]-rx*dy_[i];c->y[i]=c->py[i]=ry*h+ty*dx_[i]-ry*dy_[i];c->nt[i]=dt_[i];}
  for(int s=0;s<m;s++){if(sa_[s]>=n||sb_[s]>=n||sa_[s]==sb_[s]){c->alive=0;break;}
   c->sa[s]=sa_[s];c->sb[s]=sb_[s];c->st[s]=st_[s];
   float x=dx_[sb_[s]]-dx_[sa_[s]],y=dy_[sb_[s]]-dy_[sa_[s]];c->rest[s]=sq(x*x+y*y);if(c->rest[s]<2)c->rest[s]=2;
  }
  if(!c->alive)continue;
  for(int i=0;i<NHID*NIN;i++)c->w1[i]=gau()*1.25f;
  for(int i=0;i<m*NHID;i++)c->w2[i]=gau()*1.25f;
  c->freq=.06f+rnd()*.08f;c->phase=rnd()*6.2832f;c->energy=110;c->tpref=.45f+rnd()*.2f;made++;
 }return made;
}
static void add_food(float x,float y){if(nf<MAXF){fx[nf]=x;fy[nf]=y;nf++;}else{int j=ri(MAXF);fx[j]=x;fy[j]=y;}}
static void link_(Cr*c,int a,int b,int t){
 if(c->m>=MAXS)return;int s=c->m++;c->sa[s]=a;c->sb[s]=b;c->st[s]=t;c->act[s]=0;
 float x=c->x[b]-c->x[a],y=c->y[b]-c->y[a];c->rest[s]=clampf(sq(x*x+y*y),10,90);
 for(int j=0;j<NHID;j++)c->w2[s*NHID+j]=gau()*1.25f;
}
static void add_node(Cr*c){
 if(c->n>=MAXN||c->m+2>MAXS){c->body_scale*=1.015f;c->brain_size+=.025f*mutr;for(int s=0;s<c->m;s++)c->rest[s]*=1.005f;return;}int a=ri(c->n),b=ri(c->n);if(b==a)b=(a+1)%c->n;
 int k=c->n++;float ang=rnd()*6.2832f,L=22+rnd()*22;
 c->x[k]=c->px[k]=c->x[a]+sn(ang+1.5708f)*L;c->y[k]=c->py[k]=c->y[a]+sn(ang)*L;
 c->nt[k]=rnd()<.45f?N_SKIN:ri(N_TYPES);link_(c,a,k,ri(S_TYPES));link_(c,k,b,ri(S_TYPES));
}
static void del_leaf(Cr*c){
 int k=-1,s0=-1;
 for(int i=0;i<c->n;i++){int d=0,last=-1;for(int s=0;s<c->m;s++)if(c->sa[s]==i||c->sb[s]==i){d++;last=s;}if(d==1){k=i;s0=last;break;}}
 if(k<0||s0<0)return;
 for(int s=s0;s<c->m-1;s++){c->sa[s]=c->sa[s+1];c->sb[s]=c->sb[s+1];c->st[s]=c->st[s+1];c->rest[s]=c->rest[s+1];c->act[s]=c->act[s+1];for(int h=0;h<NHID;h++)c->w2[s*NHID+h]=c->w2[(s+1)*NHID+h];}c->m--;
 for(int i=k;i<c->n-1;i++){c->x[i]=c->x[i+1];c->y[i]=c->y[i+1];c->px[i]=c->px[i+1];c->py[i]=c->py[i+1];c->nt[i]=c->nt[i+1];}c->n--;
 for(int s=0;s<c->m;s++){if(c->sa[s]>k)c->sa[s]--;if(c->sb[s]>k)c->sb[s]--;}
}
static void mutate(Cr*c){
 float M=mutr;c->tpref=clampf(c->tpref+(rnd()<.2f*M?gau()*.08f:0),0,1);
 for(int i=0;i<NHID*NIN;i++)if(rnd()<clampf(.08f*M,0,.65f))c->w1[i]=clampf(c->w1[i]+gau()*.65f,-4,4);
 for(int i=0;i<c->m*NHID;i++)if(rnd()<clampf(.08f*M,0,.65f))c->w2[i]=clampf(c->w2[i]+gau()*.65f,-4,4);
 if(rnd()<.15f*M)c->freq=clampf(c->freq*(1+gau()*.25f),.02f,.3f);
 for(int s=0;s<c->m;s++){if(rnd()<.08f*M)c->rest[s]=clampf(c->rest[s]*(1+gau()*.22f),10,90);if(rnd()<.035f*M)c->st[s]=ri(S_TYPES);}
 if(rnd()<.06f*M)c->nt[ri(c->n)]=ri(N_TYPES);if(c->n>=MAXN||c->m+2>MAXS){if(rnd()<.1f*M){c->brain_size+=.01f*M;c->body_scale*=1.005f;for(int s=0;s<c->m;s++)c->rest[s]*=1.002f;}}
 if(rnd()<.07f*M)add_node(c);if(rnd()<.04f*M&&c->n>2)del_leaf(c);
}
static void reproduce(Cr*p){
 int ci=free_slot();if(ci<0||p->energy<145)return;Cr*q=&cr[ci];*q=*p;p->energy-=65;q->energy=65;q->age=0;q->gen++;
 float sx=0,sy=0;for(int i=0;i<q->n;i++){sx+=q->x[i];sy+=q->y[i];}sx/=q->n;sy/=q->n;
 float d=sq(sx*sx+sy*sy)+.001f,rx=sx/d,ry=sy/d,o=(rnd()-.5f)*90;
 for(int i=0;i<q->n;i++){q->x[i]+=-ry*o+rx*25;q->y[i]+=rx*o+ry*25;q->px[i]=q->x[i];q->py[i]=q->y[i];}
 for(int s=0;s<q->m;s++)q->act[s]=0;mutate(q);
}
static void die(Cr*c){c->alive=0;for(int k=0;k<2;k++){int j=ri(c->n);add_food(c->x[j],c->y[j]);}}
EXPORT void body(float x,float y,float vx,float vy,float r,float mu){if(nm<MAXM){mx[nm]=x;my[nm]=y;mvx[nm]=vx;mvy[nm]=vy;mr[nm]=clampf(r,1,6000);mm[nm]=mu;nm++;}}
EXPORT void meteor(float x,float y,float vx,float vy,float r){body(x,y,vx,vy,clampf(r,1,200),0);}
static void impact(float x,float y,float r){
 float rr=r*5+40,R2=rr*rr;
 for(int i=0;i<MAXC;i++)if(cr[i].alive)for(int j=0;j<cr[i].n;j++){float dx=cr[i].x[j]-x,dy=cr[i].y[j]-y;if(dx*dx+dy*dy<R2){cr[i].energy=-1;break;}}
 for(int f=nf-1;f>=0;f--){float dx=fx[f]-x,dy=fy[f]-y;if(dx*dx+dy*dy<R2){nf--;fx[f]=fx[nf];fy[f]=fy[nf];}}
}
static void think(int ci){
 Cr*c=&cr[ci];float in[NIN]={0};in[0]=sn(c->phase);in[1]=sn(c->phase+1.5708f);in[5]=1;
 float d0=sq(ccx[ci]*ccx[ci]+ccy[ci]*ccy[ci])+.001f,rx=ccx[ci]/d0,ry=ccy[ci]/d0;
 // Energia e direção da luz estão disponíveis mesmo sem olhos.
 in[6]=clampf(c->energy/120.f-1,-1,1);
 float sunx=sn(sun+1.5708f),suny=sn(sun);
 in[7]=clampf((ccx[ci]*sunx+ccy[ci]*suny)/(d0+1),-1,1);
 int ne=0;for(int i=0;i<c->n;i++)ne+=c->nt[i]==N_EYE;
 if(ne){  // visão: alcance cresce com nº de olhos; cada olho mira o alvo mais próximo
  float rng=200+90*(ne>4?4:ne),r2=rng*rng,bf=1e18f,br=1e18f;
  for(int e=0;e<c->n;e++)if(c->nt[e]==N_EYE){
   float ex=c->x[e],ey=c->y[e],bd=r2,tx=0,ty=0;int seen=0;
   for(int f=0;f<nf;f++){float dx=fx[f]-ex,dy=fy[f]-ey,d=dx*dx+dy*dy;
    if(d<bd){bd=d;tx=dx;ty=dy;seen=1;}
    if(d<bf&&d<r2){bf=d;in[2]=clampf((dx*-ry+dy*rx)/rng,-1,1);in[3]=clampf(-(dx*rx+dy*ry)/rng,-1,1);}}
   for(int o=0;o<MAXC;o++)if(o!=ci&&cr[o].alive&&cr[o].sid!=c->sid){
    float dx=ccx[o]-ex,dy=ccy[o]-ey,d=dx*dx+dy*dy;
    if(d<r2&&d<br){br=d;in[4]=clampf((dx*-ry+dy*rx)/rng,-1,1);}
    if(d<bd){bd=d;tx=dx;ty=dy;seen=1;}}
   if(!seen){tx=sn(c->phase*.7f+e);ty=sn(c->phase*.5f+e*2.f);}
   float L=sq(tx*tx+ty*ty)+.001f;
   c->gx[e]+=(tx/L-c->gx[e])*.15f;c->gy[e]+=(ty/L-c->gy[e])*.15f;}
 }
 float h[NHID];for(int j=0;j<NHID;j++){float z=0;for(int i=0;i<NIN;i++)z+=c->w1[j*NIN+i]*in[i];h[j]=th(z);}
 for(int s=0;s<c->m;s++){float z=0;for(int j=0;j<NHID;j++)z+=c->w2[s*NHID+j]*h[j];c->act[s]=c->st[s]==S_MUSCLE?th(z):0;}
 c->phase+=c->freq;if(c->phase>6.2832f)c->phase-=6.2832f;
}
static void physics(Cr*c){
 float GM=G0*PR*PR,fl=0;for(int s=0;s<c->m;s++)fl+=ab(c->act[s]);fl=c->m?fl/c->m:0;
 for(int i=0;i<c->n;i++){float x=c->x[i],y=c->y[i],d=sq(x*x+y*y)+.001f,dd=d<PR?PR:d,g=G0*PR*PR/(dd*dd),dm=air;int wet=d<PR+sea;
  if(wet){dm=c->nt[i]==N_FIN?.85f:.93f;g*=.25f;}
  float ax=-x/d*g,ay=-y/d*g;
  if(c->nt[i]==N_WING){ax+=x/d*g*1.1f*fl;ay+=y/d*g*1.1f*fl;}
  for(int j=0;j<nm;j++)if(mm[j]>0){float dx=mx[j]-x,dy=my[j]-y,dj=sq(dx*dx+dy*dy)+10,rj=sq(mx[j]*mx[j]+my[j]*my[j])+.001f,qq=GM*mm[j];
   ax+=qq*(dx/(dj*dj*dj)-mx[j]/(rj*rj*rj));ay+=qq*(dy/(dj*dj*dj)-my[j]/(rj*rj*rj));}
  float vx=(x-c->px[i])*dm,vy=(y-c->py[i])*dm;c->px[i]=x;c->py[i]=y;c->x[i]=x+vx+ax;c->y[i]=y+vy+ay;}
 for(int it=0;it<5;it++){
  for(int s=0;s<c->m;s++){int a=c->sa[s],b=c->sb[s];if(a>=c->n||b>=c->n)continue;
   float dx=c->x[b]-c->x[a],dy=c->y[b]-c->y[a],d=sq(dx*dx+dy*dy);if(d<.001f)continue;
   float target=c->rest[s],k=.8f;if(c->st[s]==S_MUSCLE){target*=1+.4f*c->act[s];k=.5f;}else if(c->st[s]==S_TENDON)k=.2f;
   float df=(d-target)/d*.5f*k;c->x[a]+=dx*df;c->y[a]+=dy*df;c->x[b]-=dx*df;c->y[b]-=dy*df;
  }
  for(int i=0;i<c->n;i++){float d=sq(c->x[i]*c->x[i]+c->y[i]*c->y[i]);if(d<PR&&d>.001f){c->x[i]*=PR/d;c->y[i]*=PR/d;}}
 }
 for(int i=0;i<c->n;i++){float d=sq(c->x[i]*c->x[i]+c->y[i]*c->y[i])+.001f;if(d<PR+.5f){float rx=c->x[i]/d,ry=c->y[i]/d,vx=c->x[i]-c->px[i],vy=c->y[i]-c->py[i],vn=vx*rx+vy*ry,fr=c->nt[i]==N_FOOT?.08f:c->nt[i]==N_SKIN?.25f:c->nt[i]==N_EYE?.7f:.45f;c->px[i]=c->x[i]-(vx-vn*rx)*fr;c->py[i]=c->y[i]-(vy-vn*ry)*fr;}}
}
static void eat(Cr*c){
 int st=0;for(int i=0;i<c->n;i++)st+=c->nt[i]==N_STOMACH;
 float gain=38*(1+.35f*(st>3?3:st));
 for(int f=nf-1;f>=0;f--)for(int i=0;i<c->n;i++){float dx=fx[f]-c->x[i],dy=fy[f]-c->y[i],r=c->nt[i]==N_MOUTH?20:9;if(dx*dx+dy*dy<r*r){c->energy=clampf(c->energy+gain,-5,240);nf--;fx[f]=fx[nf];fy[f]=fy[nf];break;}}
}
static int cnt(Cr*c,int t){int n=0;for(int i=0;i<c->n;i++)n+=c->nt[i]==t;return n;}
static void hunt(Cr*c){
 for(int i=0;i<c->n;i++)if(c->nt[i]==N_MOUTH)for(int o=0;o<MAXC;o++){Cr*v=&cr[o];if(!v->alive||v->sid==c->sid)continue;
  for(int j=0;j<v->n;j++){float dx=v->x[j]-c->x[i],dy=v->y[j]-c->y[i];if(dx*dx+dy*dy<256){v->energy-=.45f*(1+.5f*cnt(c,N_CLAW))/(1+.5f*cnt(v,N_HORN));c->energy=clampf(c->energy+.35f,0,240);}}
 }
}
static void bodies_step(void){  // n-body no referencial do planeta (maré = termo indireto)
 float GM=G0*PR*PR;
 for(int i=0;i<nm;i++){
  float x=mx[i],y=my[i],d=sq(x*x+y*y)+.001f,k=-(GM*(1+mm[i]))/(d*d*d),ax=k*x,ay=k*y;
  for(int j=0;j<nm;j++)if(j!=i&&mm[j]>0){
   float dx=mx[j]-x,dy=my[j]-y,dj=sq(dx*dx+dy*dy)+mr[j],rj=sq(mx[j]*mx[j]+my[j]*my[j])+.001f,g=GM*mm[j];
   ax+=g*(dx/(dj*dj*dj)-mx[j]/(rj*rj*rj));ay+=g*(dy/(dj*dj*dj)-my[j]/(rj*rj*rj));}
  mvx[i]+=ax;mvy[i]+=ay;}
 for(int i=0;i<nm;){
  mx[i]+=mvx[i];my[i]+=mvy[i];float d=sq(mx[i]*mx[i]+my[i]*my[i]);int gone=d>80*PR;
  if(d<PR+mr[i]){impact(mx[i],my[i],mm[i]>0?mr[i]*3:mr[i]);gone=1;}
  if(gone){nm--;mx[i]=mx[nm];my[i]=my[nm];mvx[i]=mvx[nm];mvy[i]=mvy[nm];mr[i]=mr[nm];mm[i]=mm[nm];}else i++;}
}
static void step(void){
 sun+=.0006f;if(rnd()<food_rate){float a=rnd()*6.2832f;add_food(sn(a+1.5708f)*(PR+260),sn(a)*(PR+260));}
 for(int f=0;f<nf;f++){float d=sq(fx[f]*fx[f]+fy[f]*fy[f])+.001f,nd=d-.15f;if(nd<PR+4)nd=PR+4;fx[f]*=nd/d;fy[f]*=nd/d;}
 bodies_step();
 for(int i=0;i<MAXC;i++)if(cr[i].alive){float x=0,y=0;for(int j=0;j<cr[i].n;j++){x+=cr[i].x[j];y+=cr[i].y[j];}ccx[i]=x/cr[i].n;ccy[i]=y/cr[i].n;}
 for(int i=0;i<MAXC;i++)if(cr[i].alive){think(i);physics(&cr[i]);}
 for(int i=0;i<MAXC;i++)if(cr[i].alive){eat(&cr[i]);hunt(&cr[i]);}
 float cs=sn(sun+1.5708f),ss=sn(sun);
 for(int i=0;i<MAXC;i++)if(cr[i].alive){Cr*c=&cr[i];float activity=0,cost=.012f+.0035f*c->n;
  for(int s=0;s<c->m;s++)activity+=ab(c->act[s]);
  for(int j=0;j<c->n;j++){if(c->nt[j]==N_EYE)cost+=.0025f;if(c->nt[j]==N_STOMACH)cost+=.003f;
   if(c->nt[j]==N_LEAF){float d=sq(c->x[j]*c->x[j]+c->y[j]*c->y[j])+.001f,l=(c->x[j]*cs+c->y[j]*ss)/d;if(l>0)c->energy+=.075f*l;}}
  cost+=.001f*c->brain_size;c->age+=1;
  {float cd=sq(c->x[0]*c->x[0]+c->y[0]*c->y[0])+.001f,lt=(c->x[0]*cs+c->y[0]*ss)/cd,tl=.35f+temp+.4f*(lt>0?lt:0),dev=ab(tl-c->tpref);
   int hr=0,lu=0,gl=0,tt=0;
   for(int j=0;j<c->n;j++){int t=c->nt[j];hr+=t==N_HEART;lu+=t==N_LUNG;gl+=t==N_GILL;tt+=t==N_TAIL;if(t==N_WING)cost+=.002f;if(t==N_HEART)cost+=.003f;}
   int wet=cd<PR+sea;
   float mult=(1-.07f*(hr>3?3:hr))*(1+1.2f*dev)*(wet?(gl?1.f:2.2f):(gl&&!lu?1.8f:1.f))*(1-.06f*(lu>2?2:lu));
   activity*=1-.08f*(tt>3?3:tt);
   c->energy-=(cost+.006f*activity)*metab*mult;}
  if(c->energy<=0||!(c->x[0]==c->x[0])){die(c);continue;}
  c->energy=clampf(c->energy,0,240);if(c->energy>150&&total()<MAXC)reproduce(c);
 }
}
EXPORT void step_n(int n){if(n<0)n=0;if(n>1000)n=1000;while(n--)step();}
EXPORT int count_sid(int sid){int n=0;for(int i=0;i<MAXC;i++)n+=cr[i].alive&&cr[i].sid==sid;return n;}
EXPORT float avg_nodes(int sid){int n=0,t=0;for(int i=0;i<MAXC;i++)if(cr[i].alive&&cr[i].sid==sid){n++;t+=cr[i].n;}return n?(float)t/n:0;}
EXPORT float *render_ptr(void){return rb;}
EXPORT int render(void){
 int k=0;rb[k++]=nf;for(int f=0;f<nf;f++){rb[k++]=fx[f];rb[k++]=fy[f];}
 int ci=k++;for(int i=0;i<MAXC;i++)if(cr[i].alive){Cr*c=&cr[i];rb[k++]=c->sid;rb[k++]=c->n;rb[k++]=c->m;rb[k++]=c->energy;rb[k++]=c->brain_size;rb[k++]=c->body_scale;
  for(int j=0;j<c->n;j++){rb[k++]=c->x[j];rb[k++]=c->y[j];rb[k++]=c->nt[j];rb[k++]=(c->nt[j]==N_BRAIN?c->brain_size:1.f);rb[k++]=c->gx[j];rb[k++]=c->gy[j];}
  for(int s=0;s<c->m;s++){rb[k++]=c->sa[s];rb[k++]=c->sb[s];rb[k++]=c->st[s];rb[k++]=c->act[s];}}
 rb[ci]=0;for(int i=0;i<MAXC;i++)rb[ci]+=cr[i].alive;
 rb[k++]=nm;for(int i=0;i<nm;i++){rb[k++]=mx[i];rb[k++]=my[i];rb[k++]=mr[i];rb[k++]=mm[i];}return k;
}
