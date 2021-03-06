/* ============================================================
 *
 * This file is a part of kipi-plugins project
 * http://www.kipi-plugins.org
 *
 * Date        : 2009-11-13
 * Description : a plugin to blend bracketed images.
 *
 * Copyright (C) 2009-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef IMPORTWIZARD_DLG_H
#define IMPORTWIZARD_DLG_H

// Qt includes

#include <QString>
#include <QWidget>

// KDE includes

#include <kurl.h>
#include <kassistantdialog.h>

// Local includes

#include "actions.h"

class KPageWidgetItem;

namespace KIPI
{
    class Interface;
}

using namespace KIPI;

namespace KIPIExpoBlendingPlugin
{

class Manager;
class ExpoBlendingAboutData;
class ImportWizardDlgPriv;

class ImportWizardDlg : public KAssistantDialog
{
    Q_OBJECT

public:

    ImportWizardDlg(Manager* mngr, QWidget* parent=0);
    ~ImportWizardDlg();

    KUrl::List itemUrls() const;

    Manager* manager() const;

private Q_SLOTS:

    void next();
    void back();

    void slotItemsPageIsValid(bool);
    void slotPreProcessed(const ItemUrlsMap&);
    void slotHelp();

private:

    ImportWizardDlgPriv* const d;
};

}   // namespace KIPIExpoBlendingPlugin

#endif /* IMPORTWIZARD_DLG_H */
