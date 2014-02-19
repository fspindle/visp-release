/****************************************************************************
 *
 * $Id: vpRequest.cpp 4632 2014-02-03 17:06:40Z fspindle $
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
 * Network request.
 *
 * Authors:
 * Aurelien Yol
 *
 *****************************************************************************/

#include <visp/vpRequest.h>

vpRequest::vpRequest()
  : request_id(""), listOfParams()
{}

vpRequest::~vpRequest()
{}
  
/*!
  Add a message as parameter of the request.
  
  \sa vpNetwork::addParameterObject()
  
  \param params : Array of characters representing the message to add.
*/
void vpRequest::addParameter(char *params)
{  
  std::string val = params;
  listOfParams.push_back(val);
}

/*!
  Add a message as parameter of the request.
  
  \sa vpNetwork::addParameterObject()
  
  \param params : std::string representing the message to add.
*/
void vpRequest::addParameter(std::string &params)
{
  listOfParams.push_back(params);
}

/*!
  Add messages as parameters of the request.
  Each message corresponds to one parameter.
  
  \sa vpNetwork::addParameterObject()
  
  \param listOfparams : Array of std::string representing the messages to add.
*/
void vpRequest::addParameter(std::vector< std::string > &listOfparams)
{  
  for(unsigned int i = 0; i < listOfparams.size() ; i++)
    listOfparams.push_back(listOfparams[i]);
}
