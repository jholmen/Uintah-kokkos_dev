
/*
 *  OpenGL.cc: Render geometry using opengl
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   December 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <tcl.h>
#include <tk.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <strstream.h>
#include <fstream.h>
#include <string.h>

#include "image.h"
#include <SCICore/Geom/GeomObj.h>
#include <SCICore/Containers/HashTable.h>
#include <SCICore/Util/Timer.h>
#include <SCICore/Geom/GeomObj.h>
#include <SCICore/Geom/GeomOpenGL.h>
#include <SCICore/Geom/Light.h>
#include <SCICore/Geom/Lighting.h>
#include <SCICore/Geom/RenderMode.h>
#include <SCICore/Geom/View.h>
#include <SCICore/Malloc/Allocator.h>
#include <SCICore/Math/Trig.h>
#include <SCICore/TclInterface/TCLTask.h>
#include <SCICore/Datatypes/Image.h>
#include <PSECore/Datatypes/GeometryPort.h>
#include <PSECommon/Modules/Salmon/Ball.h>
#include <PSECommon/Modules/Salmon/MpegEncoder.h>
#include <PSECommon/Modules/Salmon/Renderer.h>
#include <PSECommon/Modules/Salmon/Roe.h>
#include <PSECommon/Modules/Salmon/Salmon.h>
#include <SCICore/Thread/FutureValue.h>
#include <SCICore/Thread/Runnable.h>
#include <SCICore/Thread/Thread.h>

#ifdef __sgi
#include <X11/extensions/SGIStereo.h>
#include "imagelib.h"
#endif

extern "C" GLXContext OpenGLGetContext(Tcl_Interp*, char*);
extern Tcl_Interp* the_interp;

namespace PSECommon {
namespace Modules {

using PSECore::Datatypes::GeometryData;
using SCICore::Datatypes::DepthImage;

using SCICore::Datatypes::ColorImage;
using SCICore::GeomSpace::Light;
using SCICore::GeomSpace::Lighting;
using SCICore::GeomSpace::View;
using SCICore::Containers::to_string;
using SCICore::TclInterface::TCLTask;
using SCICore::Thread::Runnable;
using SCICore::Thread::Thread;

const int STRINGSIZE=200;
class OpenGLHelper;

#define DO_REDRAW 0
#define DO_PICK 1
#define DO_GETDATA 2
#define REDRAW_DONE 4
#define PICK_DONE 5

struct GetReq {
    int datamask;
    FutureValue<GeometryData*>* result;
    GetReq(int, FutureValue<GeometryData*>* result);
    GetReq();
};

class OpenGL : public Renderer {
    Tk_Window tkwin;
    Window win;
    Display* dpy;
    GLXContext cx;
    char* strbuf;
    int maxlights;
    SCICore::GeomSpace::DrawInfoOpenGL* drawinfo;
    WallClockTimer fpstimer;
    double current_time;

    int old_stereo;
    GLuint imglist;
    void make_image();

    void redraw_obj(Salmon* salmon, Roe* roe, GeomObj* obj);
    void pick_draw_obj(Salmon* salmon, Roe* roe, GeomObj* obj);
    OpenGLHelper* helper;
    clString my_openglname;
    SCICore::Containers::Array1<XVisualInfo*> visuals;

   /* Call this each time an mpeg frame is generated. */
   void addMpegFrame();
   MpegEncoder mpEncoder;
   bool encoding_mpeg;

public:
    OpenGL();
    virtual ~OpenGL();
    virtual clString create_window(Roe* roe,
				   const clString& name,
				   const clString& width,
				   const clString& height);
    virtual void redraw(Salmon*, Roe*, double tbeg, double tend,
			int ntimesteps, double frametime);
    void real_get_pick(Salmon*, Roe*, int, int, GeomObj*&, GeomPick*&, int&);
    virtual void get_pick(Salmon*, Roe*, int, int,
			  GeomObj*&, GeomPick*&, int& );
    virtual void hide();
    virtual void dump_image(const clString&);
    virtual void put_scanline(int y, int width, Color* scanline, int repeat=1);

    clString myname;
    void redraw_loop();
    SCICore::Thread::Mailbox<int> send_mb;
    SCICore::Thread::Mailbox<int> recv_mb;
    SCICore::Thread::Mailbox<GetReq> get_mb;

    Salmon* salmon;
    Roe* roe;
    double tbeg;
    double tend;
    int nframes;
    double framerate;
    void redraw_frame();

    int send_pick_x;
    int send_pick_y;

    GeomObj* ret_pick_obj;
    GeomPick* ret_pick_pick;
    int ret_pick_index;

    virtual void listvisuals(TCLArgs&);
    virtual void setvisual(const clString&, int i, int width, int height);

    View lastview;
    double znear, zfar;

    // these functions were added to clean things up a bit...

protected:
    
    void initState(void);

    virtual void getData(int datamask, FutureValue<GeometryData*>* result);
    virtual void real_getData(int datamask, FutureValue<GeometryData*>* result);
};

static OpenGL* current_drawer=0;
static const int pick_buffer_size = 512;
static const double pick_window = 5.0;

static Renderer* make_OpenGL()
{
    return scinew OpenGL;
}

static int query_OpenGL()
{
    TCLTask::lock();
    int have_opengl=glXQueryExtension(Tk_Display(Tk_MainWindow(the_interp)),
				      NULL, NULL);
    TCLTask::unlock();
    return have_opengl;
}

RegisterRenderer OpenGL_renderer("OpenGL", &query_OpenGL, &make_OpenGL);

OpenGL::OpenGL()
: tkwin(0), send_mb("OpenGL renderer send mailbox",10),
  recv_mb("OpenGL renderer receive mailbox", 10), helper(0),
  get_mb("OpenGL renderer request mailbox", 5)
{
    encoding_mpeg = false;
    strbuf=scinew char[STRINGSIZE];
    drawinfo=scinew SCICore::GeomSpace::DrawInfoOpenGL;
    fpstimer.start();
}

