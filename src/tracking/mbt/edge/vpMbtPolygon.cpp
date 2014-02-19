/****************************************************************************
 *
 * $Id: vpMbtPolygon.cpp 4632 2014-02-03 17:06:40Z fspindle $
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2014 by INRIA. All rights reserved.
 * 
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ("GPL") version 2 as published by the Free Software Foundation.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact INRIA about acquiring a ViSP Professional 
 * Edition License.
 *
 * See http://www.irisa.fr/lagadic/visp/visp.html for more information.
 * 
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://www.irisa.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * Description:
 * Make the complete tracking of an object by using its CAD model
 *
 * Authors:
 * Nicolas Melchior
 * Romain Tallonneau
 * Eric Marchand
 * Aurelien Yol
 *
 *****************************************************************************/

#include <limits.h>

#include <visp/vpConfig.h>
/*!
 \file vpMbtPolygon.cpp
 \brief Implements a polygon of the model used by the model-based tracker.
*/

#include <visp/vpMbtPolygon.h>

/*!
  Basic constructor.
*/
vpMbtPolygon::vpMbtPolygon()
  : index(-1), nbpt(0), nbCornersInsidePrev(0), isvisible(false), isappearing(false),
    p(NULL), roiPointsClip(), clippingFlag(vpMbtPolygon::NO_CLIPPING),
    distNearClip(0.001), distFarClip(100.)
{
}

vpMbtPolygon::vpMbtPolygon(const vpMbtPolygon& mbtp)
  : index(-1), nbpt(0), nbCornersInsidePrev(0), isvisible(false), isappearing(false),
    p(NULL), roiPointsClip(), clippingFlag(vpMbtPolygon::NO_CLIPPING),
    distNearClip(0.001), distFarClip(100.)
{
  *this = mbtp;
}

vpMbtPolygon& vpMbtPolygon::operator=(const vpMbtPolygon& mbtp)
{
  index = mbtp.index;
  nbpt = mbtp.nbpt;
  nbCornersInsidePrev = mbtp.nbCornersInsidePrev;
  isvisible = mbtp.isvisible;
  isappearing = mbtp.isappearing;
  roiPointsClip = mbtp.roiPointsClip;
  clippingFlag = mbtp.clippingFlag;
  distNearClip = mbtp.distFarClip;

  if (p) delete [] p;
  p = new vpPoint [nbpt];
  for(unsigned int i = 0; i < nbpt; i++)
    p[i] = mbtp.p[i];

  return (*this);
}

/*!
  Basic destructor.
*/
vpMbtPolygon::~vpMbtPolygon()
{
  if (p !=NULL)
  {
    delete[] p;
    p = NULL;
  }
}

/*!
  Get a reference to a corner.

  \throw vpException::dimensionError if the _index is out of range.

  \param _index : the index of the corner
*/
vpPoint &
vpMbtPolygon::getPoint(const unsigned int _index)
{
  if(_index >= nbpt){
    throw vpException(vpException::dimensionError, "index out of range");
  }
  return p[_index];
}

/*!
  Set the number of points which are the corners of the polygon.
  
  \param nb : The number of corners.
*/
void
vpMbtPolygon::setNbPoint(const unsigned int nb)
{
  nbpt = nb ;
  if (p != NULL)
    delete[] p;
  p = new vpPoint[nb] ;
}

/*!
  Add a corner point to the list of polygon's corners.
  
  \param n : The index of the corner.
  \param P : The point to add.
*/
void
vpMbtPolygon::addPoint(const unsigned int n, const vpPoint &P)
{
  //if( p!NULL && n < nbpt )
    p[n] = P ;
}

/*!
  Project the 3D corner points into the image thanks to the pose of the camera.
  
  \param cMo : The pose of the camera.
*/
void
vpMbtPolygon::changeFrame(const vpHomogeneousMatrix &cMo)
{
  for (unsigned int i = 0 ; i < nbpt ; i++)
  {
    p[i].changeFrame(cMo) ;
    p[i].projection() ;
  }
}

