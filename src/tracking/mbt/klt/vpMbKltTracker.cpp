/****************************************************************************
 *
 * $Id: vpMbKltTracker.cpp 4661 2014-02-10 19:34:58Z fspindle $
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
 * Description:
 * Model based tracker using only KLT
 *
 * Authors:
 * Romain Tallonneau
 * Aurelien Yol
 *
 *****************************************************************************/

#include <visp/vpMbKltTracker.h>

#ifdef VISP_HAVE_OPENCV

vpMbKltTracker::vpMbKltTracker()
  : cur(NULL), c0Mo(), angleAppears(0), angleDisappears(0), compute_interaction(true),
    firstInitialisation(true), maskBorder(5), lambda(0.8), maxIter(200), threshold_outlier(0.5),
    percentGood(0.6), useOgre(false), ctTc0(), tracker(), faces(), firstTrack(false),
    distNearClip(0.01), distFarClip(100), clippingFlag(vpMbtPolygon::NO_CLIPPING)
{  
  tracker.setTrackerId(1);
  tracker.setUseHarris(1);
  
  tracker.setMaxFeatures(10000);
  tracker.setWindowSize(5);
  tracker.setQuality(0.01);
  tracker.setMinDistance(5);
  tracker.setHarrisFreeParameter(0.01);
  tracker.setBlockSize(3);
  tracker.setPyramidLevels(3);
  
  angleAppears = vpMath::rad(65);
  angleDisappears = vpMath::rad(75);
    
#ifdef VISP_HAVE_OGRE
  faces.getOgreContext()->setWindowName("MBT KLT");
#endif
}

/*!
  Basic destructor.

*/
vpMbKltTracker::~vpMbKltTracker()
{
  if(cur != NULL){
    cvReleaseImage(&cur);
    cur = NULL;
  }
}

void 
vpMbKltTracker::init(const vpImage<unsigned char>& I)
{
  if(!modelInitialised){
    throw vpException(vpException::fatalError, "model not initialized");
  }
  
 bool reInitialisation = false;
  if(!useOgre)
    faces.setVisible(I, cam, cMo, angleAppears, angleDisappears, reInitialisation);
  else{
#ifdef VISP_HAVE_OGRE   
    if(!faces.isOgreInitialised()){
      faces.setBackgroundSizeOgre(I.getHeight(), I.getWidth());
      faces.initOgre(cam);
    }
    
    faces.setVisibleOgre(I, cam, cMo, angleAppears, angleDisappears, reInitialisation);
    
#else
    faces.setVisible(I, cam, cMo, angleAppears, angleDisappears, reInitialisation);
#endif
  }
  
  reinit(I);
}

void 
vpMbKltTracker::reinit(const vpImage<unsigned char>& I)
{  
  c0Mo = cMo;
  ctTc0.setIdentity();
  firstTrack = false;

  vpImageConvert::convert(I, cur);
  
  // mask
  IplImage* mask = cvCreateImage(cvSize((int)I.getWidth(), (int)I.getHeight()), IPL_DEPTH_8U, 1);
  cvZero(mask);
  
  unsigned char val = 255/* - i*15*/;
  for (unsigned int i = 0; i < faces.size(); i += 1){
    if(faces[i]->isVisible())
      faces[i]->updateMask(mask, val, maskBorder);
  }
  
  tracker.initTracking(cur, mask);
  
  for (unsigned int i = 0; i < faces.size(); i += 1){
    if(faces[i]->isVisible()){
      faces[i]->init(tracker);
    }
  }

  cvReleaseImage(&mask);
}

