
/*
 *  ScaledFrameWidget.h
 *
 *  Written by:
 *   James Purciful
 *   Department of Computer Science
 *   University of Utah
 *   Jan. 1995
 *
 *  Copyright (C) 1995 SCI Group
 */


#ifndef SCI_project_ScaledFrame_Widget_h
#define SCI_project_ScaledFrame_Widget_h 1

#include <Widgets/BaseWidget.h>


class ScaledFrameWidget : public BaseWidget {
public:
   ScaledFrameWidget( Module* module, CrowdMonitor* lock, double widget_scale );
   ScaledFrameWidget( const ScaledFrameWidget& );
   virtual ~ScaledFrameWidget();

   virtual void widget_execute();
   virtual void geom_moved(int, double, const Vector&, int, const BState&);

   virtual void MoveDelta( const Vector& delta );
   virtual Point ReferencePoint() const;

   void SetPosition( const Point& center, const Point& R, const Point& D );
   void GetPosition( Point& center, Point& R, Point& D );

   void SetPosition( const Point& center, const Vector& normal,
		     const Real size1, const Real size2 );
   void GetPosition( Point& center, Vector& normal,
		     Real& size1, Real& size2 );

   void SetRatioR( const Real ratio );
   Real GetRatioR() const;
   void SetRatioD( const Real ratio );
   Real GetRatioD() const;

   void SetSize( const Real sizeR, const Real sizeD );
   void GetSize( Real& sizeR, Real& sizeD ) const;
   
   const Vector& GetRightAxis();
   const Vector& GetDownAxis();

   // Variable indexs
   enum { CenterVar, PointRVar, PointDVar, DistRVar, DistDVar, HypoVar,
	  SDistRVar, RatioRVar, SDistDVar, RatioDVar };

   // Material indexs
   enum { PointMatl, EdgeMatl, ResizeMatl, SliderMatl };

protected:
   virtual clString GetMaterialName( const Index mindex ) const;   
   
private:
   Vector oldrightaxis, olddownaxis;
};


#endif
