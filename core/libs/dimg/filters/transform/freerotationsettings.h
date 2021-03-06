/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2010-03-16
 * Description : Free rotation settings view.
 *
 * Copyright (C) 2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef FREEROTATIONSETTINGS_H
#define FREEROTATIONSETTINGS_H

// Local includes

#include <QWidget>

// KDE includes

#include <kconfig.h>

// Local includes

#include "digikam_export.h"
#include "freerotationfilter.h"

namespace Digikam
{

class FreeRotationSettingsPriv;

class DIGIKAM_EXPORT FreeRotationSettings : public QWidget
{
    Q_OBJECT

public:

    FreeRotationSettings(QWidget* parent);
    ~FreeRotationSettings();

    FreeRotationContainer defaultSettings() const;
    void resetToDefault();

    FreeRotationContainer settings() const;
    void setSettings(const FreeRotationContainer& settings);

    void readSettings(KConfigGroup& group);
    void writeSettings(KConfigGroup& group);

Q_SIGNALS:

    void signalSettingsChanged();

private:

    FreeRotationSettingsPriv* const d;
};

}  // namespace Digikam

#endif /* FREEROTATIONSETTINGS_H */
