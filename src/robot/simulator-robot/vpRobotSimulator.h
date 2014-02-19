/****************************************************************************
 *
 * $Id: vpRobotSimulator.h 4574 2014-01-09 08:48:51Z fspindle $
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
 * Basic class used to make robot simulators.
 *
 * Authors:
 * Nicolas Melchior
 *
 *****************************************************************************/

#ifndef vpRobotSimulator_HH
#define vpRobotSimulator_HH

/*!
  \file vpRobotSimulator.h
  \brief Basic class used to make robot simulators.
*/

#include <visp/vpConfig.h>
#include <visp/vpRobot.h>
#include <visp/vpTime.h>
#include <visp/vpVelocityTwistMatrix.h>


/*!
  \class vpRobotSimulator

  \ingroup RobotSimuWithVisu, RobotSimuWithoutVisu
  
  \brief This class aims to be a basis used to create all the
  robot simulators.
  
*/
class VISP_EXPORT vpRobotSimulator : public vpRobot
{
protected:
  double delta_t_; // sampling time in second

public:
  vpRobotSimulator();
  /*!
    Basic destructor
  */
  virtual ~vpRobotSimulator() {};

  /*!
    Return the sampling time.

    \return Sampling time in second used to compute the robot displacement from
    the velocity applied to the robot during this time.
  */
  inline double getSamplingTime() const
  {
    return(this->delta_t_);
  }

  /*!
    Set the sampling time.

    \param delta_t : Sampling time in second used to compute the robot displacement from
    the velocity applied to the robot during this time.

  */
  virtual inline void setSamplingTime(const double &delta_t)
  {
    this->delta_t_ = delta_t;
  }
};

#endif