OpenGL::~OpenGL()
{
    fpstimer.stop();
    delete[] strbuf;
    
    if(encoding_mpeg) // make sure we finish up mpeg that was in progress
      encoding_mpeg = false;
}

clString OpenGL::create_window(Roe*,
			       const clString& name,
			       const clString& width,
			       const clString& height)
{
    myname=name;
    width.get_int(xres);
    height.get_int(yres);
    static int direct=1;
    int d=direct;
    direct=0;
    return "opengl "+name+" -geometry "+width+"x"+height+" -doublebuffer true -direct "+(d?"true":"false")+" -rgba true -redsize 1 -greensize 1 -bluesize 1 -depthsize 2";
}

void OpenGL::initState(void)
{
    
}

class OpenGLHelper : public Runnable {
    OpenGL* opengl;
public:
    OpenGLHelper(OpenGL* opengl);
    virtual ~OpenGLHelper();
    virtual void run();
};

OpenGLHelper::OpenGLHelper(OpenGL* opengl)
: opengl(opengl)
{
}

OpenGLHelper::~OpenGLHelper()
{
}

void OpenGLHelper::run()
{
    opengl->redraw_loop();
}

void OpenGL::redraw(Salmon* s, Roe* r, double _tbeg, double _tend,
		    int _nframes, double _framerate)
{
    salmon=s;
    roe=r;
    tbeg=_tbeg;
    tend=_tend;
    nframes=_nframes;
    framerate=_framerate;
    // This is the first redraw - if there is not an OpenGL thread,
    // start one...
    if(!helper){
	my_openglname=clString("OpenGL: ")+myname;
	helper=new OpenGLHelper(this);
	Thread* t=new Thread(helper, my_openglname());
	t->detach();
    }

    send_mb.send(DO_REDRAW);
    int rc=recv_mb.receive();
    if(rc != REDRAW_DONE){
	cerr << "Wanted redraw_done, but got: " << r << endl;
    }
}


void OpenGL::redraw_loop()
{
    // Tell the Roe that we are started...
    TimeThrottle throttle;
    throttle.start();
    double newtime=0;
    for(;;){
	int nreply=0;
	if(roe->inertia_mode){
	    double current_time=throttle.time();
	    if(framerate==0)
		framerate=30;
	    double frametime=1./framerate;
	    double delta=current_time-newtime;
	    if(delta > 1.5*frametime){
		framerate=1./delta;
		frametime=delta;
		newtime=current_time;
	    } if(delta > .85*frametime){
		framerate*=.9;
		frametime=1./framerate;
		newtime=current_time;
	    } else if(delta < .5*frametime){
		framerate*=1.1;
		if(framerate>30)
		    framerate=30;
		frametime=1./framerate;
		newtime=current_time;
	    }
	    newtime+=frametime;
	    throttle.wait_for_time(newtime);

	    for(;;){
		int r;
		if(!send_mb.tryReceive(r))
		    break;
		if(r == DO_PICK){
		    real_get_pick(salmon, roe, send_pick_x, send_pick_y, ret_pick_obj, ret_pick_pick, ret_pick_index);
		    recv_mb.send(PICK_DONE);
		} else if(r== DO_GETDATA){
		    GetReq req(get_mb.receive());
		    real_getData(req.datamask, req.result);
		} else {
		    // Gobble them up...
		    nreply++;
		}
	    }

	    // you want to just rotate around the current rotation
	    // axis - the current quaternion is roe->ball->qNow	    
	    // the first 3 components of this 

	    roe->ball->SetAngle(newtime*roe->angular_v);

	    View tmpview(roe->rot_view);
	    
	    Transform tmp_trans;
	    HMatrix mNow;
	    roe->ball->Value(mNow);
	    tmp_trans.set(&mNow[0][0]);
	    
	    Transform prv = roe->prev_trans;
	    prv.post_trans(tmp_trans);
	    
	    HMatrix vmat;
	    prv.get(&vmat[0][0]);
	    
	    Point y_a(vmat[0][1],vmat[1][1],vmat[2][1]);
	    Point z_a(vmat[0][2],vmat[1][2],vmat[2][2]);
	    
	    tmpview.up(y_a.vector());
	    tmpview.eyep((z_a*(roe->eye_dist)) + tmpview.lookat().vector());
	    
	    roe->view.set(tmpview);	    
	} else {
	    for(;;){
		int r=send_mb.receive();
		if(r == DO_PICK){
		    real_get_pick(salmon, roe, send_pick_x, send_pick_y, ret_pick_obj, ret_pick_pick, ret_pick_index);
		    recv_mb.send(PICK_DONE);
		} else if(r== DO_GETDATA){
		    GetReq req(get_mb.receive());
		    real_getData(req.datamask, req.result);
		} else {
		    nreply++;
		    break;
		}
	    }
	    newtime=throttle.time();
	    throttle.stop();
	    throttle.clear();
	    throttle.start();
	}
	redraw_frame();
	for(int i=0;i<nreply;i++)
	    recv_mb.send(REDRAW_DONE);
    }
}

void OpenGL::make_image()
{
  imglist=glGenLists(1);
  glNewList(imglist, GL_COMPILE_AND_EXECUTE);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, xres-1, 0.0, yres-1, -10.0, 10.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_DEPTH_TEST);
  glRasterPos2i(xres-168-5, yres-5);
  glDrawPixels(168, 95, GL_LUMINANCE, GL_UNSIGNED_BYTE, logoimg);
  glEnable(GL_DEPTH_TEST);
  glEndList();
}

