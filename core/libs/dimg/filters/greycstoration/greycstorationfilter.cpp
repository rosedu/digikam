/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2007-12-03
 * Description : Greycstoration interface.
 *
 * Copyright (C) 2007-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2010 by Martin Klapetek <martin dot klapetek at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

/** Don't use CImg interface (keyboard/mouse interaction) */
#define cimg_display 0
/** Only print debug information on the console */
#define cimg_debug 1

#include "greycstorationfilter.h"

// C++ includes

#include <cassert>

// KDE includes

#include <kdebug.h>

#define cimg_plugin "greycstoration.h"
// Unix-like (Linux, Solaris, BSD, MacOSX, Irix,...).
#if defined(unix)       || defined(__unix)      || defined(__unix__) \
 || defined(linux)      || defined(__linux)     || defined(__linux__) \
 || defined(sun)        || defined(__sun) \
 || defined(BSD)        || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined __DragonFly__ \
 || defined(__MACOSX__) || defined(__APPLE__) \
 || defined(sgi)        || defined(__sgi) \
 || defined(__CYGWIN__)
#include <pthread.h>
#endif

/** Uncomment this line if you use future GreycStoration implementation with GFact parameter
 */
#define GREYSTORATION_USING_GFACT 1

// KDE includes

#include <solid/device.h>

// CImg includes

#include "CImg.h"

extern "C"
{
#include <unistd.h>
}

using namespace cimg_library;

namespace Digikam
{

class GreycstorationFilter::GreycstorationFilterPriv
{

public:

    GreycstorationFilterPriv() :
        gfact(1.0),
        computationThreads(2),
        mode(GreycstorationFilter::Restore)
    {
    }

    float                   gfact;

    int                     computationThreads;  // Number of threads used by CImg during computation.
    int                     mode;                // The interface running mode.

    QSize                   newSize;
    QImage                  inPaintingMask;      // Mask for inpainting.

    GreycstorationContainer settings;            // Current Greycstoraion algorithm settings.