/*!
  Reset the tracker. The model is removed and the pose is set to identity.
  The tracker needs to be initialized with a new model and a new pose. 
*/
void            
vpMbKltTracker::resetTracker()
{
  cMo.setIdentity();
  
  if(cur != NULL){
    cvReleaseImage(&cur);
    cur = NULL;
  }
  
  compute_interaction = true;
  firstInitialisation = true;
  computeCovariance = false;
  firstTrack = false;

  tracker.setTrackerId(1);
  tracker.setUseHarris(1);
  
  tracker.setMaxFeatures(10000);
  tracker.setWindowSize(5);
  tracker.setQuality(0.01);
  tracker.setMinDistance(5);
  tracker.setHarrisFreeParameter(0.01);
  tracker.setBlockSize(3);
  tracker.setPyramidLevels(3);
  
  angleAppears = vpMath::rad(65);
  angleDisappears = vpMath::rad(75);
  
  clippingFlag = vpMbtPolygon::NO_CLIPPING;
  
  maskBorder = 5;
  threshold_outlier = 0.5;
  percentGood = 0.7;
  
  lambda = 0.8;
  maxIter = 200;
  
  faces.reset();
  
#ifdef VISP_HAVE_OGRE
  useOgre = false;
#endif
}

/*!
  Get the current list of KLT points.
  
  \warning Contrary to getKltPoints which returns a pointer on CvPoint2D32f. This function convert and copy the openCV KLT points into vpImagePoints.
  
  \return the list of KLT points through vpKltOpencv.
*/
std::vector<vpImagePoint> 
vpMbKltTracker::getKltImagePoints() const
{
  std::vector<vpImagePoint> kltPoints;
  for (unsigned int i = 0; i < static_cast<unsigned int>(tracker.getNbFeatures()); i ++){
    int id;
    float x_tmp, y_tmp;
    tracker.getFeature((int)i, id, x_tmp, y_tmp);
    kltPoints.push_back(vpImagePoint(y_tmp, x_tmp));
  }
  
  return kltPoints;
}

/*!
  Get the current list of KLT points and their id.
  
  \warning Contrary to getKltPoints which returns a pointer on CvPoint2D32f. This function convert and copy the openCV KLT points into vpImagePoints.
  
  \return the list of KLT points and their id through vpKltOpencv.
*/
std::map<int, vpImagePoint> 
vpMbKltTracker::getKltImagePointsWithId() const
{
  std::map<int, vpImagePoint> kltPoints;
  for (unsigned int i = 0; i < static_cast<unsigned int>(tracker.getNbFeatures()); i ++){
    int id;
    float x_tmp, y_tmp;
    tracker.getFeature((int)i, id, x_tmp, y_tmp);
    kltPoints[id] = vpImagePoint(y_tmp, x_tmp);
  }
  
  return kltPoints;
}

/*!
  Set the new value of the klt tracker.

  \param t : Klt tracker containing the new values.
*/
void            
vpMbKltTracker::setKltOpencv(const vpKltOpencv& t){
  tracker.setMaxFeatures(t.getMaxFeatures());
  tracker.setWindowSize(t.getWindowSize());
  tracker.setQuality(t.getQuality());
  tracker.setMinDistance(t.getMinDistance());
  tracker.setHarrisFreeParameter(t.getHarrisFreeParameter());
  tracker.setBlockSize(t.getBlockSize());
  tracker.setPyramidLevels(t.getPyramidLevels());
}

/*!
  Set the camera parameters.

  \param camera : the new camera parameters.
*/
void
vpMbKltTracker::setCameraParameters(const vpCameraParameters& camera)
{
  for (unsigned int i = 0; i < faces.size(); i += 1){
    faces[i]->setCameraParameters(camera);
  }
  this->cam = camera;
}
   
/*!
  Use Ogre3D for visibility tests
  
  \warning This function has to be called before the initialization of the tracker.
  
  \param v : True to use it, False otherwise
*/
void    
vpMbKltTracker::setOgreVisibilityTest(const bool &v) 
{
  useOgre = v; 
  if(useOgre){
#ifndef VISP_HAVE_OGRE     
    useOgre = false;
    std::cout << "WARNING: ViSP doesn't have Ogre3D, basic visibility test will be used. setOgreVisibilityTest() set to false." << std::endl;
#endif
  }
}

