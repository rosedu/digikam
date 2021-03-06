/* ============================================================
 *
 * Date        : 2008-02-10
 * Description : a tool to fix automatically camera lens aberrations
 *
 * Copyright (C) 2008 by Adrian Schroeter <adrian at suse dot de>
 * Copyright (C) 2008-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "lensfunsettings.moc"

// Qt includes

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QString>
#include <QWidget>

// KDE includes

#include <kcombobox.h>
#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>

namespace Digikam
{

class LensFunSettings::LensFunSettingsPriv
{
public:

    LensFunSettingsPriv() :
        filterCCA(0),
        filterVIG(0),
        filterCCI(0),
        filterDST(0),
        filterGEO(0)
    {}

    static const QString configCCAEntry;
    static const QString configVignettingEntry;
    static const QString configCCIEntry;
    static const QString configDistortionEntry;
    static const QString configGeometryEntry;

    QCheckBox*           filterCCA;
    QCheckBox*           filterVIG;
    QCheckBox*           filterCCI;
    QCheckBox*           filterDST;
    QCheckBox*           filterGEO;
};
const QString LensFunSettings::LensFunSettingsPriv::configCCAEntry("CCA");
const QString LensFunSettings::LensFunSettingsPriv::configVignettingEntry("Vignetting");
const QString LensFunSettings::LensFunSettingsPriv::configCCIEntry("CCI");
const QString LensFunSettings::LensFunSettingsPriv::configDistortionEntry("Distortion");
const QString LensFunSettings::LensFunSettingsPriv::configGeometryEntry("Geometry");

// --------------------------------------------------------

LensFunSettings::LensFunSettings(QWidget* parent)
    : QWidget(parent),
      d(new LensFunSettingsPriv)
{
    QGridLayout* grid = new QGridLayout(this);

    QLabel* title = new QLabel(i18n("Lens Corrections to Apply:"));
    d->filterCCA  = new QCheckBox(i18n("Chromatic Aberration"));
    d->filterCCA->setWhatsThis(i18n("Chromatic aberration is easily recognized as color fringes "
                                    "towards the image corners. CA is due to a varying lens focus "
                                    "for different colors."));
    d->filterVIG  = new QCheckBox(i18n("Vignetting"));
    d->filterVIG->setWhatsThis(i18n("Vignetting refers to an image darkening, mostly in the corners. "
                                    "Optical and natural vignetting can be canceled out with this option, "
                                    "whereas mechanical vignetting will not be cured."));
    d->filterCCI  = new QCheckBox(i18n("Color"));
    d->filterCCI->setWhatsThis(i18n("All lenses have a slight color tinge to them, "
                                    "mostly due to the anti-reflective coating. "
                                    "The tinge can be canceled when the respective data is known for the lens."));
    d->filterDST = new QCheckBox(i18n("Distortion"));
    d->filterDST->setWhatsThis(i18n("Distortion refers to an image deformation, which is most pronounced "
                                    "towards the corners. These Seidel aberrations are known as pincushion "
                                    "and barrel distortions."));
    d->filterGEO = new QCheckBox(i18n("Geometry"));
    d->filterGEO->setWhatsThis(i18n("Four geometries are handled here: Rectilinear (99 percent of all lenses), "
                                    "Fisheye, Cylindrical, Equirectangular."));
    QLabel* note  = new QLabel(i18n("<b>Note: lens correction options depend of filters available in LensFun library. "
                                    "See <a href='http://lensfun.berlios.de'>LensFun project web site</a> "
                                    "for more information.</b>"), this);
    note->setOpenExternalLinks(true);
    note->setWordWrap(true);
    note->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    grid->addWidget(title,        0, 0, 1, 2);
    grid->addWidget(d->filterCCA, 1, 0, 1, 2);
    grid->addWidget(d->filterVIG, 2, 0, 1, 2);
    grid->addWidget(d->filterCCI, 3, 0, 1, 2);
    grid->addWidget(d->filterDST, 4, 0, 1, 2);
    grid->addWidget(d->filterGEO, 5, 0, 1, 2);
    grid->addWidget(note,         6, 0, 1, 2);
    grid->setRowStretch(7, 10);
    grid->setMargin(KDialog::spacingHint());
    grid->setSpacing(KDialog::spacingHint());

    connect(d->filterCCA, SIGNAL(toggled(bool)),
            this, SIGNAL(signalSettingsChanged()));

    connect(d->filterVIG, SIGNAL(toggled(bool)),
            this, SIGNAL(signalSettingsChanged()));

    connect(d->filterCCI, SIGNAL(toggled(bool)),
            this, SIGNAL(signalSettingsChanged()));

    connect(d->filterDST, SIGNAL(toggled(bool)),
            this, SIGNAL(signalSettingsChanged()));

    connect(d->filterGEO, SIGNAL(toggled(bool)),
            this, SIGNAL(signalSettingsChanged()));
}

LensFunSettings::~LensFunSettings()
{
    delete d;
}

void LensFunSettings::setEnabledCCA(bool b)
{
    d->filterCCA->setEnabled(b);
}

void LensFunSettings::setEnabledVig(bool b)
{
    d->filterVIG->setEnabled(b);
}

void LensFunSettings::setEnabledCCI(bool b)
{
    d->filterCCI->setEnabled(b);
}

void LensFunSettings::setEnabledDist(bool b)
{
    d->filterDST->setEnabled(b);
}

void LensFunSettings::setEnabledGeom(bool b)
{
    d->filterGEO->setEnabled(b);
}

void LensFunSettings::assignFilterSettings(LensFunContainer& prm)
{
    prm.filterCCA = (d->filterCCA->isChecked() && d->filterCCA->isEnabled());
    prm.filterVIG = (d->filterVIG->isChecked() && d->filterVIG->isEnabled());
    prm.filterCCI = (d->filterCCI->isChecked() && d->filterCCI->isEnabled());
    prm.filterDST = (d->filterDST->isChecked() && d->filterDST->isEnabled());
    prm.filterGEO = (d->filterGEO->isChecked() && d->filterGEO->isEnabled());
}

void LensFunSettings::setFilterSettings(const LensFunContainer& settings)
{
    blockSignals(true);
    d->filterCCA->setChecked(settings.filterCCA);
    d->filterVIG->setChecked(settings.filterVIG);
    d->filterCCI->setChecked(settings.filterCCI);
    d->filterDST->setChecked(settings.filterDST);
    d->filterGEO->setChecked(settings.filterGEO);
    blockSignals(false);
}

void LensFunSettings::resetToDefault()
{
    setFilterSettings(LensFunContainer());
}

LensFunContainer LensFunSettings::defaultSettings() const
{
    LensFunContainer prm;
    return prm;
}

void LensFunSettings::readSettings(KConfigGroup& group)
{
    LensFunContainer prm;
    LensFunContainer defaultPrm = defaultSettings();
    prm.filterCCA = group.readEntry(d->configCCAEntry,        defaultPrm.filterCCA);
    prm.filterVIG = group.readEntry(d->configVignettingEntry, defaultPrm.filterVIG);
    prm.filterCCI = group.readEntry(d->configCCIEntry,        defaultPrm.filterCCI);
    prm.filterDST = group.readEntry(d->configDistortionEntry, defaultPrm.filterDST);
    prm.filterGEO = group.readEntry(d->configGeometryEntry,   defaultPrm.filterGEO);
    setFilterSettings(prm);
}

void LensFunSettings::writeSettings(KConfigGroup& group)
{
    LensFunContainer prm;
    assignFilterSettings(prm);

    if ( d->filterCCA->isEnabled() )
    {
        group.writeEntry(d->configCCAEntry,        (prm.filterCCA));
    }

    if ( d->filterVIG->isEnabled() )
    {
        group.writeEntry(d->configVignettingEntry, (prm.filterVIG));
    }

    if ( d->filterCCI->isEnabled() )
    {
        group.writeEntry(d->configCCIEntry,        (prm.filterCCI));
    }

    if ( d->filterDST->isEnabled() )
    {
        group.writeEntry(d->configDistortionEntry, (prm.filterDST));
    }

    if ( d->filterGEO->isEnabled() )
    {
        group.writeEntry(d->configGeometryEntry,   (prm.filterGEO));
    }
}

}  // namespace Digikam
