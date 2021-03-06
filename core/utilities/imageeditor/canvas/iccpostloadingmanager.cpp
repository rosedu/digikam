/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-08-12
 * Description : methods that implement color management tasks
 *
 * Copyright (C) 2005-2006 by F.J. Cruz <fj.cruz@supercable.es>
 * Copyright (C) 2005-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009-2010 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#include "iccpostloadingmanager.h"

// Qt includes

#include <QPointer>

// KDE includes

#include <kdebug.h>

// Local includes

#include "colorcorrectiondlg.h"
#include "iccprofile.h"
#include "icctransform.h"

namespace Digikam
{

IccPostLoadingManager::IccPostLoadingManager(DImg& image, const QString& filePath, const ICCSettingsContainer& settings)
    : IccManager(image, settings), m_filePath(filePath)
{
}

IccTransform IccPostLoadingManager::postLoadingManage(QWidget* parent)
{
    if (image().hasAttribute("missingProfileAskUser"))
    {
        image().removeAttribute("missingProfileAskUser");
        DImg preview                     = image().smoothScale(240, 180, Qt::KeepAspectRatio);
        QPointer<ColorCorrectionDlg> dlg = new ColorCorrectionDlg(ColorCorrectionDlg::MissingProfile, preview,
                m_filePath, parent);
        dlg->exec();

        IccTransform trans;
        getTransform(trans, dlg->behavior(), dlg->specifiedProfile());
        delete dlg;
        return trans;
    }
    else if (image().hasAttribute("profileMismatchAskUser"))
    {
        image().removeAttribute("profileMismatchAskUser");
        DImg preview                     = image().smoothScale(240, 180, Qt::KeepAspectRatio);
        QPointer<ColorCorrectionDlg> dlg = new ColorCorrectionDlg(ColorCorrectionDlg::ProfileMismatch, preview,
                m_filePath, parent);
        dlg->exec();

        IccTransform trans;
        getTransform(trans, dlg->behavior(), dlg->specifiedProfile());
        delete dlg;
        return trans;
    }
    else if (image().hasAttribute("uncalibratedColorAskUser"))
    {
        image().removeAttribute("uncalibratedColorAskUser");
        DImg preview                     = image().smoothScale(240, 180, Qt::KeepAspectRatio);
        QPointer<ColorCorrectionDlg> dlg = new ColorCorrectionDlg(ColorCorrectionDlg::UncalibratedColor, preview,
                m_filePath, parent);
        dlg->exec();

        IccTransform trans;
        getTransform(trans, dlg->behavior(), dlg->specifiedProfile());
        delete dlg;
        return trans;
    }

    return IccTransform();
}

}  // namespace Digikam