/*!
  Set the pose to be used in entry of the next call to the track() function.
  This pose will be just used once.
  
  \warning This function has to be called after the initialization of the tracker.
  
  \param I : image corresponding to the desired pose.
  \param cdMo : Pose to affect.
*/
void           
vpMbKltTracker::setPose(const vpImage<unsigned char> &I, const vpHomogeneousMatrix& cdMo)
{
  if(firstTrack){
    bool reInitialisation = false;
    if(!useOgre)
      faces.setVisible(I, cam, cdMo, angleAppears, angleDisappears, reInitialisation);
    else{
  #ifdef VISP_HAVE_OGRE    
      faces.setVisibleOgre(I, cam, cdMo, angleAppears, angleDisappears, reInitialisation);
  #else
      faces.setVisible(I, cam, cdMo, angleAppears, angleDisappears, reInitialisation);
  #endif
    }
    
    if(reInitialisation){
      std::cout << "WARNING: Visibility changed, must reinitialize to update pose" << std::endl;
      cMo = cdMo;
      reinit(I);
    }
    else{
      vpHomogeneousMatrix cdMc = cdMo * cMo.inverse();
      vpHomogeneousMatrix cMcd = cdMc.inverse();
      
      vpRotationMatrix cdRc;
      vpTranslationVector cdtc;
      
      cdMc.extract(cdRc);
      cdMc.extract(cdtc);
      
      CvPoint2D32f* initial_guess = NULL;
      initial_guess = (CvPoint2D32f*)cvAlloc((unsigned int)tracker.getMaxFeatures()*sizeof(initial_guess[0]));
        
      for (unsigned int i = 0; i < faces.size(); i += 1){
        if(faces[i]->isVisible() && faces[i]->hasEnoughPoints()){  
          //Get the normal to the face at the current state cMo
          vpPlane plan(faces[i]->p[0], faces[i]->p[1], faces[i]->p[2]);
          plan.changeFrame(cMcd);
          
          vpColVector Nc = plan.getNormal(); 
          Nc.normalize();
          
          double invDc = 1.0 / plan.getD();
          
          //Create the homography
          vpMatrix cdHc;
          vpGEMM(cdtc, Nc, -invDc, cdRc, 1.0, cdHc, VP_GEMM_B_T);
          cdHc /= cdHc[2][2];
          
          //Create the 2D homography
          vpMatrix cdGc = cam.get_K() * cdHc * cam.get_K_inverse();
          
          //Points displacement
          std::map<int, vpImagePoint>::const_iterator iter = faces[i]->getCurrentPoints().begin();
          for( ; iter != faces[i]->getCurrentPoints().end(); iter++){
            vpColVector cdp(3);
            cdp[0] = iter->second.get_j(); cdp[1] = iter->second.get_i(); cdp[2] = 1.0;
            
            double p_mu_t_2 = cdp[0] * cdGc[2][0] + cdp[1] * cdGc[2][1] + cdGc[2][2];

            if( fabs(p_mu_t_2) < std::numeric_limits<double>::epsilon()){
              cdp[0] = 0.0;
              cdp[1] = 0.0;
              throw vpException(vpException::divideByZeroError, "the depth of the point is calculated to zero");
            }

            cdp[0] = (cdp[0] * cdGc[0][0] + cdp[1] * cdGc[0][1] + cdGc[0][2]) / p_mu_t_2;
            cdp[1] = (cdp[0] * cdGc[1][0] + cdp[1] * cdGc[1][1] + cdGc[1][2]) / p_mu_t_2;
            
            //Set value to the KLT tracker
            initial_guess[(faces[i]->getCurrentPointsInd())[iter->first]].x = (float)cdp[0];
            initial_guess[(faces[i]->getCurrentPointsInd())[iter->first]].y = (float)cdp[1];
          }
        }
      }  
      
      tracker.setInitialGuess(&initial_guess);
      
      if(initial_guess) cvFree(&initial_guess);
      initial_guess = NULL;
      
      cMo = cdMo;
    }
  }
}