/*!
  Check if the polygon is visible in the image and if the angle between the normal 
  to the face and the line vector going from the optical center to the cog of the face is below
  the given threshold.
  To do that, the polygon is projected into the image thanks to the camera pose.
  
  \param cMo : The pose of the camera.
  \param alpha : Maximum angle to detect if the face is visible (in rad).
  \param modulo : Indicates if the test should also consider faces that are not oriented
  counter clockwise. If true, the orientation of the face is without importance.
  
  \return Return true if the polygon is visible.
*/
bool 
vpMbtPolygon::isVisible(const vpHomogeneousMatrix &cMo, const double alpha, const bool &modulo)
{
  //   std::cout << "Computing angle from MBT Face (cMo, alpha)" << std::endl;
  if(nbpt <= 2){
    /* a line is allways visible */
    isvisible = true;
    isappearing = false;
    return  true ;
  }

  changeFrame(cMo);

  vpColVector e1(3) ;
  vpColVector e2(3) ;
  vpColVector facenormal(3) ;

  e1[0] = p[1].get_X() - p[0].get_X() ;
  e1[1] = p[1].get_Y() - p[0].get_Y() ;
  e1[2] = p[1].get_Z() - p[0].get_Z() ;

  e2[0] = p[2].get_X() - p[1].get_X() ;
  e2[1] = p[2].get_Y() - p[1].get_Y() ;
  e2[2] = p[2].get_Z() - p[1].get_Z() ;
 
  e1.normalize();
  e2.normalize();
  
  facenormal = vpColVector::crossProd(e1,e2) ;
  facenormal.normalize();
 
  vpColVector e4(3) ;
  vpPoint pt;
  for (unsigned int i = 0; i < nbpt; i += 1){
    pt.set_X(pt.get_X() + p[i].get_X());
    pt.set_Y(pt.get_Y() + p[i].get_Y());
    pt.set_Z(pt.get_Z() + p[i].get_Z());
  }
  e4[0] = -pt.get_X()/(double)nbpt; e4[1] = -pt.get_Y()/(double)nbpt; e4[2] = -pt.get_Z()/(double)nbpt; 
  e4.normalize();
  
  double cos_angle = vpColVector::dotProd (e4, facenormal);
  double angle = acos(cos_angle);
  
//   std::cout << cos_angle << "/" << vpMath::deg(angle) << std::endl;
  
  if( angle < alpha ){
    isvisible = true;
    isappearing = false;
    return true;
  }

  if(modulo && (M_PI - angle) < alpha){
    isvisible = true;
    isappearing = false;
    return true;
  }

  if (angle < alpha+vpMath::rad(1) ){
    isappearing = true;
  }
  else if (modulo && (M_PI - angle) < alpha+vpMath::rad(1) ){
    isappearing = true;
  }
  else {
    isappearing = false;
  }
  isvisible = false ;
  return false ;
}

/*!
  Compute the region of interest in the image according to the used clipping.
  
  \param cam : camera parameters used to compute the field of view.
*/
void
vpMbtPolygon::computeRoiClipped(const vpCameraParameters &cam)
{  
  roiPointsClip = std::vector<std::pair<vpPoint,unsigned int> >();
  std::vector<vpColVector> fovNormals;
  
  if(cam.isFovComputed() && clippingFlag > 3)
    fovNormals = cam.getFovNormals();
  
  for (unsigned int i = 0; i < nbpt; i ++){
    vpPoint p1Clipped, p2Clipped;
    unsigned int p1ClippedInfo, p2ClippedInfo;
    vpImagePoint ip;
    if(vpMbtPolygon::getClippedPoints(p[i], p[(i+1)%nbpt], p1Clipped, p2Clipped, p1ClippedInfo, p2ClippedInfo, cam, fovNormals)){
      p1Clipped.projection();
      roiPointsClip.push_back(std::make_pair(p1Clipped, p1ClippedInfo));
      
      if(p2ClippedInfo != vpMbtPolygon::NO_CLIPPING){
        p2Clipped.projection();
        roiPointsClip.push_back(std::make_pair(p2Clipped, p2ClippedInfo));
      }
      
      if(nbpt == 2){ //Case of a line.
        if(p2ClippedInfo == vpMbtPolygon::NO_CLIPPING){
          p2Clipped.projection();
          roiPointsClip.push_back(std::make_pair(p2Clipped, p2ClippedInfo));
        }
        break;
      }
    }
  }
}    
    