void OpenGL::redraw_frame()
{
    // Get window information
    TCLTask::lock();
    Tk_Window new_tkwin=Tk_NameToWindow(the_interp,
					const_cast<char *>(myname()),
					Tk_MainWindow(the_interp));
    if(!new_tkwin){
      cerr << "Unable to locate window!\n";
      TCLTask::unlock();
      return;
    }
    if(tkwin != new_tkwin){
      tkwin=new_tkwin;
      dpy=Tk_Display(tkwin);
      win=Tk_WindowId(tkwin);
      cx=OpenGLGetContext(the_interp, const_cast<char *>(myname()));
      if(!cx){
	cerr << "Unable to create OpenGL Context!\n";
	TCLTask::unlock();
	return;
      }
      glXMakeCurrent(dpy, win, cx);
      glXWaitX();
      current_drawer=this;
      GLint data[1];
      glGetIntegerv(GL_MAX_LIGHTS, data);
      maxlights=data[0];
      // Look for multisample extension...
#ifdef __sgi
      if(strstr((char*)glGetString(GL_EXTENSIONS), "GL_SGIS_multisample")){
        cerr << "Enabling multisampling...\n";
	glEnable(GL_MULTISAMPLE_SGIS);
	glSamplePatternSGIS(GL_1PASS_SGIS);
      }
#endif
    }

    TCLTask::unlock();

    // Start polygon counter...
    WallClockTimer timer;
    timer.clear();
    timer.start();

    initState();

    // Get the window size
    xres=Tk_Width(tkwin);
    yres=Tk_Height(tkwin);

    // Make ourselves current
    if(current_drawer != this){
	current_drawer=this;
	TCLTask::lock();
	glXMakeCurrent(dpy, win, cx);
	TCLTask::unlock();
    }

    // Get a lock on the geometry database...
    // Do this now to prevent a hold and wait condition with TCLTask
    salmon->geomlock.readLock();

    TCLTask::lock();

    // Clear the screen...
    glViewport(0, 0, xres, yres);
    Color bg(roe->bgcolor.get());
    glClearColor(bg.r(), bg.g(), bg.b(), 1);

    clString saveprefix(roe->saveprefix.get());

    // Setup the view...
    View view(roe->view.get());
    lastview=view;
    double aspect=double(xres)/double(yres);
    double fovy=RtoD(2*Atan(aspect*Tan(DtoR(view.fov()/2.))));

    drawinfo->reset();
    int do_stereo=roe->do_stereo.get();

    // Compute znear and zfar...
    if(compute_depth(roe, view, znear, zfar)){


	// Set up graphics state
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	
	clString globals("global");
	roe->setState(drawinfo,globals);
	drawinfo->pickmode=0;

	int errcode;
	while((errcode=glGetError()) != GL_NO_ERROR){
	    cerr << "We got an error from GL: " << (char*)gluErrorString(errcode) << endl;
	}

	// Do the redraw loop for each time value
	double dt=(tend-tbeg)/nframes;
	double frametime=framerate==0?0:1./framerate;
	TimeThrottle throttle;
	throttle.start();
	Vector eyesep(0,0,0);
	if(do_stereo){
	    double eye_sep_dist=0.025/2;
	    Vector u, v;
	    view.get_viewplane(aspect, 1.0, u, v);
	    u.normalize();
	    double zmid=(znear+zfar)/2.;
	    eyesep=u*eye_sep_dist*zmid;
	}
	for(int t=0;t<nframes;t++){
	  int n=1;
	  if(do_stereo)
	    n=2;
	  for(int i=0;i<n;i++){
	    if(do_stereo){
	      glDrawBuffer(i==0?GL_BACK_LEFT:GL_BACK_RIGHT);
	    } else {
	      glDrawBuffer(GL_BACK);
	    }

	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    double modeltime=t*dt+tbeg;
	    roe->set_current_time(modeltime);

	    // Setup view...
	    glViewport(0, 0, xres, yres);
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    gluPerspective(fovy, aspect, znear, zfar);
	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();
	    Point eyep(view.eyep());
	    Point lookat(view.lookat());
	    if(do_stereo){
	      if(i==0){
		eyep-=eyesep;
		lookat-=eyesep;
	      } else {
		eyep+=eyesep;
		lookat+=eyesep;
	      }
	    }
	    Vector up(view.up());
	    gluLookAt(eyep.x(), eyep.y(), eyep.z(),
		      lookat.x(), lookat.y(), lookat.z(),
		      up.x(), up.y(), up.z());

	    // Set up Lighting
	    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	    Lighting& l=salmon->lighting;
	    int idx=0;
	    int ii;
	    for(ii=0;ii<l.lights.size();ii++){
	      Light* light=l.lights[ii];
	      light->opengl_setup(view, drawinfo, idx);
	    }
	    for(ii=0;ii<idx && ii<maxlights;ii++)
	      glEnable(GL_LIGHT0+ii);
	    for(;ii<maxlights;ii++)
	      glDisable(GL_LIGHT0+ii);

	    // now set up the fog stuff

	    glFogi(GL_FOG_MODE,GL_LINEAR);
	    glFogf(GL_FOG_START,float(znear));
	    glFogf(GL_FOG_END,float(zfar));
	    // now make the Roe setup its clipping planes...
	    roe->setClip(drawinfo);
	         
	    // Draw it all...
	    current_time=modeltime;
	    roe->do_for_visible(this, (RoeVisPMF)&OpenGL::redraw_obj);
	  }

#if 0
	  if(roe->drawimg.get()){
	    if(!imglist)
	      make_image();
	    else
	      glCallList(imglist);
	  }
#endif

	  // Wait for the right time before swapping buffers
	  //TCLTask::unlock();
	  double realtime=t*frametime;
	  throttle.wait_for_time(realtime);
	  //TCLTask::lock();
	  TCL::execute("update idletasks");

	  // Show the pretty picture
	  glXSwapBuffers(dpy, win);
#ifdef __sgi
	  if(saveprefix != ""){
	    // Save out the image...
	    char filename[200];
	    sprintf(filename, "%s%04d.rgb", saveprefix(), t);
	    unsigned short* reddata=scinew unsigned short[xres*yres];
	    unsigned short* greendata=scinew unsigned short[xres*yres];
	    unsigned short* bluedata=scinew unsigned short[xres*yres];
	    glReadPixels(0, 0, xres, yres, GL_RED, GL_UNSIGNED_SHORT, reddata);
	    glReadPixels(0, 0, xres, yres, GL_GREEN, GL_UNSIGNED_SHORT, greendata);
	    glReadPixels(0, 0, xres, yres, GL_BLUE, GL_UNSIGNED_SHORT, bluedata);
	    IMAGE* image=iopen(filename, "w", RLE(1), 3, xres, yres, 3);
	    unsigned short* rr=reddata;
	    unsigned short* gg=greendata;
	    unsigned short* bb=bluedata;
	    for(int y=0;y<yres;y++){
	      for(int x=0;x<xres;x++){
		rr[x]>>=8;
		gg[x]>>=8;
		bb[x]>>=8;
	      }
	      putrow(image, rr, y, 0);
	      putrow(image, gg, y, 1);
	      putrow(image, bb, y, 2);
	      rr+=xres;
	      gg+=xres;
	      bb+=xres;
	    }
	    iclose(image);
	    delete[] reddata;
	    delete[] greendata;
	    delete[] bluedata;
	  }
#endif
	}
	throttle.stop();
	double fps=nframes/throttle.time();
	int fps_whole=(int)fps;
	int fps_hund=(int)((fps-fps_whole)*100);
	ostrstream str(strbuf, STRINGSIZE);
	str << roe->id << " setFrameRate " << fps_whole << "." << fps_hund << '\0';
	TCL::execute(str.str());
	roe->set_current_time(tend);
    } else {
	// Just show the cleared screen
	roe->set_current_time(tend);
	if(do_stereo){
	    glDrawBuffer(GL_BACK_LEFT);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    glDrawBuffer(GL_BACK_RIGHT);
        } else {
	    glDrawBuffer(GL_BACK);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(roe->drawimg.get()){
	  if(!imglist)
	    make_image();
	  else
	    glCallList(imglist);
	}
	glXSwapBuffers(dpy, win);
    }
    salmon->geomlock.readUnlock();

    // Look for errors
    int errcode;
    while((errcode=glGetError()) != GL_NO_ERROR){
	cerr << "We got an error from GL: " << (char*)gluErrorString(errcode) << endl;
    }

    // Report statistics
    timer.stop();
    fpstimer.stop();
    double fps=nframes/fpstimer.time();
    fps+=0.05; // Round to nearest tenth
    int fps_whole=(int)fps;
    int fps_tenths=(int)((fps-fps_whole)*10);
    fpstimer.clear();
    fpstimer.start(); // Start it running for next time
    ostrstream str(strbuf, STRINGSIZE);
    str << roe->id << " updatePerf \"";
    str << drawinfo->polycount << " polygons in " << timer.time()
	<< " seconds\" \"" << drawinfo->polycount/timer.time()
	    << " polygons/second\"" << " \"" << fps_whole << "."
		<< fps_tenths << " frames/sec\""	<< '\0';
    if (roe->doingMovie) {
//      cerr << "Saving a movie!\n";
      unsigned char movie[10];
      int startDiv = 100;
      int idx=0;
      int fi = roe->curFrame;
      while (startDiv >= 1) {
	movie[idx] = '0' + fi/startDiv;
	fi = fi - (startDiv)*(fi/startDiv);
	startDiv /= 10;
	idx++;
      }
      movie[idx] = 0;
      clString segname(roe->curName);

      int lasthash=-1;
      for (int ii=0; ii<segname.len(); ii++) {
	  if (segname()[ii] == '/') lasthash=ii;
      }
      clString pathname;
      if (lasthash == -1) pathname = "./";
      else pathname = segname.substr(0, lasthash+1);
      clString fname = segname.substr(lasthash+1, -1);
      fname = fname + ".raw";
      clString framenum((char *)movie);
      framenum = framenum + ".";
      clString fullpath(pathname + framenum + fname);
      cerr << "Dumping "<<fullpath<<"....  ";
      dump_image(fullpath);
      cerr << " done!\n";
      roe->curFrame++;
    }
    TCL::execute(str.str());
    TCLTask::unlock();
}

void OpenGL::hide()
{
    tkwin=0;
    if(current_drawer==this)
	current_drawer=0;
}


void OpenGL::get_pick(Salmon*, Roe*, int x, int y,
		      GeomObj*& pick_obj, GeomPick*& pick_pick,
		      int& pick_index)
{
    send_pick_x=x;
    send_pick_y=y;
    send_mb.send(DO_PICK);
    for(;;){
	int r=recv_mb.receive();
	if(r != PICK_DONE){
	    cerr << "WANTED A PICK!!! (got back " << r << endl;
	} else {
	    pick_obj=ret_pick_obj;
	    pick_pick=ret_pick_pick;
	    pick_index=ret_pick_index;
	    break;
	}
    }
}

void OpenGL::real_get_pick(Salmon*, Roe* roe, int x, int y,
			   GeomObj*& pick_obj, GeomPick*& pick_pick,
			   int& pick_index)
{
    pick_obj=0;
    pick_pick=0;
    pick_index = 0x12345678;
    // Make ourselves current
    if(current_drawer != this){
	current_drawer=this;
	TCLTask::lock();
	glXMakeCurrent(dpy, win, cx);
	TCLTask::unlock();
    }
    // Setup the view...
    View view(roe->view.get());
    double aspect=double(xres)/double(yres);
    double fovy=RtoD(2*Atan(aspect*Tan(DtoR(view.fov()/2.))));

    salmon->geomlock.readLock();

    // Compute znear and zfar...
    double znear;
    double zfar;
    if(compute_depth(roe, view, znear, zfar)){
	// Setup picking...
	TCLTask::lock();

	GLuint pick_buffer[pick_buffer_size];
	glSelectBuffer(pick_buffer_size, pick_buffer);
	glRenderMode(GL_SELECT);
	glInitNames();
#if (_MIPS_SZPTR == 64)
	glPushName(0);
	glPushName(0);
	glPushName(0x12345678);
#else
	glPushName(0);
	glPushName(0x12345678);
#endif

	glViewport(0, 0, xres, yres);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluPickMatrix(x, viewport[3]-y, pick_window, pick_window, viewport);
	gluPerspective(fovy, aspect, znear, zfar);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Point eyep(view.eyep());
	Point lookat(view.lookat());
	Vector up(view.up());
	gluLookAt(eyep.x(), eyep.y(), eyep.z(),
		  lookat.x(), lookat.y(), lookat.z(),
		  up.x(), up.y(), up.z());

	drawinfo->lighting=0;
	drawinfo->set_drawtype(DrawInfoOpenGL::Flat);
	drawinfo->pickmode=1;
	//drawinfo->pickable=0;
	
	// Draw it all...
	roe->do_for_visible(this, (RoeVisPMF)&OpenGL::pick_draw_obj);

#if (_MIPS_SZPTR == 64)
	glPopName();
	glPopName();
	glPopName();
#else
	glPopName();
	glPopName();
#endif

	glFlush();
	int hits=glRenderMode(GL_RENDER);
	int errcode;
	while((errcode=glGetError()) != GL_NO_ERROR){
	    cerr << "We got an error from GL: " << (char*)gluErrorString(errcode) << endl;
	}
	TCLTask::unlock();
	GLuint min_z;
#if (_MIPS_SZPTR == 64)
	unsigned long hit_obj=0;
	GLuint hit_obj_index = 0x12345678;
	unsigned long hit_pick=0;
	GLuint hit_pick_index = 0x12345678;  // need for object indexing
#else
	GLuint hit_obj=0;
	GLuint hit_obj_index = 0x12345678;  // need for object indexing
	GLuint hit_pick=0;
	GLuint hit_pick_index = 0x12345678;  // need for object indexing
#endif
	cerr << "hits=" << hits << endl;
	if(hits >= 1){
	    int idx=0;
	    min_z=0;
	    int have_one=0;
	    for (int h=0; h<hits; h++) {
		int nnames=pick_buffer[idx++];
		GLuint z=pick_buffer[idx++];
		cerr << "h=" << h << ", nnames=" << nnames << ", z=" << z << endl;
		if (nnames > 1 && (!have_one || z < min_z)) {
		    min_z=z;
		    have_one=1;
		    idx++; // Skip Max Z
#if (_MIPS_SZPTR == 64)
		    unsigned int ho1=pick_buffer[idx++];
		    unsigned int ho2=pick_buffer[idx++];
		    hit_obj=((long)ho1<<32)|ho2;
		    hit_obj_index = pick_buffer[idx++];
		    idx+=nnames-6; // Skip to the last one...
		    unsigned int hp1=pick_buffer[idx++];
		    unsigned int hp2=pick_buffer[idx++];
		    hit_pick=((long)hp1<<32)|hp2;
		    hit_pick_index = pick_buffer[idx++];
#else
		    hit_obj=pick_buffer[idx++];
		    hit_obj_index=pick_buffer[idx++];
		    idx+=nnames-4; // Skip to the last one...
		    hit_pick=pick_buffer[idx++];
		    hit_pick_index=pick_buffer[idx++];
#endif
		    cerr << "new min... (obj=" << hit_obj
			 << ", pick="          << hit_pick
			 << ", index = "       << hit_pick_index << ")\n";
		} else {
		    idx+=nnames+1;
		}
	    }
	}
	pick_obj=(GeomObj*)hit_obj;
	pick_pick=(GeomPick*)hit_pick;
	pick_index=(int)hit_pick_index;
	cerr << "pick_pick=" << pick_pick << ", pick_index="<<pick_index<<endl;
    }
    salmon->geomlock.readUnlock();
}

void OpenGL::dump_image(const clString& name) {
    ofstream dumpfile(name());
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT,vp);
    int n=3*vp[2]*vp[3];
    cerr << "Dumping: " << vp[2] << "x" << vp[3] << endl;
    unsigned char* pxl=scinew unsigned char[n];
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0,0,vp[2],vp[3],GL_RGB,GL_UNSIGNED_BYTE,pxl);
    dumpfile.write((const char *)pxl,n);
    delete[] pxl;
}