/*!
  Set the far distance for clipping.
  
  \param dist : Far clipping value.
*/
void            
vpMbKltTracker::setFarClippingDistance(const double &dist) 
{ 
  if( (clippingFlag & vpMbtPolygon::NEAR_CLIPPING) == vpMbtPolygon::NEAR_CLIPPING && dist <= distNearClip)
    vpTRACE("Far clipping value cannot be inferior than near clipping value. Far clipping won't be considered.");
  else if ( dist < 0 ) 
    vpTRACE("Far clipping value cannot be inferior than 0. Far clipping won't be considered.");
  else{   
    clippingFlag = (clippingFlag | vpMbtPolygon::FAR_CLIPPING);
    distFarClip = dist; 
    for (unsigned int i = 0; i < faces.size(); i ++){
      faces[i]->setFarClippingDistance(distFarClip);
    }  
  }
}

/*!
  Set the near distance for clipping.
  
  \param dist : Near clipping value.
*/
void           
vpMbKltTracker::setNearClippingDistance(const double &dist) 
{ 
  if( (clippingFlag & vpMbtPolygon::FAR_CLIPPING) == vpMbtPolygon::FAR_CLIPPING && dist >= distFarClip)
    vpTRACE("Near clipping value cannot be superior than far clipping value. Near clipping won't be considered.");
  else if ( dist < 0 ) 
    vpTRACE("Near clipping value cannot be inferior than 0. Near clipping won't be considered.");
  else{
    clippingFlag = (clippingFlag | vpMbtPolygon::NEAR_CLIPPING);
    distNearClip = dist; 
    for (unsigned int i = 0; i < faces.size(); i ++){
      faces[i]->setNearClippingDistance(distNearClip);
    } 
  }
}

/*!
  Specify which clipping to use.
  
  \sa vpMbtPolygonClipping
  
  \param flags : New clipping flags.
*/
void            
vpMbKltTracker::setClipping(const unsigned int &flags) 
{ 
  clippingFlag = flags;
  for (unsigned int i = 0; i < faces.size(); i ++)
    faces[i]->setClipping(clippingFlag);
}
          
/*!
  Initialise a new face from the coordinates given in parameter.

  \param corners : Coordinates of the corners of the face in the object frame.
  \param indexFace : index of the face (depends on the vrml file organization).
*/
void
vpMbKltTracker::initFaceFromCorners(const std::vector<vpPoint>& corners, const unsigned int indexFace)
{     
  if( corners.size() > 2){ // This tracker can't handle lignes
    vpMbtKltPolygon *polygon = new vpMbtKltPolygon;
  //   polygon->setCameraParameters(cam);
    polygon->setNbPoint((unsigned int)corners.size());
    polygon->setIndex((int)indexFace);
    for(unsigned int j = 0; j < corners.size(); j++) {
      polygon->addPoint(j, corners[j]);
    }
    faces.addPolygon(polygon);
    faces.getPolygon().back()->setCameraParameters(cam);
    
    if(clippingFlag != vpMbtPolygon::NO_CLIPPING)
      faces.getPolygon().back()->setClipping(clippingFlag);
    
    if((clippingFlag & vpMbtPolygon::NEAR_CLIPPING) == vpMbtPolygon::NEAR_CLIPPING)
      faces.getPolygon().back()->setNearClippingDistance(distNearClip);
    
    if((clippingFlag & vpMbtPolygon::FAR_CLIPPING) == vpMbtPolygon::FAR_CLIPPING)
      faces.getPolygon().back()->setFarClippingDistance(distFarClip);

    delete polygon;
    polygon = NULL;
  }
}

