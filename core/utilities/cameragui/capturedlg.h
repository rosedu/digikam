/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2007-09-06
 * Description : a dialog to control camera capture.
 *
 * Copyright (C) 2007-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef CAPTUREDLG_H
#define CAPTUREDLG_H

// Qt includes

#include <QCloseEvent>

// KDE includes

#include <kdialog.h>

// Local includes

#include "digikam_export.h"

class QWidget;

namespace Digikam
{

class CameraController;
class CaptureDlgPriv;

class CaptureDlg : public KDialog
{
    Q_OBJECT

public:

    CaptureDlg(QWidget* parent, CameraController* controller,
               const QString& cameraTitle);
    ~CaptureDlg();

protected:

    void closeEvent(QCloseEvent* e);

private Q_SLOTS:

    void slotPreview();
    void slotPreviewDone(const QImage&);
    void slotCapture();
    void slotCancel();

private:

    CaptureDlgPriv* const d;
};

}  // namespace Digikam

#endif /* CAPTUREDLG_H */
