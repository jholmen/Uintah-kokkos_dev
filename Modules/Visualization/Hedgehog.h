
/*
 *  Hedgehog.h: Visualization module
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   March 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#ifndef SCI_project_module_Hedgehog_h
#define SCI_project_module_Hedgehog_h

#include <Dataflow/Module.h>
#include <Datatypes/ScalarField.h>
#include <Datatypes/ScalarFieldPort.h>
#include <Datatypes/VectorField.h>
#include <Datatypes/VectorFieldPort.h>
#include <Geometry/Point.h>
class GeometryOPort;
class MaterialProp;


class Hedgehog : public Module {
    VectorFieldIPort* infield;
    GeometryOPort* ogeom;
    int abort_flag;

    Point min;
    Point max;
    double space_x;
    double space_y;
    double space_z;
    double length_scale;
    double radius;

    int need_minmax;

    int hedgehog_id;

    MaterialProp* front_matl;
    MaterialProp* back_matl;
    virtual void geom_moved(int, double, const Vector&, void*);
public:
    Hedgehog(const clString& id);
    Hedgehog(const Hedgehog&, int deep);
    virtual ~Hedgehog();
    virtual Module* clone(int deep);
    virtual void execute();
};

#endif