/*!
  Realize the pre tracking operations

  \param I : The input image.
  \param nbInfos : Size of the features.
  \param nbFaceUsed : Number of face used for the tracking.
*/
void
vpMbKltTracker::preTracking(const vpImage<unsigned char>& I, unsigned int &nbInfos, unsigned int &nbFaceUsed)
{
  vpImageConvert::convert(I, cur);
  tracker.track(cur);
  
  if(!firstTrack)
    firstTrack = true;
  
  nbInfos = 0;  
  nbFaceUsed = 0;
  for (unsigned int i = 0; i < faces.size(); i += 1){
    if(faces[i]->isVisible()){
      faces[i]->computeNbDetectedCurrent(tracker);
//       faces[i]->ransac();
      if(faces[i]->hasEnoughPoints()){
        nbInfos += faces[i]->getNbPointsCur();
        nbFaceUsed++;
      }
    }
  } 
}

/*!
  Realize the post tracking operations. Mostly visibility tests
*/
bool
vpMbKltTracker::postTracking(const vpImage<unsigned char>& I, vpColVector &w)
{
  // # For a better Post Tracking, tracker should reinitialize if so faces don't have enough points but are visible.
  // # Here we are not doing it for more speed performance.
  bool reInitialisation = false;
  
  unsigned int initialNumber = 0;
  unsigned int currentNumber = 0;
  unsigned int shift = 0;
  for (unsigned int i = 0; i < faces.size(); i += 1){
    if(faces[i]->isVisible()){
      initialNumber += faces[i]->getInitialNumberPoint();
      if(faces[i]->hasEnoughPoints()){    
        vpSubColVector sub_w(w, shift, 2*faces[i]->getNbPointsCur());
        faces[i]->removeOutliers(sub_w, threshold_outlier);
        shift += 2*faces[i]->getNbPointsCur();
        
        currentNumber += faces[i]->getNbPointsCur();
      }
//       else{
//         reInitialisation = true;
//         break;
//       }
    }
  }
  
//   if(!reInitialisation){
    double value = percentGood * (double)initialNumber;
    if((double)currentNumber < value){
//     std::cout << "Too many point disappear : " << initialNumber << "/" << currentNumber << std::endl;
      reInitialisation = true;
    }
    else{
      if(!useOgre)
        faces.setVisible(I, cam, cMo, angleAppears, angleDisappears, reInitialisation);
      else{
#ifdef VISP_HAVE_OGRE    
        faces.setVisibleOgre(I, cam, cMo, angleAppears, angleDisappears, reInitialisation);
#else
        faces.setVisible(I, cam, cMo, angleAppears, angleDisappears, reInitialisation);
#endif
      }
    }
//   }
  
  if(reInitialisation)
    return true;
  
  return false;
}

