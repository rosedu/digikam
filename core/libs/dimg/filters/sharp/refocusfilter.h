/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-05-25
 * Description : Refocus threaded image filter.
 *
 * Copyright (C) 2005-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009 by Matthias Welwarsky <matze at welwarsky dot de>
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

#ifndef REFOCUSFILTER_H
#define REFOCUSFILTER_H

// Local includes

#include "digikam_export.h"
#include "dimgthreadedfilter.h"
#include "globals.h"

namespace Digikam
{

class DIGIKAM_EXPORT RefocusFilter : public DImgThreadedFilter
{

public:

    explicit RefocusFilter(QObject* parent = 0);
    explicit RefocusFilter(DImg* orgImage, QObject* parent=0, int matrixSize=5, double radius=0.9,
                           double gauss=0.0, double correlation=0.5, double noise=0.01);

    ~RefocusFilter();

    static int maxMatrixSize();

    static QString          FilterIdentifier()
    {
        return "digikam:RefocusFilter";
    }
    static QString          DisplayableName()
    {
        return I18N_NOOP("Refocus");
    }
    static QList<int>       SupportedVersions()
    {
        return QList<int>() << 1;
    }
    static int              CurrentVersion()
    {
        return 1;
    }

    virtual QString         filterIdentifier() const
    {
        return FilterIdentifier();
    }
    virtual FilterAction    filterAction();
    void                    readParameters(const FilterAction& action);

private:

    void filterImage();

    void refocusImage(uchar* data, int width, int height, bool sixteenBit,
                      int matrixSize, double radius, double gauss,
                      double correlation, double noise);

    void convolveImage(uchar* orgData, uchar* destData, int width, int height,
                       bool sixteenBit, const double* const matrix, int mat_size);

private:

    DImg   m_preImage;

    int    m_matrixSize;

    double m_radius;
    double m_gauss;
    double m_correlation;
    double m_noise;
};

}  // namespace Digikam

#endif /* REFOCUSFILTER_H */