void OpenGL::put_scanline(int y, int width, Color* scanline, int repeat)
{
    float* pixels=scinew float[width*3];
    float* p=pixels;
    int i;
    for(i=0;i<width;i++){
	*p++=scanline[i].r();
	*p++=scanline[i].g();
	*p++=scanline[i].b();
    }
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslated(-1, -1, 0);
    glScaled(2./xres, 2./yres, 1.0);
    glDepthFunc(GL_ALWAYS);
    glDrawBuffer(GL_FRONT);
    for(i=0;i<repeat;i++){
	glRasterPos2i(0, y+i);
	glDrawPixels(width, 1, GL_RGB, GL_FLOAT, pixels);
    }
    glDepthFunc(GL_LEQUAL);
    glDrawBuffer(GL_BACK);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    delete[] pixels;
}

void OpenGL::pick_draw_obj(Salmon* salmon, Roe*, GeomObj* obj)
{
#if (_MIPS_SZPTR == 64)
    unsigned long o=(unsigned long)obj;
    unsigned int o1=(o>>32)&0xffffffff;
    unsigned int o2=o&0xffffffff;
    glPopName();
    glPopName();
    glPopName();
    glPushName(o1);
    glPushName(o2);
    glPushName(0x12345678);
#else
    glPopName();
    glLoadName((GLuint)obj);
    glPushName(0x12345678);
#endif
    obj->draw(drawinfo, salmon->default_matl.get_rep(), current_time);
}