/*!
  Realize the VVS loop for the tracking

  \param nbInfos : Size of the features
  \param w : weight of the features after M-Estimation.
*/
void
vpMbKltTracker::computeVVS(const unsigned int &nbInfos, vpColVector &w)
{
  vpMatrix J;     // interaction matrix
  vpColVector R;  // residu
  vpMatrix J_true;     // interaction matrix
  vpColVector R_true;  // residu
  vpColVector v;  // "speed" for VVS
  vpHomography H;
  vpColVector w_true;
  vpRobust robust(2*nbInfos);

  vpMatrix JTJ, JTR;
  
  double normRes = 0;
  double normRes_1 = -1;
  unsigned int iter = 0;

  R.resize(2*nbInfos);
  J.resize(2*nbInfos, 6, 0);
  
  while( ((int)((normRes - normRes_1)*1e8) != 0 )  && (iter<maxIter) ){
    
    unsigned int shift = 0;
    for (unsigned int i = 0; i < faces.size(); i += 1){
      if(faces[i]->isVisible() && faces[i]->hasEnoughPoints()){
        vpSubColVector subR(R, shift, 2*faces[i]->getNbPointsCur());
        vpSubMatrix subJ(J, shift, 0, 2*faces[i]->getNbPointsCur(), 6);
        try{
          faces[i]->computeHomography(ctTc0, H);
          faces[i]->computeInteractionMatrixAndResidu(subR, subJ);
        }catch(...){
          std::cerr << "exception while tracking face " << i << std::endl;
          throw ;
        }

        shift += 2*faces[i]->getNbPointsCur();
      }
    }

      /* robust */
    if(iter == 0){
      w_true.resize(2*nbInfos);
      w.resize(2*nbInfos);
      w = 1;
    }
    robust.setIteration(iter);
    robust.setThreshold(2/cam.get_px());
    robust.MEstimator( vpRobust::TUKEY, R, w);
    
    if(computeCovariance){
      R_true = R;
      J_true = J;
    }

    normRes_1 = normRes;
    normRes = 0;
    for (unsigned int i = 0; i < static_cast<unsigned int>(R.getRows()); i += 1){
      w_true = w[i] * w[i];
      R[i] = R[i] * w[i];
      normRes += R[i];
    }

    if((iter == 0) || compute_interaction){
      for(unsigned int i=0; i<static_cast<unsigned int>(R.getRows()); i++){
        for(unsigned int j=0; j<6; j++){
          J[i][j] *= w[i];
        }
      }
    }
    
    JTJ = J.AtA();
    computeJTR(J, R, JTR);
    v = -lambda * JTJ.pseudoInverse(1e-16) * JTR;
    
    ctTc0 = vpExponentialMap::direct(v).inverse() * ctTc0;
    
    iter++;
  }
  
  if(computeCovariance){
    vpMatrix D;
    D.diag(w_true);
    covarianceMatrix = vpMatrix::computeCovarianceMatrix(J_true,v,-lambda*R_true,D);
  }
  
  cMo = ctTc0 * c0Mo;
}

/*!
  Realize the tracking of the object in the image

  \throw vpException : if the tracking is supposed to have failed

  \param I : the input image
*/
void
vpMbKltTracker::track(const vpImage<unsigned char>& I)
{   
  unsigned int nbInfos;
  unsigned int nbFaceUsed;
  preTracking(I, nbInfos, nbFaceUsed);
  
  if(nbInfos < 4 || nbFaceUsed == 0){
    vpERROR_TRACE("\n\t\t Error-> not enough data") ;
    throw vpTrackingException(vpTrackingException::notEnoughPointError, "\n\t\t Error-> not enough data");
  }

  vpColVector w;
  computeVVS(nbInfos, w);  

  if(postTracking(I, w))
    reinit(I);
}

/*!
  Load the xml configuration file. An example of such a file is provided in loadConfigFile(const char*) documentation.
  From the configuration file initialize the parameters corresponding to the objects: KLT, camera.

  \warning To clean up memory allocated by the xml library, the user has to call
  vpXmlParser::cleanup() before the exit().

  \param configFile : full name of the xml file.

  \sa loadConfigFile(const char*), vpXmlParser::cleanup()
*/
void 
vpMbKltTracker::loadConfigFile(const std::string& configFile)
{
  vpMbKltTracker::loadConfigFile(configFile.c_str());
}