/*!
  Get the clipped points according to a plane equation.

  \param cam : camera parameters
  \param p1 : First extremity of the line.
  \param p2 : Second extremity of the line.
  \param p1Clipped : Resulting p1.
  \param p2Clipped : Resulting p2.
  \param p1ClippedInfo : Resulting clipping flag for p1.
  \param p2ClippedInfo : Resulting clipping flag for p2.
  \param A : Param A from plane equation.
  \param B : Param B from plane equation.
  \param C : Param C from plane equation.
  \param D : Param D from plane equation.
  \param flag : flag specifying the clipping used when calling this function.
  
  \return True if the points have been clipped, False otherwise
*/
bool
vpMbtPolygon::getClippedPointsFov(const vpPoint &p1, const vpPoint &p2, 
                                vpPoint &p1Clipped, vpPoint &p2Clipped, 
                                unsigned int &p1ClippedInfo, unsigned int &p2ClippedInfo,
                                const vpColVector &normal, const unsigned int &flag)
{    
  vpRowVector p1Vec(3);
  p1Vec[0] = p1.get_X(); p1Vec[1] = p1.get_Y(); p1Vec[2] = p1.get_Z();
  p1Vec = p1Vec.normalize();
  
  vpRowVector p2Vec(3);
  p2Vec[0] = p2.get_X(); p2Vec[1] = p2.get_Y(); p2Vec[2] = p2.get_Z();
  p2Vec = p2Vec.normalize();
  
  if((clippingFlag & flag) == flag){
    double beta1 = acos( p1Vec * normal );
    double beta2 = acos( p2Vec * normal );
    
    if(beta1 < M_PI / 2.0 && beta2 < M_PI / 2.0)
      return false;
    else if (beta1 < M_PI / 2.0 || beta2 < M_PI / 2.0){
      vpPoint pClipped;
      double t = -(normal[0] * p1.get_X() + normal[1] * p1.get_Y() + normal[2] * p1.get_Z());
      t = t / ( normal[0] * (p2.get_X() - p1.get_X()) + normal[1] * (p2.get_Y() - p1.get_Y()) + normal[2] * (p2.get_Z() - p1.get_Z()) );
      
      pClipped.set_X((p2.get_X() - p1.get_X())*t + p1.get_X());
      pClipped.set_Y((p2.get_Y() - p1.get_Y())*t + p1.get_Y());
      pClipped.set_Z((p2.get_Z() - p1.get_Z())*t + p1.get_Z());
      
      if(beta1 < M_PI / 2.0){
        p1ClippedInfo = p1ClippedInfo | flag;
        p1Clipped = pClipped;
      }
      else{
        p2ClippedInfo = p2ClippedInfo | flag;
        p2Clipped = pClipped;
      }
    }
  }
  
  return true;
}

