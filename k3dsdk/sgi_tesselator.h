#ifndef K3DSDK_SGI_TESSELATOR_H
#define K3DSDK_SGI_TESSELATOR_H


/** \file
		\brief Declares the SGI tesselator
*/

extern "C"
{

typedef void (*callback_t)();

struct SGItesselator;
	
extern SGItesselator* sgiNewTess (void);
extern void sgiDeleteTess (SGItesselator* tess);
extern void sgiGetTessProperty (SGItesselator* tess, GLenum which, GLdouble* data);
extern void sgiTessBeginContour (SGItesselator* tess);
extern void sgiTessBeginPolygon (SGItesselator* tess, GLvoid* data);
extern void sgiTessCallback (SGItesselator* tess, GLenum which, callback_t CallBackFunc);
extern void sgiTessEndContour (SGItesselator* tess);
extern void sgiTessEndPolygon (SGItesselator* tess);
extern void sgiTessNormal (SGItesselator* tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ);
extern void sgiTessProperty (SGItesselator* tess, GLenum which, GLdouble data);
extern void sgiTessVertex (SGItesselator* tess, GLdouble *location, GLvoid* data);

}

#endif // !K3DSDK_SGI_TESSELATOR_H