/*!
  Load the xml configuration file.
  From the configuration file initialize the parameters corresponding to the objects: KLT, camera.

  \warning To clean up memory allocated by the xml library, the user has to call
  vpXmlParser::cleanup() before the exit().

  \throw vpException::ioError if the file has not been properly parsed (file not
  found or wrong format for the data). 

  \param configFile : full name of the xml file.

  The XML configuration file has the following form:
  \code
<?xml version="1.0"?>
<conf>
  <camera>
    <width>640</width>
    <height>480</height>
    <u0>320</u0>
    <v0>240</v0>
    <px>686.24</px>
    <py>686.24</py>
  </camera>
  <face>
    <angle_appear>65</angle_appear>
    <angle_disappear>85</angle_disappear>
    <near_clipping>0.01</near_clipping>
    <far_clipping>0.90</far_clipping>
    <fov_clipping>1</fov_clipping>
  </face>
  <klt>
    <mask_border>10</mask_border>
    <max_features>10000</max_features>
    <window_size>5</window_size>
    <quality>0.02</quality>
    <min_distance>10</min_distance>
    <harris>0.02</harris>
    <size_block>3</size_block>
    <pyramid_lvl>3</pyramid_lvl>
  </klt>
</conf>
  \endcode

  \sa loadConfigFile(const std::string&), vpXmlParser::cleanup()
*/
void
vpMbKltTracker::loadConfigFile(const char* configFile)
{
#ifdef VISP_HAVE_XML2
  vpMbtKltXmlParser xmlp;
  
  xmlp.setMaxFeatures(10000);
  xmlp.setWindowSize(5);
  xmlp.setQuality(0.01);
  xmlp.setMinDistance(5);
  xmlp.setHarrisParam(0.01);
  xmlp.setBlockSize(3);
  xmlp.setPyramidLevels(3);
  xmlp.setMaskBorder(maskBorder);
  xmlp.setAngleAppear(vpMath::deg(angleAppears));
  xmlp.setAngleDisappear(vpMath::deg(angleDisappears));
  
  try{
    std::cout << " *********** Parsing XML for MBT KLT Tracker ************ " << std::endl;
    xmlp.parse(configFile);
  }
  catch(...){
    vpERROR_TRACE("Can't open XML file \"%s\"\n ", configFile);
    throw vpException(vpException::ioError, "problem to parse configuration file.");
  }

  vpCameraParameters camera;
  xmlp.getCameraParameters(camera);
  setCameraParameters(camera); 
  
  tracker.setMaxFeatures((int)xmlp.getMaxFeatures());
  tracker.setWindowSize((int)xmlp.getWindowSize());
  tracker.setQuality(xmlp.getQuality());
  tracker.setMinDistance(xmlp.getMinDistance());
  tracker.setHarrisFreeParameter(xmlp.getHarrisParam());
  tracker.setBlockSize((int)xmlp.getBlockSize());
  tracker.setPyramidLevels((int)xmlp.getPyramidLevels());
  maskBorder = xmlp.getMaskBorder();
  angleAppears = vpMath::rad(xmlp.getAngleAppear());
  angleDisappears = vpMath::rad(xmlp.getAngleDisappear());
  
  if(xmlp.hasNearClippingDistance())
    setNearClippingDistance(xmlp.getNearClippingDistance());
  
  if(xmlp.hasFarClippingDistance())
    setFarClippingDistance(xmlp.getFarClippingDistance());
  
  if(xmlp.getFovClipping())
    setClipping(clippingFlag = clippingFlag | vpMbtPolygon::FOV_CLIPPING);

#else
  vpTRACE("You need the libXML2 to read the config file %s", configFile);
#endif
}

