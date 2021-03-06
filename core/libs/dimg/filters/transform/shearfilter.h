/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-07-18
 * Description : Shear tool threaded image filter.
 *
 * Copyright (C) 2005-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef SHEARFILTER_H
#define SHEARFILTER_H

// Qt includes

#include <QSize>
#include <QColor>

// Local includes

#include "digikam_export.h"
#include "dimgthreadedfilter.h"
#include "globals.h"

namespace Digikam
{

class DIGIKAM_EXPORT ShearFilter : public DImgThreadedFilter
{

public:

    explicit ShearFilter(QObject* parent = 0);
    explicit ShearFilter(DImg* orgImage, QObject* parent=0, float hAngle=0.0, float vAngle=0.0,
                         bool antialiasing=true, const QColor& backgroundColor=Qt::black, int orgW=0, int orgH=0);
    ~ShearFilter();

    QSize getNewSize(void)
    {
        return m_newSize;
    };

    static QString          FilterIdentifier()
    {
        return "digikam:ShearFilter";
    }
    static QString          DisplayableName()
    {
        return I18N_NOOP("Shear Tool");
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

    inline int setPosition (int Width, int X, int Y)
    {
        return (Y *Width*4 + 4*X);
    };

    inline bool isInside (int Width, int Height, int X, int Y)
    {
        bool bIsWOk = ((X < 0) ? false : (X >= Width ) ? false : true);
        bool bIsHOk = ((Y < 0) ? false : (Y >= Height) ? false : true);
        return (bIsWOk && bIsHOk);
    };

private:

    bool   m_antiAlias;

    int    m_orgW;
    int    m_orgH;

    float  m_hAngle;
    float  m_vAngle;

    QColor m_backgroundColor;

    QSize  m_newSize;
};

}  // namespace Digikam

#endif /* SHEARFILTER_H */