void OpenGL::redraw_obj(Salmon* salmon, Roe* roe, GeomObj* obj)
{
    drawinfo->roe = roe;
    obj->draw(drawinfo, salmon->default_matl.get_rep(), current_time);
}

void Roe::setState(DrawInfoOpenGL* drawinfo,clString tclID)
{
    clString val;
    double dval;
    clString type(tclID+"-"+"type");
    clString lighting(tclID+"-"+"light");
    clString fog(tclID+"-"+"fog");
    clString cull(tclID+"-"+"cull");
    clString debug(tclID+"-"+"debug");
    clString psize(tclID+"-"+"psize");
    clString movie(tclID+"-"+"movie");
    clString movieName(tclID+"-"+"movieName");
    clString movieFrame(tclID+"-"+"movieFrame");
    clString use_clip(tclID+"-"+"clip");

    if (!get_tcl_stringvar(id,type,val)) {
	cerr << "Error illegal name!\n";
	return;
    }
    else {
	if(val == "Wire"){
	    drawinfo->set_drawtype(DrawInfoOpenGL::WireFrame);
	    drawinfo->lighting=0;
	} else if(val == "Flat"){
	    drawinfo->set_drawtype(DrawInfoOpenGL::Flat);
	    drawinfo->lighting=0;
	} else if(val == "Gouraud"){
	    drawinfo->set_drawtype(DrawInfoOpenGL::Gouraud);
	    drawinfo->lighting=1;
	}
	else if (val == "Default") {
//	    drawinfo->currently_lit=drawinfo->lighting;
//	    drawinfo->init_lighting(drawinfo->lighting);
	    clString globals("global");
	    setState(drawinfo,globals);	    
	    return; // if they are using the default, con't change
	} else {
	    cerr << "Unknown shading(" << val << "), defaulting to phong" << endl;
	    drawinfo->set_drawtype(DrawInfoOpenGL::Gouraud);
	    drawinfo->lighting=1;
	}

	// now see if they want a bounding box...

	if (get_tcl_stringvar(id,debug,val)) {
	    if (val == "0")
		drawinfo->debug = 0;
	    else
		drawinfo->debug = 1;
	}	
	else {
	    cerr << "Error, no debug level set!\n";
	    drawinfo->debug = 0;
	}

        if (get_tcl_doublevar(id,psize,dval)) {
            drawinfo->point_size = dval;
        }

	if (get_tcl_stringvar(id,use_clip,val)) {
	    if (val == "0")
		drawinfo->check_clip = 0;
	    else
		drawinfo->check_clip = 1;
	}	
	else {
	    cerr << "Error, no clipping info\n";
	    drawinfo->check_clip = 0;
	}

	// only set with globals...
	if (get_tcl_stringvar(id,movie,val)) {
	    get_tcl_stringvar(id,movieName,curName);
	    clString curFrameStr;
	    get_tcl_stringvar(id,movieFrame,curFrameStr);
//	    curFrameStr.get_int(curFrame);
//	    cerr << "curFrameStr="<<curFrameStr<<"  curFrame="<<curFrame<<"\n";
	    if (val == "0") {
		doingMovie = 0;
	    } else {

		if (!doingMovie) {
		    doingMovie = 1;
		    curFrame=0;
		}
	    }
	}

	drawinfo->init_clip(); // set clipping 

	if (get_tcl_stringvar(id,cull,val)) {
	    if (val == "0")
		drawinfo->cull = 0;
	    else
		drawinfo->cull = 1;
	}	
	else {
	    cerr << "Error, no culling info\n";
	    drawinfo->cull = 0;
	}
	if (!get_tcl_stringvar(id,lighting,val))
	    cerr << "Error, no lighting!\n";
	else {
	    if (val == "0"){
		drawinfo->lighting=0;
	    }
	    else if (val == "1") {
		drawinfo->lighting=1;
	    }
	    else {
		cerr << "Unknown lighting setting(" << val << "\n";
	    }

	    if (get_tcl_stringvar(id,fog,val)) {
		if (val=="0"){
		    drawinfo->fog=0;
		}
		else {
		    drawinfo->fog=1;
		}
	    }
	    else {
		cerr << "Fog not defined properly!\n";
		drawinfo->fog=0;
	    }

	}
    }
    drawinfo->currently_lit=drawinfo->lighting;
    drawinfo->init_lighting(drawinfo->lighting);
	
    
}