/*!
  Get the clipped points.

  \param p1 : First extremity of the line.
  \param p2 : Second extremity of the line.
  \param p1Clipped : Resulting p1.
  \param p2Clipped : Resulting p2.
  \param p1ClippedInfo : Resulting clipping flag for p1.
  \param p2ClippedInfo : Resulting clipping flag for p2.
  \param cam : camera parameters.
  
  \return True if the points have been clipped, False otherwise
*/
bool           
vpMbtPolygon::getClippedPoints(const vpPoint &p1, const vpPoint &p2, 
                               vpPoint &p1Clipped, vpPoint &p2Clipped, 
                               unsigned int &p1ClippedInfo, unsigned int &p2ClippedInfo,
                               const vpCameraParameters &cam, const std::vector<vpColVector> &fovNormals)
{
  p1Clipped = p1;
  p2Clipped = p2;
  p1ClippedInfo = vpMbtPolygon::NO_CLIPPING;
  p2ClippedInfo = vpMbtPolygon::NO_CLIPPING;
  
  if(clippingFlag != vpMbtPolygon::NO_CLIPPING){
    // Near Clipping
    if( (clippingFlag & vpMbtPolygon::NEAR_CLIPPING) == vpMbtPolygon::NEAR_CLIPPING || 
        ((clippingFlag & vpMbtPolygon::LEFT_CLIPPING) == vpMbtPolygon::LEFT_CLIPPING && (clippingFlag & vpMbtPolygon::RIGHT_CLIPPING) == vpMbtPolygon::RIGHT_CLIPPING) || 
        ((clippingFlag & vpMbtPolygon::UP_CLIPPING) == vpMbtPolygon::UP_CLIPPING && (clippingFlag & vpMbtPolygon::DOWN_CLIPPING) == vpMbtPolygon::DOWN_CLIPPING)){
      if(p1Clipped.get_Z() < distNearClip && p2Clipped.get_Z() < distNearClip) //Not using getClippedPointsFov for efficiency
        return false;
      else if(p1Clipped.get_Z() < distNearClip || p2Clipped.get_Z() < distNearClip){
        vpPoint pClippedNear;
        double t = (p2Clipped.get_Z() - p1Clipped.get_Z());
        t = (distNearClip - p1Clipped.get_Z()) / t;
        
        pClippedNear.set_X((p2Clipped.get_X() - p1Clipped.get_X())*t + p1Clipped.get_X());
        pClippedNear.set_Y((p2Clipped.get_Y() - p1Clipped.get_Y())*t + p1Clipped.get_Y());
        pClippedNear.set_Z(distNearClip);
        
        if(p1Clipped.get_Z() < distNearClip){
          p1Clipped = pClippedNear;
          p1ClippedInfo = p1ClippedInfo | vpMbtPolygon::NEAR_CLIPPING;
        }
        else{
          p2Clipped = pClippedNear;
          p2ClippedInfo = p2ClippedInfo | vpMbtPolygon::NEAR_CLIPPING;
        }
      }
    }
    
    // Left Clipping
    if((clippingFlag & vpMbtPolygon::LEFT_CLIPPING) == vpMbtPolygon::LEFT_CLIPPING){
      if(!cam.isFovComputed())
        vpTRACE("Field of view not computed, left clipping skipped.");
      else if(!getClippedPointsFov(p1Clipped, p2Clipped, p1Clipped, p2Clipped, p1ClippedInfo, p2ClippedInfo, 
             fovNormals[0], vpMbtPolygon::LEFT_CLIPPING))
        return false;
    }
    
    // Right Clipping
    if((clippingFlag & vpMbtPolygon::RIGHT_CLIPPING) == vpMbtPolygon::RIGHT_CLIPPING ){
      if(!cam.isFovComputed())
        vpTRACE("Field of view not computed, right clipping skipped.");
      else if(!getClippedPointsFov(p1Clipped, p2Clipped, p1Clipped, p2Clipped, p1ClippedInfo, p2ClippedInfo, 
             fovNormals[1], vpMbtPolygon::RIGHT_CLIPPING))
        return false;
    }
      
    // Up Clipping
    if((clippingFlag & vpMbtPolygon::UP_CLIPPING) == vpMbtPolygon::UP_CLIPPING){
      if(!cam.isFovComputed())
        vpTRACE("Field of view not computed, up clipping skipped.");
      else if(!getClippedPointsFov(p1Clipped, p2Clipped, p1Clipped, p2Clipped, p1ClippedInfo, p2ClippedInfo, 
              fovNormals[2], vpMbtPolygon::UP_CLIPPING))
        return false;
    }
      
    // Down Clipping
    if((clippingFlag & vpMbtPolygon::DOWN_CLIPPING) == vpMbtPolygon::DOWN_CLIPPING ){
      if(!cam.isFovComputed())
        vpTRACE("Field of view not computed, down clipping skipped.");
      else if(!getClippedPointsFov(p1Clipped, p2Clipped, p1Clipped, p2Clipped, p1ClippedInfo, p2ClippedInfo, 
              fovNormals[3], vpMbtPolygon::DOWN_CLIPPING))
        return false;
    }
      
    // Far Clipping
    if( (clippingFlag & vpMbtPolygon::FAR_CLIPPING) == vpMbtPolygon::FAR_CLIPPING ){
      if(p1Clipped.get_Z() > distFarClip && p2Clipped.get_Z() > distFarClip) //Not using getClippedPointsFov for efficiency
        return false;
      else if(p1Clipped.get_Z() > distFarClip || p2Clipped.get_Z() > distFarClip){
        vpPoint pClippedFar;
        double t = (p2Clipped.get_Z() - p1Clipped.get_Z());
        t = (distFarClip - p1Clipped.get_Z()) / t;
        
        pClippedFar.set_X((p2Clipped.get_X() - p1Clipped.get_X())*t + p1Clipped.get_X());
        pClippedFar.set_Y((p2Clipped.get_Y() - p1Clipped.get_Y())*t + p1Clipped.get_Y());
        pClippedFar.set_Z(distFarClip); 
        
        if(p1Clipped.get_Z() > distFarClip){
          p1Clipped = pClippedFar;
          p1ClippedInfo = p1ClippedInfo | vpMbtPolygon::FAR_CLIPPING;
        }
        else{
          p2Clipped = pClippedFar;
          p2ClippedInfo = p2ClippedInfo | vpMbtPolygon::FAR_CLIPPING;
        }
      }
    }
  }
  
  return true;
}

