/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-24-01
 * Description : auto levels image filter.
 *
 * Copyright (C) 2005-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "autolevelsfilter.h"

// C++ includes

#include <cstdio>
#include <cmath>

// KDE includes

#include <kdebug.h>

// Local includes

#include "dimg.h"
#include "imagehistogram.h"
#include "imagelevels.h"

namespace Digikam
{

AutoLevelsFilter::AutoLevelsFilter(QObject* parent)
    : DImgThreadedFilter(parent)
{
    initFilter();
}

AutoLevelsFilter::AutoLevelsFilter(DImg* orgImage, const DImg* refImage, QObject* parent)
    : DImgThreadedFilter(orgImage, parent, "AutoLevelsFilter"),
      m_refImage(*refImage)
{
    initFilter();
}

AutoLevelsFilter::~AutoLevelsFilter()
{
    cancelFilter();
}

void AutoLevelsFilter::filterImage()
{
    if (m_refImage.isNull())
    {
        m_refImage = m_orgImage;
    }

    autoLevelsCorrectionImage();
    m_destImage = m_orgImage;
}

/** Performs histogram auto correction of levels.
    This method maximizes the tonal range in the Red,
    Green, and Blue channels. It search the image shadow and highlight
    limit values and adjust the Red, Green, and Blue channels
    to a full histogram range.*/
void AutoLevelsFilter::autoLevelsCorrectionImage()
{
    if (m_orgImage.sixteenBit() != m_refImage.sixteenBit())
    {
        kDebug() << "Ref. image and Org. has different bits depth";
        return;
    }

    uchar* data     = m_orgImage.bits();
    int w           = m_orgImage.width();
    int h           = m_orgImage.height();
    bool sixteenBit = m_orgImage.sixteenBit();

    uchar* desData            = 0;
    ImageHistogram* histogram = 0;
    ImageLevels* levels       = 0;

    postProgress(10);

    // Create the new empty destination image data space.
    if (runningFlag())
    {
        if (sixteenBit)
        {
            desData = new uchar[w*h*8];
        }
        else
        {
            desData = new uchar[w*h*4];
        }

        postProgress(20);
    }

    // Create an histogram of the reference image.
    if (runningFlag())
    {
        histogram = new ImageHistogram(m_refImage.bits(), m_refImage.width(),
                                       m_refImage.height(), m_refImage.sixteenBit());
        histogram->calculate();
        postProgress(30);
    }

    // Create an empty instance of levels to use.
    if (runningFlag())
    {
        levels = new ImageLevels(sixteenBit);
        postProgress(40);
    }

    // Initialize an auto levels correction of the histogram.
    if (runningFlag())
    {
        levels->levelsAuto(histogram);
        postProgress(50);
    }

    // Calculate the LUT to apply on the image.
    if (runningFlag())
    {
        levels->levelsLutSetup(AlphaChannel);
        postProgress(60);
    }

    // Apply the lut to the image.
    if (runningFlag())
    {
        levels->levelsLutProcess(data, desData, w, h);
        postProgress(70);
    }

    if (runningFlag())
    {
        if (sixteenBit)
        {
            memcpy (data, desData, w*h*8);
        }
        else
        {
            memcpy (data, desData, w*h*4);
        }

        postProgress(80);
    }

    delete [] desData;
    delete histogram;
    delete levels;

    if (runningFlag())
    {
        postProgress(90);
    }
}

FilterAction AutoLevelsFilter::filterAction()
{
    return DefaultFilterAction<AutoLevelsFilter>();
}

void AutoLevelsFilter::readParameters(const FilterAction& /*action*/)
{
    return;
}

}  // namespace Digikam