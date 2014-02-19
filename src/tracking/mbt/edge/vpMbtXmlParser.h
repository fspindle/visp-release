/****************************************************************************
 *
 * $Id: vpMbtXmlParser.h 4582 2014-01-14 14:02:46Z ayol $
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
 * Load XML parameters of the Model based tracker (using edges).
 *
 * Authors:
 * Nicolas Melchior
 * Romain Tallonneau
 * Eric Marchand
 * Aurelien Yol
 *
 *****************************************************************************/

/*!
 * \file vpMbtXmlParser.h
 * \brief Parse an Xml file to extract configuration parameters of a mbtConfig object.
*/

#ifndef vpMbtXmlParser_HH
#define vpMbtXmlParser_HH

#include <visp/vpConfig.h>

#ifdef VISP_HAVE_XML2

#include <libxml/xmlmemory.h>      /* Fonctions de la lib XML.                */

#include <visp/vpMbXmlParser.h>
#include <visp/vpMe.h>

/*!
  \class vpMbtXmlParser
  \brief Parse an Xml file to extract configuration parameters of a mbtConfig object.
  \ingroup ModelBasedTracking

  Data parser for the model based tracker.

 */
class VISP_EXPORT vpMbtXmlParser: virtual public vpMbXmlParser
{
protected:
  //! Moving edges parameters.
  vpMe m_ecm;

  typedef enum{
    ecm = vpMbXmlParser::last,
    mask,
    size,
    nb_mask,
    range,
    tracking,
    contrast,
    edge_threshold,
    mu1,
    mu2,
    sample,
    step,
    nb_sample,
    last
  } dataToParseMb;


public:

	vpMbtXmlParser();
	virtual ~vpMbtXmlParser();

  void getMe(vpMe& _ecm) const { _ecm = this->m_ecm;}
  

  void parse(const char * filename);

  virtual void readMainClass(xmlDocPtr doc, xmlNodePtr node);
  void read_ecm (xmlDocPtr doc, xmlNodePtr node);
  void read_sample (xmlDocPtr doc, xmlNodePtr node);
  void read_mask (xmlDocPtr doc, xmlNodePtr node);
  void read_range (xmlDocPtr doc, xmlNodePtr node);
  void read_contrast (xmlDocPtr doc, xmlNodePtr node);
  
  void setMovingEdge(const vpMe &_ecm){ m_ecm = _ecm; }
	
  void writeMainClass(xmlNodePtr node);
	
protected:
  void init();

};

#endif

#endif /* NMBTXMLPARSER_H_ */



