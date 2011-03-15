/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-11-03
 * Description : a busy dialog for digiKam
 *
 * Copyright (C) 2009 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef DBUSYDLG_H
#define DBUSYDLG_H

// Qt includes

#include <QThread>
#include <QString>

// KDE includes

#include <kprogressdialog.h>

// Local includes

#include "digikam_export.h"

namespace Digikam
{

class DBusyThread;
class DBusyDlgPriv;

class DIGIKAM_EXPORT DBusyDlg : public KProgressDialog
{
    Q_OBJECT

public:

    explicit DBusyDlg(const QString& txt, QWidget* parent=0);
    virtual ~DBusyDlg();

    void setBusyThread(DBusyThread* thread);

public Q_SLOTS:

    void slotComplete();

private:

    DBusyDlgPriv* const d;
};

// ----------------------------------------------------------------------------------

class DIGIKAM_EXPORT DBusyThread : public QThread
{
    Q_OBJECT

public:

    DBusyThread(QObject* parent);
    virtual ~DBusyThread();

Q_SIGNALS:

    void signalComplete();

protected:

    virtual void run();
};

}  // namespace Digikam

#endif  // DBUSYDLG_H