void Roe::setDI(DrawInfoOpenGL* drawinfo,clString name)
{
    ObjTag* vis;

    if (visible.lookup(name,vis)){
	setState(drawinfo,to_string(vis->tagid));
    }
}

// set the bits for the clipping planes that are on...

void Roe::setClip(DrawInfoOpenGL* drawinfo)
{
    clString val;
    int i;

    drawinfo->clip_planes = 0; // set them all of for default
    clString num_clip("clip-num");

    if (get_tcl_stringvar(id,"clip-visible",val) && 
	get_tcl_intvar(id,num_clip,i)) {

	int cur_flag = SCICore::GeomSpace::CLIP_P5;
	if ( (i>0 && i<7) ) {
	    while(i--) {
		
		clString vis("clip-visible-"+to_string(i+1));


		if (get_tcl_stringvar(id,vis,val)) {
		    if (val == "1") {
			double plane[4];
			clString nx("clip-normal-x-"+to_string(i+1));
			clString ny("clip-normal-y-"+to_string(i+1));
			clString nz("clip-normal-z-"+to_string(i+1));
			clString nd("clip-normal-d-"+to_string(i+1));
			
			int rval=0;

			rval = get_tcl_doublevar(id,nx,plane[0]);
			rval = get_tcl_doublevar(id,ny,plane[1]);
			rval = get_tcl_doublevar(id,nz,plane[2]);
			rval = get_tcl_doublevar(id,nd,plane[3]);
			
			double mag = plane[0]*plane[0] +
			    plane[1]*plane[1] +
				plane[2]*plane[2];
			plane[0] /= mag;
			plane[1] /= mag;
			plane[2] /= mag;
			plane[3] = -plane[3]; // so moves in planes direction...
			glClipPlane(GL_CLIP_PLANE0+i,plane);

			if (drawinfo->check_clip)
			    glEnable(GL_CLIP_PLANE0+i);
			else
			    glDisable(GL_CLIP_PLANE0+i);
			    
			drawinfo->clip_planes |= cur_flag;

			if (!rval ) {
			    cerr << "Error, variable is hosed!\n";
			}
		    }
		    else {
			glDisable(GL_CLIP_PLANE0+i);
		    }

		}
		cur_flag >>= 1; // shift the bit we are looking at...
	    }
	}
    }
}