/*!
  Display the 3D model at a given position using the given camera parameters

  \param I : The image.
  \param cMo_ : Pose used to project the 3D model into the image.
  \param camera : The camera parameters.
  \param col : The desired color.
  \param thickness : The thickness of the lines.
  \param displayFullModel : Boolean to say if all the model has to be displayed.
*/
void
vpMbKltTracker::display(const vpImage<unsigned char>& I, const vpHomogeneousMatrix &cMo_, const vpCameraParameters & camera,
                        const vpColor& col, const unsigned int thickness, const bool displayFullModel)
{
  vpCameraParameters c = camera;
  
  if(clippingFlag > 3) // Contains at least one FOV constraint
    c.computeFov(I.getWidth(), I.getHeight());
  
  for (unsigned int i = 0; i < faces.size(); i += 1){
    if(displayFullModel || faces[i]->isVisible())
    {
      faces[i]->changeFrame(cMo_);
      faces[i]->computeRoiClipped(c);
      std::vector<std::pair<vpImagePoint,unsigned int> > roi;
      faces[i]->getRoiClipped(c, roi);
      
      for (unsigned int j = 0; j < roi.size(); j += 1){
        if(((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::NEAR_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::FAR_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::DOWN_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::UP_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::LEFT_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::RIGHT_CLIPPING) == 0)){
          vpImagePoint ip1, ip2;
          ip1 = roi[j].first;
          ip2 = roi[(j+1)%roi.size()].first;
          
          vpDisplay::displayLine (I, ip1, ip2, col, thickness);
        }
      }
    }
    if(displayFeatures && faces[i]->hasEnoughPoints() && faces[i]->isVisible()) {
        faces[i]->displayPrimitive(I);
//         faces[i]->displayNormal(I);
    }
  }

#ifdef VISP_HAVE_OGRE
  if(useOgre)
    faces.displayOgre(cMo_);
#endif
}

/*!
  Display the 3D model at a given position using the given camera parameters

  \param I : The color image.
  \param cMo_ : Pose used to project the 3D model into the image.
  \param camera : The camera parameters.
  \param col : The desired color.
  \param thickness : The thickness of the lines.
  \param displayFullModel : Boolean to say if all the model has to be displayed.
*/
void
vpMbKltTracker::display(const vpImage<vpRGBa>& I, const vpHomogeneousMatrix &cMo_, const vpCameraParameters & camera,
                        const vpColor& col , const unsigned int thickness, const bool displayFullModel)
{
  vpCameraParameters c = camera;
  
  if(clippingFlag > 3) // Contains at least one FOV constraint
    c.computeFov(I.getWidth(), I.getHeight());
  
  for (unsigned int i = 0; i < faces.size(); i += 1){
    if(displayFullModel || faces[i]->isVisible())
    {
      faces[i]->changeFrame(cMo_);
      faces[i]->computeRoiClipped(c);      
      std::vector<std::pair<vpImagePoint,unsigned int> > roi;
      faces[i]->getRoiClipped(c, roi);
      
      for (unsigned int j = 0; j < roi.size(); j += 1){
        if(((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::NEAR_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::FAR_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::DOWN_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::UP_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::LEFT_CLIPPING) == 0) && 
           ((roi[(j+1)%roi.size()].second & roi[j].second & vpMbtPolygon::RIGHT_CLIPPING) == 0)){
          vpImagePoint ip1, ip2;
          ip1 = roi[j].first;
          ip2 = roi[(j+1)%roi.size()].first;
          
          vpDisplay::displayLine (I, ip1, ip2, col, thickness);
        }
      }
    }
    if(displayFeatures && faces[i]->hasEnoughPoints() && faces[i]->isVisible()) {
        faces[i]->displayPrimitive(I);
//         faces[i]->displayNormal(I);
    }
  }

#ifdef VISP_HAVE_OGRE
  if(useOgre)
    faces.displayOgre(cMo_);
#endif
}

/*!
  Test the quality of the tracking.
  The tracking is supposed to fail if less than 10 points are tracked.

  \todo Find a efficient way to test the quality.

  \throw vpTrackingException::fatalError  if the test fails.
*/
void
vpMbKltTracker::testTracking()
{
  unsigned int nbTotalPoints = 0;
  for (unsigned int i = 0; i < faces.size(); i += 1){
    if(faces[i]->isVisible()){
      nbTotalPoints += faces[i]->getNbPointsCur();
    }
  }

  if(nbTotalPoints < 10){
    std::cerr << "test tracking failed (too few points to realize a good tracking)." << std::endl;
    throw vpTrackingException(vpTrackingException::fatalError,
          "test tracking failed (too few points to realize a good tracking).");
  }
}

#endif //VISP_HAVE_OPENCV