/*!
  Get the region of interest in the image.
  
  \warning Suppose that changeFrame() has already been called.
  
  \param cam : camera parameters.

  \return Image point corresponding to the region of interest.
*/
std::vector<vpImagePoint>
vpMbtPolygon::getRoi(const vpCameraParameters &cam)
{     
  std::vector<vpImagePoint> roi;
  for (unsigned int i = 0; i < nbpt; i ++){
    vpImagePoint ip;
    vpMeterPixelConversion::convertPoint(cam, p[i].get_x(), p[i].get_y(), ip);
    roi.push_back(ip);
  }
  
  return roi;
}

/*!
  Get the region of interest in the image.
  
  \param cam : camera parameters.
  \param cMo : pose.

  \return Image point corresponding to the region of interest.
*/
std::vector<vpImagePoint> 
vpMbtPolygon::getRoi(const vpCameraParameters &cam, const vpHomogeneousMatrix &cMo)
{
  changeFrame(cMo);
  return getRoi(cam);
}

/*!
  Get the region of interest clipped in the image.
  
  \warning Suppose that changeFrame() and computeRoiClipped() have already been called.
  
  \param cam : camera parameters.
  \param roi : image point corresponding to the region of interest.
*/
void
vpMbtPolygon::getRoiClipped(const vpCameraParameters &cam, std::vector<vpImagePoint> &roi)
{
  for(unsigned int i = 0 ; i < roiPointsClip.size() ; i++){
    vpImagePoint ip;
    vpMeterPixelConversion::convertPoint(cam,roiPointsClip[i].first.get_x(),roiPointsClip[i].first.get_y(),ip);
    roi.push_back(ip);
  }
}

/*!
  Get the region of interest clipped in the image.
  
  \param cam : camera parameters.
  \param cMo : pose.
  \param roi : image point corresponding to the region of interest.
*/
void
vpMbtPolygon::getRoiClipped(const vpCameraParameters &cam, std::vector<vpImagePoint> &roi, const vpHomogeneousMatrix &cMo)
{
  changeFrame(cMo);
  computeRoiClipped(cam);
  getRoiClipped(cam, roi);
}
  
/*!
  Get the region of interest clipped in the image and the information to know if it's a clipped point.
  
  \warning Suppose that changeFrame() and computeRoiClipped() have already been called.
  
  \param cam : camera parameters.
  \param roi : image point corresponding to the region of interest with clipping information.
*/
void
vpMbtPolygon::getRoiClipped(const vpCameraParameters &cam, std::vector<std::pair<vpImagePoint,unsigned int> > &roi)
{
  for(unsigned int i = 0 ; i < roiPointsClip.size() ; i++){
    vpImagePoint ip;
    vpMeterPixelConversion::convertPoint(cam,roiPointsClip[i].first.get_x(),roiPointsClip[i].first.get_y(),ip);
    roi.push_back(std::make_pair(ip, roiPointsClip[i].second));
  }
}

/*!
  Get the region of interest clipped in the image and the information to know if it's a clipped point.
  
  \param cam : camera parameters.
  \param roi : image point corresponding to the region of interest with clipping information.
  \param cMo : pose.
*/
void
vpMbtPolygon::getRoiClipped(const vpCameraParameters &cam, std::vector<std::pair<vpImagePoint,unsigned int> > &roi, const vpHomogeneousMatrix &cMo)
{
  changeFrame(cMo);
  computeRoiClipped(cam);
  getRoiClipped(cam, roi);
}