void GeomSalmonItem::draw(DrawInfoOpenGL* di, Material *m, double time)
{
    // here we need to query the roe with our name and give it our
    // di so it can change things if they need to be...
    di->roe->setDI(di,name);

    // lets get the childs bounding box, and draw it...

    BBox bb;
    //    child->reset_bbox();
    child->get_bounds(bb);
    if(!bb.valid())
	return;

    // might as well try and draw the arcball also...

    Point min,max;

    min = bb.min();
    max = bb.max();
    if (!di->debug)
	child->draw(di,m,time);

    if (di->debug) {

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);

	glColor4f(1.0,0.0,1.0,0.2);

	glDisable(GL_LIGHTING);	

	glBegin(GL_QUADS);
	//front
	glVertex3d(max.x(),min.y(),max.z());
	//	glColor4f(0.0,1.0,0.0,0.8);
	glVertex3d(max.x(),max.y(),max.z());
	glColor4f(0.0,1.0,0.0,0.2);
	glVertex3d(min.x(),max.y(),max.z());
	glVertex3d(min.x(),min.y(),max.z());
	//back
	glVertex3d(max.x(),max.y(),min.z());
	glVertex3d(max.x(),min.y(),min.z());
	//	glColor4f(1.0,0.0,0.0,0.8);
	glVertex3d(min.x(),min.y(),min.z());
	glColor4f(0.0,1.0,0.0,0.2);
	glVertex3d(min.x(),max.y(),min.z());

	glColor4f(1.0,0.0,0.0,0.2);

	//left
	glVertex3d(min.x(),min.y(),max.z());
	glVertex3d(min.x(),max.y(),max.z());
	glVertex3d(min.x(),max.y(),min.z());
	//	glColor4f(1.0,0.0,0.0,0.8);
	glVertex3d(min.x(),min.y(),min.z());
	glColor4f(1.0,0.0,0.0,0.2);

	//right
	glVertex3d(max.x(),min.y(),min.z());
	glVertex3d(max.x(),max.y(),min.z());
	//	glColor4f(0.0,1.0,0.0,0.8);
	glVertex3d(max.x(),max.y(),max.z());
	glColor4f(1.0,0.0,0.0,0.2);
	glVertex3d(max.x(),min.y(),max.z());


	glColor4f(0.0,0.0,1.0,0.2);

	//top
	glVertex3d(min.x(),max.y(),max.z());
	//	glColor4f(0.0,1.0,0.0,0.8);
	glVertex3d(max.x(),max.y(),max.z());
	glColor4f(0.0,0.0,1.0,0.2);
	glVertex3d(max.x(),max.y(),min.z());
	glVertex3d(min.x(),max.y(),min.z());
	//bottom
	//	glColor4f(1.0,0.0,0.0,0.8);
	glVertex3d(min.x(),min.y(),min.z());
	glColor4f(0.0,0.0,1.0,0.2);
	glVertex3d(max.x(),min.y(),min.z());
	glVertex3d(max.x(),min.y(),max.z());
	glVertex3d(min.x(),min.y(),max.z());

	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
    }
}

