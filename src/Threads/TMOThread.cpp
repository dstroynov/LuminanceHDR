/*
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 *
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing 
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <iostream>

#include "TMOThread.h"
#include "Libpfs/colorspace.h"
#include "Common/config.h"
#include "Filter/pfscut.h"
#include "Filter/pfsgamma.h"
#include "Fileformat/pfsoutldrimage.h"

pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int xSize);


TMOThread::TMOThread(pfs::Frame *frame, const TonemappingOptions &opts) :
QThread(0), opts(opts), out_CS(pfs::CS_RGB)
{
	pfs::DOMIO pfsio;
  
	workingframe = pfscopy(frame);
  
	ph = new ProgressHelper(0);
  
	if (opts.pregamma != 1.0f)
  {
		applyGammaOnFrame( workingframe, opts.pregamma );
	}
  
	if ((opts.xsize != opts.origxsize) && !opts.tonemapSelection)
  {
		pfs::Frame *resized = resizeFrame(workingframe, opts.xsize);
		pfsio.freeFrame(workingframe);
		workingframe = resized;
	}
  
	// Convert to CS_XYZ: tm operator now use this colorspace
	pfs::Channel *X, *Y, *Z;
	workingframe->getXYZChannels( X, Y, Z );
	pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                           pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());	
}

TMOThread::~TMOThread()
{
	wait();
	delete ph;
	std::cout << "~TMOThread()" << std::endl;
}

void TMOThread::terminateRequested()
{
	std::cout << "TMOThread::terminateRequested()" << std::endl;
	ph->terminate(true);
}

void TMOThread::startTonemapping()
{
  start();
}

void TMOThread::finalize()
{
  LuminanceOptions *luminance_options = LuminanceOptions::getInstance();
  
	if (!(ph->isTerminationRequested()))
  {
		const QImage& res = fromLDRPFStoQImage(workingframe, out_CS);
    
    if ( luminance_options->tmowindow_showprocessed )
    {
      emit processedFrame(workingframe);
    }
		emit imageComputed(res);
	}
	emit finished();
	emit deleteMe(this);
}