/*!
  Static method to check the number of points of a region defined by the vector of image point that are inside the image.

  \param I : The image used for its size.
  \param cam : The camera parameters.
*/
unsigned int 
vpMbtPolygon::getNbCornerInsideImage(const vpImage<unsigned char>& I, const vpCameraParameters &cam)
{
  unsigned int nbPolyIn = 0;
  for (unsigned int i = 0; i < nbpt; i ++){
    if(p[i].get_Z() > 0){
      vpImagePoint ip;
      vpMeterPixelConversion::convertPoint(cam, p[i].get_x(), p[i].get_y(), ip);
      if((ip.get_i() >= 0) && (ip.get_j() >= 0) && (ip.get_i() < I.getHeight()) && (ip.get_j() < I.getWidth()))
        nbPolyIn++;
    }
  }
  
  nbCornersInsidePrev = nbPolyIn;
  
  return nbPolyIn;
}

//###################################
//      Static functions
//###################################

void                
vpMbtPolygon::getMinMaxRoi(const std::vector<vpImagePoint> &iroi, int & i_min, int &i_max, int &j_min, int &j_max)
{
  // i_min_d = std::numeric_limits<double>::max(); // create an error under Windows. To fix it we have to add #undef max
  double i_min_d = (double) INT_MAX;
  double i_max_d = 0;
  double j_min_d = (double) INT_MAX;
  double j_max_d = 0;

  for (unsigned int i = 0; i < iroi.size(); i += 1){
    if(i_min_d > iroi[i].get_i())
      i_min_d = iroi[i].get_i();
    
    if(iroi[i].get_i() < 0)
      i_min_d = 1;
    
    if((iroi[i].get_i() > 0) && (i_max_d < iroi[i].get_i()))
      i_max_d = iroi[i].get_i();
    
    if(j_min_d > iroi[i].get_j())
      j_min_d = iroi[i].get_j();
    
    if(iroi[i].get_j() < 0)
      j_min_d = 1;//border
      
    if((iroi[i].get_j() > 0) && j_max_d < iroi[i].get_j())
      j_max_d = iroi[i].get_j();
  }
  i_min = static_cast<int> (i_min_d);
  i_max = static_cast<int> (i_max_d);
  j_min = static_cast<int> (j_min_d);
  j_max = static_cast<int> (j_max_d);
}

/*!
  Static method to check whether the region defined by the vector of image point
  is contained entirely in the image.

  \param I : The image used for its size.
  \param corners : The vector of points defining a region
*/
bool
vpMbtPolygon::roiInsideImage(const vpImage<unsigned char>& I, const std::vector<vpImagePoint>& corners)
{
  double nbPolyIn = 0;
  for(unsigned int i=0; i<corners.size(); ++i){
    if((corners[i].get_i() >= 0) && (corners[i].get_j() >= 0) &&
       (corners[i].get_i() < I.getHeight()) && (corners[i].get_j() < I.getWidth())){
      nbPolyIn++;
    }
  }
  
  if(nbPolyIn < 3 && nbPolyIn < 0.7 * corners.size())
    return false;
  
  return true;
}

#ifdef VISP_BUILD_DEPRECATED_FUNCTIONS
/*!
  \deprecated This method is deprecated since it is no more used since ViSP 2.7.2. \n
  
  Check if the polygon is visible in the image. To do that, the polygon is projected into the image thanks to the camera pose.
  
  \param cMo : The pose of the camera.
  \param depthTest : True if a face has to be entirely visible (in front of the camera). False if it can be partially visible.
  
  \return Return true if the polygon is visible.
*/
bool
vpMbtPolygon::isVisible(const vpHomogeneousMatrix &cMo, const bool &depthTest)
{
  changeFrame(cMo) ;
  
  if(depthTest)
    for (unsigned int i = 0 ; i < nbpt ; i++){
      if(p[i].get_Z() < 0){
        isappearing = false;
        isvisible = false ;
        return false ;
      }
    }
  
  return isVisible(cMo, vpMath::rad(89));
}
#endif