    CImg<>                  img;                 // Main image.
    CImg<uchar>             mask;                // The mask used with inpaint or resize mode
};

GreycstorationFilter::GreycstorationFilter(QObject* parent)
    : DImgThreadedFilter(parent),
      d(new GreycstorationFilterPriv)
{
    setOriginalImage(DImg());
    setSettings(GreycstorationContainer());
    setMode(Restore);
    setInPaintingMask(QImage());
}

GreycstorationFilter::GreycstorationFilter(DImg* orgImage,
        const GreycstorationContainer& settings,
        int mode,
        int newWidth, int newHeight,
        const QImage& inPaintingMask,
        QObject* parent)
    : DImgThreadedFilter(parent),
      d(new GreycstorationFilterPriv)
{
    setOriginalImage(orgImage->copyImageData());
    setSettings(settings);
    setMode(mode, newWidth, newHeight);
    setInPaintingMask(inPaintingMask);
    setup();
}

GreycstorationFilter::~GreycstorationFilter()
{
    cancelFilter();
    delete d;
}

void GreycstorationFilter::setSettings(const GreycstorationContainer& settings)
{
    d->settings = settings;
}

void GreycstorationFilter::setMode(int mode, int newWidth, int newHeight)
{
    d->mode    = mode;
    d->newSize = QSize(newWidth, newHeight);
}

void GreycstorationFilter::setInPaintingMask(const QImage& inPaintingMask)
{
    d->inPaintingMask = inPaintingMask;
}

void GreycstorationFilter::computeChildrenThreads()
{
    // Check number of CPU with Solid interface.

    const int numProcs    = qMax(Solid::Device::listFromType(Solid::DeviceInterface::Processor).count(), 1);
    const int maxThreads  = 16;
    d->computationThreads = qMin(maxThreads, 2 + ((numProcs - 1) * 2));
    kDebug() << "GreycstorationFilter::Computation threads: " << d->computationThreads;
}

void GreycstorationFilter::setup()
{
    // NOTE: Sound like using more than 2 threads at the same time create dysfunctions to stop Cimg children threads
    //       calling cancelFilter(). We limit children threads to 2.
    //computeChildrenThreads();

    if (m_orgImage.sixteenBit())   // 16 bits image.
    {
        d->gfact = 1.0/256.0;
    }

    if (d->mode == Resize || d->mode == SimpleResize)
    {
        m_destImage = DImg(d->newSize.width(), d->newSize.height(),
                           m_orgImage.sixteenBit(), m_orgImage.hasAlpha());

        kDebug() << "GreycstorationFilter::Resize: new size: ("
                 << d->newSize.width() << ", " << d->newSize.height() << ")";
    }
    else
    {
        m_destImage = DImg(m_orgImage.width(), m_orgImage.height(),
                           m_orgImage.sixteenBit(), m_orgImage.hasAlpha());
    }

    initFilter();
}

QString GreycstorationFilter::cimgVersionString()
{
    return QString::number(cimg_version);
}

// We need to re-implement this method from DImgThreadedFilter class because
// target image size can be different from original if d->mode = Resize.

void GreycstorationFilter::initFilter()
{
    // (left out here: creation of m_destImage)

    if (m_master)
    {
        startFilterDirectly();
    }
}

void GreycstorationFilter::cancelFilter()
{
    // Because Greycstoration algorithm run in a child thread, we need
    // to stop it before to stop this thread.
    if (d->img.greycstoration_is_running())
    {
        // If the user abort, we stop the algorithm.
        kDebug() << "Stop Greycstoration computation...";
        d->img.greycstoration_stop();
    }

    // And now when stop main loop and clean up all
    DImgThreadedFilter::cancelFilter();
}

void GreycstorationFilter::filterImage()
{
    register int x, y;

    kDebug() << "Initialization...";

    uchar* data = m_orgImage.bits();
    int width   = m_orgImage.width();
    int height  = m_orgImage.height();

    // convert DImg (interleaved RGBA) to CImg (planar RGBA)
    if (!m_orgImage.sixteenBit())           // 8 bits image.
    {
        d->img = CImg<unsigned char>(data, 4, width, height, 1, false).
                 get_permute_axes("yzvx");
    }
    else                                    // 16 bits image.
    {
        d->img = CImg<unsigned short>((unsigned short*)data, 4, width, height, 1, false).
                 get_permute_axes("yzvx");
    }

    kDebug() << "Process Computation...";

    try
    {
        switch (d->mode)
        {
            case Restore:
                restoration();
                break;

            case InPainting:
                inpainting();
                break;

            case Resize:
                resize();
                break;

            case SimpleResize:
                simpleResize();
                break;
        }
    }
    catch (...)        // Everything went wrong.
    {
        kDebug() << "Error during Greycstoration filter computation!";

        return;
    }

    if (!runningFlag())
    {
        return;
    }

    // Copy CImg onto destination.

    kDebug() << "Finalization...";

    uchar* newData = m_destImage.bits();
    int newWidth   = m_destImage.width();
    int newHeight  = m_destImage.height();

    if (!m_orgImage.sixteenBit())           // 8 bits image.
    {
        uchar* ptr = newData;

        for (y = 0; y < newHeight; ++y)
        {
            for (x = 0; x < newWidth; ++x)
            {
                // Overwrite RGB values to destination.
                ptr[0] = static_cast<uchar>(d->img(x, y, 0));        // Blue
                ptr[1] = static_cast<uchar>(d->img(x, y, 1));        // Green
                ptr[2] = static_cast<uchar>(d->img(x, y, 2));        // Red
                ptr[3] = static_cast<uchar>(d->img(x, y, 3));        // Alpha
                ptr    += 4;
            }
        }
    }
    else                                     // 16 bits image.
    {
        unsigned short* ptr = (unsigned short*)newData;

        for (y = 0; y < newHeight; ++y)
        {
            for (x = 0; x < newWidth; ++x)
            {
                // Overwrite RGB values to destination.
                ptr[0] = static_cast<unsigned short>(d->img(x, y, 0));        // Blue
                ptr[1] = static_cast<unsigned short>(d->img(x, y, 1));        // Green
                ptr[2] = static_cast<unsigned short>(d->img(x, y, 2));        // Red
                ptr[3] = static_cast<unsigned short>(d->img(x, y, 3));        // Alpha
                ptr    += 4;
            }
        }
    }
}

void GreycstorationFilter::restoration()
{
    for (uint iter = 0 ; runningFlag() && (iter < d->settings.nbIter) ; ++iter)
    {
        // This function will start a thread running one iteration of the GREYCstoration filter.
        // It returns immediately, so you can do what you want after (update a progress bar for
        // instance).
        d->img.greycstoration_run(d->settings.amplitude,
                                  d->settings.sharpness,
                                  d->settings.anisotropy,
                                  d->settings.alpha,
                                  d->settings.sigma,
#ifdef GREYSTORATION_USING_GFACT
                                  d->gfact,
#endif
                                  d->settings.dl,
                                  d->settings.da,
                                  d->settings.gaussPrec,
                                  d->settings.interp,
                                  d->settings.fastApprox,
                                  d->settings.tile,
                                  d->settings.btile,
                                  d->computationThreads);

        iterationLoop(iter);
    }
}

void GreycstorationFilter::inpainting()
{
    if (!d->inPaintingMask.isNull())
    {
        // Copy the inpainting image data into a CImg type image with three channels and no alpha.

        register int x, y;

        d->mask    = CImg<uchar>(d->inPaintingMask.width(), d->inPaintingMask.height(), 1, 3);
        uchar* ptr = d->inPaintingMask.bits();

        for (y = 0; y < d->inPaintingMask.height(); ++y)
        {
            for (x = 0; x < d->inPaintingMask.width(); ++x)
            {
                d->mask(x, y, 0) = ptr[2];        // blue.
                d->mask(x, y, 1) = ptr[1];        // green.
                d->mask(x, y, 2) = ptr[0];        // red.
                ptr += 4;
            }
        }
    }
    else
    {
        kDebug() << "Inpainting image: mask is null!";
        stop();
        return;
    }

    for (uint iter=0 ; runningFlag() && (iter < d->settings.nbIter) ; ++iter)
    {
        // This function will start a thread running one iteration of the GREYCstoration filter.
        // It returns immediately, so you can do what you want after (update a progress bar for
        // instance).
        d->img.greycstoration_run(d->mask,
                                  d->settings.amplitude,
                                  d->settings.sharpness,
                                  d->settings.anisotropy,
                                  d->settings.alpha,
                                  d->settings.sigma,
#ifdef GREYSTORATION_USING_GFACT
                                  d->gfact,
#endif
                                  d->settings.dl,
                                  d->settings.da,
                                  d->settings.gaussPrec,
                                  d->settings.interp,
                                  d->settings.fastApprox,
                                  d->settings.tile,
                                  d->settings.btile,
                                  d->computationThreads);

        iterationLoop(iter);
    }
}

void GreycstorationFilter::resize()
{
    const bool anchor       = true;   // Anchor original pixels.
    const unsigned int init = 5;      // Initial estimate (1=block, 3=linear, 5=bicubic).

    int w = m_destImage.width();
    int h = m_destImage.height();

    d->mask.assign(d->img.dimx(), d->img.dimy(), 1, 1, 255);

    if (!anchor)
    {
        d->mask.resize(w, h, 1, 1, 1);
    }
    else
    {
        d->mask = !d->mask.resize(w, h, 1, 1, 4);
    }

    d->img.resize(w, h, 1, -100, init);

    for (uint iter = 0 ; runningFlag() && (iter < d->settings.nbIter) ; ++iter)
    {
        // This function will start a thread running one iteration of the GREYCstoration filter.
        // It returns immediately, so you can do what you want after (update a progress bar for
        // instance).
        d->img.greycstoration_run(d->mask,
                                  d->settings.amplitude,
                                  d->settings.sharpness,
                                  d->settings.anisotropy,
                                  d->settings.alpha,
                                  d->settings.sigma,
#ifdef GREYSTORATION_USING_GFACT
                                  d->gfact,
#endif
                                  d->settings.dl,
                                  d->settings.da,
                                  d->settings.gaussPrec,
                                  d->settings.interp,
                                  d->settings.fastApprox,
                                  d->settings.tile,
                                  d->settings.btile,
                                  d->computationThreads);

        iterationLoop(iter);
    }
}

void GreycstorationFilter::simpleResize()
{
    const unsigned int method = 3;      // Initial estimate (0, none, 1=block, 3=linear, 4=grid, 5=bicubic).

    int w = m_destImage.width();
    int h = m_destImage.height();

    while (d->img.dimx() > 2*w &&
           d->img.dimy() > 2*h)
    {
        d->img.resize_halfXY();
    }

    d->img.resize(w, h, -100, -100, method);
}

void GreycstorationFilter::iterationLoop(uint iter)
{
    uint mp  = 0;
    uint p   = 0;

    do
    {
        usleep(100000);

        if (runningFlag())
        {
            // Update the progress bar in dialog. We simply computes the global
            // progression index (including all iterations).

            p = (uint)((iter*100 + d->img.greycstoration_progress())/d->settings.nbIter);

            if (p > mp)
            {
                postProgress(p);
                mp = p;
            }
        }
    }
    while (d->img.greycstoration_is_running() && runningFlag());

    // A delay is require here. I suspect a sync problem between threads
    // used by GreycStoration algorithm.
    usleep(100000);
}

FilterAction GreycstorationFilter::filterAction()
{
    FilterAction action(FilterIdentifier(), CurrentVersion());
    action.setDisplayableName(DisplayableName());

    action.addParameter("alpha", d->settings.alpha);
    action.addParameter("amplitude", d->settings.amplitude);
    action.addParameter("anisotropy", d->settings.anisotropy);
    action.addParameter("btile", d->settings.btile);
    action.addParameter("da", d->settings.da);
    action.addParameter("dl", d->settings.dl);
    action.addParameter("fastApprox", d->settings.fastApprox);
    action.addParameter("gaussPrec", d->settings.gaussPrec);
    action.addParameter("interp", d->settings.interp);
    action.addParameter("nbIter", d->settings.nbIter);
    action.addParameter("sharpness", d->settings.sharpness);
    action.addParameter("sigma", d->settings.sigma);
    action.addParameter("tile", d->settings.tile);

    return action;
}

void GreycstorationFilter::readParameters(const Digikam::FilterAction& action)
{
    d->settings.alpha = action.parameter("alpha").toFloat();
    d->settings.amplitude = action.parameter("amplitude").toFloat();
    d->settings.anisotropy = action.parameter("anisotropy").toFloat();
    d->settings.btile = action.parameter("btile").toInt();
    d->settings.da= action.parameter("da").toFloat();
    d->settings.dl= action.parameter("dl").toFloat();
    d->settings.fastApprox = action.parameter("fastApprox").toBool();
    d->settings.gaussPrec = action.parameter("gaussPrec").toFloat();
    d->settings.interp = action.parameter("interp").toFloat();
    d->settings.nbIter = action.parameter("nbIter").toUInt();
    d->settings.sharpness = action.parameter("sharpness").toFloat();
    d->settings.sigma = action.parameter("sigma").toFloat();
    d->settings.tile = action.parameter("tile").toInt();
}

}  // namespace Digikam