#define GETCONFIG(attrib) \
if(glXGetConfig(dpy, &vinfo[i], attrib, &value) != 0){\
  args.error("Error getting attribute: " #attrib); \
  return; \
}

void OpenGL::listvisuals(TCLArgs& args)
{
  TCLTask::lock();

  Tk_Window topwin=Tk_NameToWindow(the_interp,
				   const_cast<char *>(args[2]()),
				   Tk_MainWindow(the_interp));
  if(!topwin){
    cerr << "Unable to locate window!\n";
    TCLTask::unlock();
    return;
  }
  dpy=Tk_Display(topwin);
  int screen=Tk_ScreenNumber(topwin);
  Array1<clString> visualtags;
  Array1<int> scores;
  visuals.remove_all();
  int nvis;
  XVisualInfo* vinfo=XGetVisualInfo(dpy, 0, NULL, &nvis);
  if(!vinfo){
    args.error("XGetVisualInfo failed");
    return;
  }
  int i;
  for(i=0;i<nvis;i++){
    int score=0;
    int value;
    GETCONFIG(GLX_USE_GL);
    if(!value)
      continue;
    GETCONFIG(GLX_RGBA);
    if(!value)
      continue;
    GETCONFIG(GLX_LEVEL);
    if(value != 0)
      continue;
    if(vinfo[i].screen != screen)
	continue;
    char buf[20];
    sprintf(buf, "id=%02x, ", vinfo[i].visualid);
    clString tag(buf);
    GETCONFIG(GLX_DOUBLEBUFFER);
    if(value){
      score+=200;
      tag+=clString("double, ");
    } else {
      tag+=clString("single, ");
    }
    GETCONFIG(GLX_STEREO);
    if(value){
	score+=1;
      tag+=clString("stereo, ");
    }
    tag+=clString("rgba=");
    GETCONFIG(GLX_RED_SIZE);
    tag+=to_string(value)+":";
    score+=value;
    GETCONFIG(GLX_GREEN_SIZE);
    tag+=to_string(value)+":";
    score+=value;
    GETCONFIG(GLX_BLUE_SIZE);
    tag+=to_string(value)+":";
    score+=value;
    GETCONFIG(GLX_ALPHA_SIZE);
    tag+=to_string(value);
    score+=value;
    GETCONFIG(GLX_DEPTH_SIZE);
    tag+=clString(", depth=")+to_string(value);
    score+=value*5;
    GETCONFIG(GLX_STENCIL_SIZE);
    tag+=clString(", stencil=")+to_string(value);
    tag+=clString(", accum=");
    GETCONFIG(GLX_ACCUM_RED_SIZE);
    tag+=to_string(value)+":";
    GETCONFIG(GLX_ACCUM_GREEN_SIZE);
    tag+=to_string(value)+":";
    GETCONFIG(GLX_ACCUM_BLUE_SIZE);
    tag+=to_string(value)+":";
    GETCONFIG(GLX_ACCUM_ALPHA_SIZE);
    tag+=to_string(value);
#ifdef __sgi
    tag+=clString(", samples=");
    GETCONFIG(GLX_SAMPLES_SGIS);
    if(value)
      score+=50;
#endif
    tag+=to_string(value);

    tag+=clString(", score=")+to_string(score);
    //cerr << score << ": " << tag << '\n';

    visualtags.add(tag);
    visuals.add(&vinfo[i]);
    scores.add(score);
  }
  for(i=0;i<scores.size()-1;i++){
    for(int j=i+1;j<scores.size();j++){
      if(scores[i] < scores[j]){
	// Swap...
	int tmp1=scores[i];
	scores[i]=scores[j];
	scores[j]=tmp1;
	clString tmp2=visualtags[i];
	visualtags[i]=visualtags[j];
	visualtags[j]=tmp2;
	XVisualInfo* tmp3=visuals[i];
	visuals[i]=visuals[j];
	visuals[j]=tmp3;
      }
    }
  }
  args.result(TCLArgs::make_list(visualtags));
  TCLTask::unlock();
}

void OpenGL::setvisual(const clString& wname, int which, int width, int height)
{
  tkwin=0;
  current_drawer=0;
  //cerr << "choosing visual " << which << '\n';
  TCL::execute(clString("opengl ")+wname+" -visual "+to_string((int)visuals[which]->visualid)+" -direct true -geometry "+to_string(width)+"x"+to_string(height));
  //cerr << clString("opengl ")+wname+" -visual "+to_string((int)visuals[which]->visualid)+" -direct true -geometry "+to_string(width)+"x"+to_string(height) << endl;
  //cerr << "done choosing visual\n";
}

void OpenGL::getData(int datamask, FutureValue<GeometryData*>* result)
{
    send_mb.send(DO_GETDATA);
    get_mb.send(GetReq(datamask, result));
}

void OpenGL::real_getData(int datamask, FutureValue<GeometryData*>* result)
{
    GeometryData* res = new GeometryData;
    if(datamask&GEOM_VIEW){
	res->view=new View(lastview);
	res->xres=xres;
	res->yres=yres;
	res->znear=znear;
	res->zfar=zfar;
    }
    if(datamask&(GEOM_COLORBUFFER|GEOM_DEPTHBUFFER)){
	TCLTask::lock();
    }
    if(datamask&GEOM_COLORBUFFER){
	ColorImage* img = res->colorbuffer = new ColorImage(xres, yres);
	float* data=new float[xres*yres*3];
	cerr << "xres=" << xres << ", yres=" << yres << endl;
	WallClockTimer timer;
	timer.start();
	glReadPixels(0, 0, xres, yres, GL_RGB, GL_FLOAT, data);
	timer.stop();
	cerr << "done in " << timer.time() << " seconds\n";
	float* p=data;
	for(int y=0;y<yres;y++){
	    for(int x=0;x<xres;x++){
		img->put_pixel(x, y, Color(p[0], p[1], p[2]));
		p+=3;
	    }
	}
	delete[] data;
    }
    if(datamask&GEOM_DEPTHBUFFER){
	DepthImage* img=res->depthbuffer=new DepthImage(xres, yres);
	unsigned int* data=new unsigned int[xres*yres*3];
	cerr << "reading depth...\n";
	WallClockTimer timer;
	timer.start();
	glReadPixels(0, 0, xres, yres, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, data);
	timer.stop();
	cerr << "done in " << timer.time() << " seconds\n";
	unsigned int* p=data;
	for(int y=0;y<yres;y++){
	    for(int x=0;x<xres;x++){
		img->put_pixel(x, y, (*p++)*(1./4294967295.));
	    }
	}
	delete[] data;
    }
    if(datamask&(GEOM_COLORBUFFER|GEOM_DEPTHBUFFER)){
	int errcode;
	while((errcode=glGetError()) != GL_NO_ERROR){
	    cerr << "We got an error from GL: " << (char*)gluErrorString(errcode) << endl;
	}
	TCLTask::unlock();
    }
    result->send(res);
}

GetReq::GetReq()
{
}

GetReq::GetReq(int datamask, FutureValue<GeometryData*>* result)
: datamask(datamask), result(result)
{
}


} // End namespace Modules
} // End namespace PSECommon

//
// $Log$
// Revision 1.9  1999/09/14 17:08:11  kuzimmer
// added glPixelStorei(GL_PACK_ALIGNMENT,1); to the dump_image function so that raw images were dumped correctly
//
// Revision 1.8  1999/09/08 22:04:32  sparker
// Fixed picking
// Added messages for pick mode
//
// Revision 1.7  1999/08/29 00:46:41  sparker
// Integrated new thread library
// using statement tweaks to compile with both MipsPRO and g++
// Thread library bug fixes
//
// Revision 1.6  1999/08/25 03:47:57  sparker
// Changed SCICore/CoreDatatypes to SCICore/Datatypes
// Changed PSECore/CommonDatatypes to PSECore/Datatypes
// Other Misc. directory tree updates
//
// Revision 1.5  1999/08/23 20:11:49  sparker
// GenAxes had no UI
// Removed extraneous print statements
// Miscellaneous compilation issues
// Fixed an authorship error
//
// Revision 1.4  1999/08/23 06:30:31  sparker
// Linux port
// Added X11 configuration options
// Removed many warnings
//
// Revision 1.3  1999/08/19 23:17:52  sparker
// Removed a bunch of #include <SCICore/Util/NotFinished.h> statements
// from files that did not need them.
//
// Revision 1.2  1999/08/17 06:37:37  sparker
// Merged in modifications from PSECore to make this the new "blessed"
// version of SCIRun/Uintah.
//
// Revision 1.1  1999/07/27 16:57:51  mcq
// Initial commit
//
// Revision 1.4  1999/07/07 21:10:27  dav
// added beginnings of support for g++ compilation
//
// Revision 1.3  1999/06/24 21:21:15  dav
// added Mpeg stuff back to Salmon
//
// Revision 1.2  1999/04/27 22:57:55  dav
// updates in Modules for Datatypes
//
// Revision 1.1.1.1  1999/04/24 23:12:30  dav
// Import sources
//
//

